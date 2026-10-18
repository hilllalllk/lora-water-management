// Protocol tests. The byte vectors are the same ones the C++ tests use
// (firmware/test/test_protocol), so both implementations must agree.
import assert from 'node:assert/strict';
import { describe, it } from 'node:test';

import {
  crc16,
  decodeCmdResult,
  decodeCommand,
  DecodeError,
  decodeFrame,
  decodeTelemetry,
  DuplicateFilter,
  encodeCmdResult,
  encodeCommand,
  encodeFrame,
  encodeTelemetry,
  FrameAssembler,
  GATEWAY_ADDRESS,
  MAX_FRAME,
  MAX_PAYLOAD,
  MsgType,
  Opcode,
  ReliableSender,
  Result,
} from '../src/protocol/protocol.ts';

const hex = (s: string) => Uint8Array.from(s.split(/\s+/).map((b) => parseInt(b, 16)));
const toHex = (b: Uint8Array) => Array.from(b, (x) => x.toString(16).padStart(2, '0').toUpperCase()).join(' ');

describe('crc16', () => {
  it('matches the CRC-16/CCITT-FALSE check value', () => {
    assert.equal(crc16(new TextEncoder().encode('123456789')), 0x29b1);
  });
});

describe('frame', () => {
  it('encodes the documented telemetry vector (same as C++)', () => {
    const frame = encodeFrame({
      type: MsgType.Telemetry,
      src: 0x1001,
      seq: 42,
      payload: encodeTelemetry({ levelMm: 1250, valve: 1 }),
    });
    assert.equal(toHex(frame), '01 01 10 01 2A 07 01 02 04 E2 02 01 01 45 CB');
  });

  it('decodes every frame of the documented conversation (docs/03 §7)', () => {
    const cmd = decodeFrame(hex('01 10 00 01 C9 03 00 0F 01 8B 42'));
    assert.equal(cmd.type, MsgType.Command);
    assert.deepEqual(decodeCommand(cmd.payload), { cmdId: 15, op: Opcode.ValveOpen, args: new Uint8Array() });

    const res = decodeFrame(hex('01 11 10 01 2C 06 00 0F 00 02 01 01 96 3E'));
    assert.equal(res.src, 0x1001);
    assert.deepEqual(decodeCmdResult(res.payload), { cmdId: 15, result: Result.Ok, state: { valve: 1 } });

    const ack = decodeFrame(hex('01 7F 10 01 2B 01 C9 B6 BD'));
    assert.equal(ack.type, MsgType.Ack);
    assert.equal(ack.payload[0], 201);

    const hb = decodeFrame(hex('01 02 10 01 2D 09 00 00 0E 10 30 2E 31 2E 30 F7 11'));
    assert.equal(new TextDecoder().decode(hb.payload.subarray(4)), '0.1.0');
  });

  it('fits in one E32 packet and rejects longer payloads', () => {
    const f = encodeFrame({ type: MsgType.Telemetry, src: 1, seq: 0, payload: new Uint8Array(MAX_PAYLOAD) });
    assert.equal(f.length, MAX_FRAME);
    assert.throws(() =>
      encodeFrame({ type: MsgType.Telemetry, src: 1, seq: 0, payload: new Uint8Array(MAX_PAYLOAD + 1) }),
    );
  });

  it('detects corruption', () => {
    const f = encodeFrame({ type: MsgType.Heartbeat, src: 1, seq: 0, payload: Uint8Array.of(1, 2) });
    const code = (data: Uint8Array) => {
      try {
        decodeFrame(data);
        return 'ok';
      } catch (e) {
        return (e as DecodeError).code;
      }
    };
    const bad = f.slice();
    bad[6]! ^= 1;
    assert.equal(code(bad), 'bad_crc');
    assert.equal(code(f.subarray(0, f.length - 1)), 'bad_length');
    assert.equal(code(f.subarray(0, 4)), 'too_short');
    const v2 = f.slice();
    v2[0] = 2;
    assert.equal(code(v2), 'bad_version');
  });
});

describe('telemetry TLV', () => {
  it('round-trips and skips unknown tags', () => {
    const tlv = encodeTelemetry({ levelMm: 900, pressureCbar: 210, flowDlMin: 35, alarms: 1 });
    const withUnknown = Uint8Array.from([...tlv, 0x40, 2, 0x12, 0x34]);
    assert.deepEqual(decodeTelemetry(withUnknown), { levelMm: 900, pressureCbar: 210, flowDlMin: 35, alarms: 1 });
  });

  it('rejects truncated input', () => {
    assert.throws(() => decodeTelemetry(Uint8Array.of(0x01, 0x02, 0x04)));
  });
});

describe('command', () => {
  it('round-trips with arguments', () => {
    const bytes = encodeCommand({ cmdId: 0xbeef, op: Opcode.SetPeriod, args: Uint8Array.of(0x00, 0x3c) });
    assert.equal(toHex(bytes), 'BE EF 05 00 3C');
    assert.deepEqual(decodeCommand(bytes), { cmdId: 0xbeef, op: Opcode.SetPeriod, args: Uint8Array.of(0, 0x3c) });
  });

  it('encodes a command result', () => {
    assert.equal(toHex(encodeCmdResult({ cmdId: 7, result: Result.Ok, state: { valve: 1 } })), '00 07 00 02 01 01');
  });
});

describe('DuplicateFilter', () => {
  it('flags repeated SEQ per sender', () => {
    const f = new DuplicateFilter();
    assert.equal(f.seen(0x1001, 5), false);
    assert.equal(f.seen(0x1001, 5), true);
    assert.equal(f.seen(0x1002, 5), false);
    assert.equal(f.seen(0x1001, 6), false);
  });
});

describe('FrameAssembler', () => {
  const a = encodeFrame({ type: MsgType.Telemetry, src: 0x1001, seq: 1, payload: Uint8Array.of(0x55) });
  const b = encodeFrame({ type: MsgType.Heartbeat, src: 0x1002, seq: 2, payload: new Uint8Array() });

  it('splits a byte stream into frames, even when chunks are cut mid-frame', () => {
    const asm = new FrameAssembler();
    const stream = Uint8Array.from([0xff, ...a, ...b]); // leading noise byte
    const frames = [...asm.push(stream.subarray(0, 5), 100), ...asm.push(stream.subarray(5), 110)];
    assert.equal(frames.length, 2);
    assert.equal(decodeFrame(frames[1]!).src, 0x1002);
  });

  it('drops a partial frame after a long gap', () => {
    const asm = new FrameAssembler(50);
    assert.equal(asm.push(a.subarray(0, 4), 0).length, 0);
    assert.equal(asm.push(a, 1000).length, 1);
  });
});

describe('ReliableSender', () => {
  it('stops waiting when the matching ACK arrives', async () => {
    const sent: number[] = [];
    const tx = new ReliableSender((dst) => void sent.push(dst));
    await tx.sendFrame(0x1001, { type: MsgType.Command, src: GATEWAY_ADDRESS, seq: 9, payload: new Uint8Array(3) }, 0);
    assert.equal(tx.busy, true);
    assert.equal(tx.onAck(0x1001, 8), false);
    assert.equal(tx.onAck(0x1002, 9), false);
    assert.equal(tx.onAck(0x1001, 9), true);
    assert.equal(tx.busy, false);
    assert.equal(sent.length, 1);
  });

  it('retries twice, then fails', async () => {
    let sends = 0;
    const tx = new ReliableSender(() => void sends++, 2000, 3);
    await tx.sendFrame(0x1001, { type: MsgType.Command, src: 1, seq: 3, payload: new Uint8Array(3) }, 0);
    assert.deepEqual(await tx.poll(1999), { kind: 'none' });
    assert.deepEqual(await tx.poll(2000), { kind: 'retried' });
    assert.deepEqual(await tx.poll(4000), { kind: 'retried' });
    assert.deepEqual(await tx.poll(6000), { kind: 'failed', seq: 3 });
    assert.equal(sends, 3);
  });

  it('does not wait for an ACK for an ACK', async () => {
    const tx = new ReliableSender(() => {});
    await tx.sendFrame(0x1001, { type: MsgType.Ack, src: 1, seq: 1, payload: Uint8Array.of(0) }, 0);
    assert.equal(tx.busy, false);
  });
});
