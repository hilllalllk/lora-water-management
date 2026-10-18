#!/usr/bin/env python3
"""LoRa çerçeve aracı: protokol v1 çerçevelerini oluşturur ve çözer.

Seri monitörde gördüğünüz baytları çözmek veya test için çerçeve üretmek için
kullanılır. Firmware'deki C++ kodu ile aynı kuralları uygular
(firmware/lib/LoraProtocol, docs/03-protokol.md).

Örnekler:
  python3 tools/frame_tool.py decode "01 01 10 01 2A 07 01 02 04 E2 02 01 01 xx xx"
  python3 tools/frame_tool.py telemetry --src 0x1001 --seq 42 --level 1250 --valve 1
  python3 tools/frame_tool.py command --seq 7 --cmd-id 15 --op VANA_AC
"""
from __future__ import annotations

import argparse
import struct
import sys

VERSION = 0x01
MAX_PAYLOAD = 47
GATEWAY = 0x0001

MSG_TYPES = {0x01: "TELEMETRY", 0x02: "HEARTBEAT", 0x10: "COMMAND", 0x11: "CMD_RESULT", 0x7F: "ACK"}
OPCODES = {"VANA_AC": 0x01, "VANA_KAPAT": 0x02, "CIHAZ_ETKIN": 0x03, "SIMDI_OKU": 0x04, "PERIYOT_AYARLA": 0x05}
RESULTS = {0: "OK", 1: "UNKNOWN_COMMAND", 2: "DEVICE_ERROR", 3: "TIMEOUT", 4: "ALREADY_IN_STATE"}
# etiket: (ad, bayt sayısı)
TAGS = {
    0x01: ("level_mm", 2),
    0x02: ("valve", 1),
    0x03: ("pressure_cbar", 2),
    0x04: ("flow_dl_min", 2),
    0x05: ("alarms", 1),
    0x06: ("supply_mv", 2),
}
TAG_BY_NAME = {name: (tag, size) for tag, (name, size) in TAGS.items()}


def crc16(data: bytes, crc: int = 0xFFFF) -> int:
    """CRC-16/CCITT-FALSE. crc16(b"123456789") == 0x29B1"""
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) if crc & 0x8000 else (crc << 1)
            crc &= 0xFFFF
    return crc


def encode_frame(msg_type: int, src: int, seq: int, payload: bytes) -> bytes:
    if len(payload) > MAX_PAYLOAD:
        raise ValueError(f"payload {len(payload)} bayt, en fazla {MAX_PAYLOAD}")
    body = struct.pack(">BBHBB", VERSION, msg_type, src, seq, len(payload)) + payload
    return body + struct.pack(">H", crc16(body))


def decode_frame(data: bytes) -> dict:
    if len(data) < 8:
        raise ValueError("çerçeve çok kısa")
    version, msg_type, src, seq, length = struct.unpack(">BBHBB", data[:6])
    if version != VERSION:
        raise ValueError(f"bilinmeyen sürüm {version}")
    if len(data) != 8 + length:
        raise ValueError(f"uzunluk uyuşmuyor: LEN={length}, toplam={len(data)}")
    payload = data[6 : 6 + length]
    (crc,) = struct.unpack(">H", data[6 + length :])
    if crc != crc16(data[: 6 + length]):
        raise ValueError(f"CRC hatalı: gelen {crc:04X}, hesaplanan {crc16(data[:6 + length]):04X}")
    return {"type": MSG_TYPES.get(msg_type, f"0x{msg_type:02X}"), "src": src, "seq": seq, "payload": payload}


def encode_tlv(values: dict) -> bytes:
    out = b""
    for name, value in values.items():
        tag, size = TAG_BY_NAME[name]
        out += bytes([tag, size]) + value.to_bytes(size, "big")
    return out


def decode_tlv(data: bytes) -> dict:
    out, pos = {}, 0
    while pos < len(data):
        if pos + 2 > len(data):
            raise ValueError("TLV yarım kaldı")
        tag, length = data[pos], data[pos + 1]
        value = data[pos + 2 : pos + 2 + length]
        if len(value) != length:
            raise ValueError("TLV değeri yarım kaldı")
        name = TAGS.get(tag, (f"tag_0x{tag:02X}", length))[0]
        out[name] = int.from_bytes(value, "big")
        pos += 2 + length
    return out


def describe(frame: dict) -> dict:
    p, t = frame["payload"], frame["type"]
    info = {"type": t, "src": f"0x{frame['src']:04X}", "seq": frame["seq"]}
    if t == "TELEMETRY":
        info["telemetry"] = decode_tlv(p)
    elif t == "COMMAND":
        cmd_id, op = struct.unpack(">HB", p[:3])
        names = {v: k for k, v in OPCODES.items()}
        info.update(cmd_id=cmd_id, op=names.get(op, f"0x{op:02X}"), args=p[3:].hex(" "))
    elif t == "CMD_RESULT":
        cmd_id, result = struct.unpack(">HB", p[:3])
        info.update(cmd_id=cmd_id, result=RESULTS.get(result, result), state=decode_tlv(p[3:]))
    elif t == "ACK":
        info["ack_seq"] = p[0]
    elif t == "HEARTBEAT":
        info.update(uptime_s=int.from_bytes(p[:4], "big"), fw=p[4:].decode(errors="replace"))
    return info


def parse_hex(text: str) -> bytes:
    return bytes.fromhex(text.replace("0x", "").replace(",", " "))


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)

    d = sub.add_parser("decode", help="hex baytları çöz")
    d.add_argument("hex")

    t = sub.add_parser("telemetry", help="TELEMETRY çerçevesi üret")
    t.add_argument("--src", type=lambda s: int(s, 0), default=0x1001)
    t.add_argument("--seq", type=int, default=0)
    for name in TAG_BY_NAME:
        t.add_argument(f"--{name.split('_')[0]}", dest=name, type=int)

    c = sub.add_parser("command", help="COMMAND çerçevesi üret (gateway -> düğüm)")
    c.add_argument("--seq", type=int, default=0)
    c.add_argument("--cmd-id", type=int, required=True)
    c.add_argument("--op", choices=OPCODES, required=True)
    c.add_argument("--args", default="", help="hex argüman baytları")

    a = ap.parse_args(argv)
    if a.cmd == "decode":
        try:
            print(describe(decode_frame(parse_hex(a.hex))))
        except ValueError as e:
            print(f"HATA: {e}", file=sys.stderr)
            return 1
        return 0
    if a.cmd == "telemetry":
        values = {n: getattr(a, n) for n in TAG_BY_NAME if getattr(a, n) is not None}
        frame = encode_frame(0x01, a.src, a.seq, encode_tlv(values))
    else:
        payload = struct.pack(">HB", a.cmd_id, OPCODES[a.op]) + parse_hex(a.args)
        frame = encode_frame(0x10, GATEWAY, a.seq, payload)
    print(frame.hex(" ").upper())
    return 0


if __name__ == "__main__":
    sys.exit(main())
