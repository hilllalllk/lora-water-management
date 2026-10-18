// LoRa link protocol v1 - TypeScript implementation.
//
// Byte-for-byte compatible with the C++ library in firmware/lib/LoraProtocol.
// Both are checked against the same test vectors (docs/03-protokol.md §7).

export const PROTOCOL_VERSION = 0x01;

export const E32_MAX_PACKET = 58; // one E32 radio packet carries at most 58 bytes
export const E32_FIXED_HEADER = 3; // ADDH, ADDL, CHAN (consumed by the module)
export const FRAME_HEADER = 6; // VER, TYPE, SRC(2), SEQ, LEN
export const FRAME_CRC = 2;
export const FRAME_OVERHEAD = FRAME_HEADER + FRAME_CRC; // 8
export const MAX_FRAME = E32_MAX_PACKET - E32_FIXED_HEADER; // 55
export const MAX_PAYLOAD = MAX_FRAME - FRAME_OVERHEAD; // 47
export const MAX_COMMAND_ARGS = 8;

export const GATEWAY_ADDRESS = 0x0001;
export const BROADCAST_ADDRESS = 0xffff;

export enum MsgType {
  Telemetry = 0x01,
  Heartbeat = 0x02,
  Command = 0x10,
  CmdResult = 0x11,
  Ack = 0x7f,
}

export enum Opcode {
  ValveOpen = 0x01,
  ValveClose = 0x02,
  DeviceEnable = 0x03,
  ReadNow = 0x04,
  SetPeriod = 0x05,
}

export enum Result {
  Ok = 0x00,
  UnknownCommand = 0x01,
  DeviceError = 0x02,
  Timeout = 0x03,
  AlreadyInState = 0x04,
}

export enum Tag {
  LevelMm = 0x01,
  Valve = 0x02,
  PressureCbar = 0x03,
  FlowDlMin = 0x04,
  Alarms = 0x05,
  SupplyMv = 0x06,
}

export function requiresAck(type: MsgType): boolean {
  return type === MsgType.Telemetry || type === MsgType.Command || type === MsgType.CmdResult;
}

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------

export interface Frame {
  type: MsgType;
  src: number;
  seq: number;
  payload: Uint8Array;
}

export class DecodeError extends Error {
  constructor(public readonly code: 'too_short' | 'bad_version' | 'bad_length' | 'bad_crc') {
    super(code);
  }
}

/** CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF). crc16("123456789") === 0x29B1 */
export function crc16(data: Uint8Array, crc = 0xffff): number {
  for (const byte of data) {
    crc ^= byte << 8;
    for (let b = 0; b < 8; b++) {
      crc = crc & 0x8000 ? ((crc << 1) ^ 0x1021) & 0xffff : (crc << 1) & 0xffff;
    }
  }
  return crc;
}

export function encodeFrame(f: Frame): Uint8Array {
  if (f.payload.length > MAX_PAYLOAD) throw new RangeError(`payload too long: ${f.payload.length}`);
  const out = new Uint8Array(FRAME_OVERHEAD + f.payload.length);
  out[0] = PROTOCOL_VERSION;
  out[1] = f.type;
  out[2] = (f.src >> 8) & 0xff;
  out[3] = f.src & 0xff;
  out[4] = f.seq & 0xff;
  out[5] = f.payload.length;
  out.set(f.payload, FRAME_HEADER);
  const crc = crc16(out.subarray(0, FRAME_HEADER + f.payload.length));
  out[FRAME_HEADER + f.payload.length] = crc >> 8;
  out[FRAME_HEADER + f.payload.length + 1] = crc & 0xff;
  return out;
}

/** Decodes and validates a frame. Throws DecodeError. */
export function decodeFrame(data: Uint8Array): Frame {
  if (data.length < FRAME_OVERHEAD) throw new DecodeError('too_short');
  if (data[0] !== PROTOCOL_VERSION) throw new DecodeError('bad_version');
  const len = data[5]!;
  if (len > MAX_PAYLOAD || data.length !== FRAME_OVERHEAD + len) throw new DecodeError('bad_length');
  const expected = (data[FRAME_HEADER + len]! << 8) | data[FRAME_HEADER + len + 1]!;
  if (crc16(data.subarray(0, FRAME_HEADER + len)) !== expected) throw new DecodeError('bad_crc');
  return {
    type: data[1] as MsgType,
    src: (data[2]! << 8) | data[3]!,
    seq: data[4]!,
    payload: data.slice(FRAME_HEADER, FRAME_HEADER + len),
  };
}

// ---------------------------------------------------------------------------
// Telemetry (TLV)
// ---------------------------------------------------------------------------

export interface Telemetry {
  levelMm?: number;
  valve?: number;
  pressureCbar?: number;
  flowDlMin?: number;
  alarms?: number;
  supplyMv?: number;
}

const TLV_FIELDS: { tag: Tag; key: keyof Telemetry; size: 1 | 2 }[] = [
  { tag: Tag.LevelMm, key: 'levelMm', size: 2 },
  { tag: Tag.Valve, key: 'valve', size: 1 },
  { tag: Tag.PressureCbar, key: 'pressureCbar', size: 2 },
  { tag: Tag.FlowDlMin, key: 'flowDlMin', size: 2 },
  { tag: Tag.Alarms, key: 'alarms', size: 1 },
  { tag: Tag.SupplyMv, key: 'supplyMv', size: 2 },
];

export function encodeTelemetry(t: Telemetry): Uint8Array {
  const bytes: number[] = [];
  for (const { tag, key, size } of TLV_FIELDS) {
    const v = t[key];
    if (v === undefined) continue;
    bytes.push(tag, size);
    if (size === 2) bytes.push((v >> 8) & 0xff);
    bytes.push(v & 0xff);
  }
  return Uint8Array.from(bytes);
}

/** Unknown tags are skipped (forward compatibility). Throws on malformed TLV. */
export function decodeTelemetry(data: Uint8Array): Telemetry {
  const t: Telemetry = {};
  let pos = 0;
  while (pos < data.length) {
    if (pos + 2 > data.length) throw new RangeError('truncated TLV header');
    const tag = data[pos]!;
    const len = data[pos + 1]!;
    pos += 2;
    if (pos + len > data.length) throw new RangeError('truncated TLV value');
    const field = TLV_FIELDS.find((f) => f.tag === tag);
    if (field) {
      if (len !== field.size) throw new RangeError(`bad length for tag ${tag}`);
      t[field.key] = len === 2 ? (data[pos]! << 8) | data[pos + 1]! : data[pos]!;
    }
    pos += len;
  }
  return t;
}

// ---------------------------------------------------------------------------
// Command and command result
// ---------------------------------------------------------------------------

export interface Command {
  cmdId: number;
  op: Opcode;
  args?: Uint8Array;
}

export function encodeCommand(c: Command): Uint8Array {
  const args = c.args ?? new Uint8Array();
  if (args.length > MAX_COMMAND_ARGS) throw new RangeError('too many command args');
  const out = new Uint8Array(3 + args.length);
  out[0] = (c.cmdId >> 8) & 0xff;
  out[1] = c.cmdId & 0xff;
  out[2] = c.op;
  out.set(args, 3);
  return out;
}

export function decodeCommand(data: Uint8Array): Command {
  if (data.length < 3 || data.length - 3 > MAX_COMMAND_ARGS) throw new RangeError('bad command');
  return { cmdId: (data[0]! << 8) | data[1]!, op: data[2] as Opcode, args: data.slice(3) };
}

export interface CmdResult {
  cmdId: number;
  result: Result;
  state: Telemetry;
}

export function encodeCmdResult(r: CmdResult): Uint8Array {
  const tlv = encodeTelemetry(r.state);
  const out = new Uint8Array(3 + tlv.length);
  out[0] = (r.cmdId >> 8) & 0xff;
  out[1] = r.cmdId & 0xff;
  out[2] = r.result;
  out.set(tlv, 3);
  return out;
}

export function decodeCmdResult(data: Uint8Array): CmdResult {
  if (data.length < 3) throw new RangeError('bad command result');
  return {
    cmdId: (data[0]! << 8) | data[1]!,
    result: data[2] as Result,
    state: decodeTelemetry(data.subarray(3)),
  };
}

export function makeAck(src: number, seq: number, ackSeq: number): Frame {
  return { type: MsgType.Ack, src, seq, payload: Uint8Array.of(ackSeq & 0xff) };
}

// ---------------------------------------------------------------------------
// Duplicate filter
// ---------------------------------------------------------------------------

/**
 * Remembers the last SEQ per sender. A repeated SEQ is a retransmission:
 * the receiver ACKs it again but does not process it.
 */
export class DuplicateFilter {
  private readonly last = new Map<number, number>();

  /** true = already seen (retransmission); false = new, now recorded. */
  seen(src: number, seq: number): boolean {
    if (this.last.get(src) === seq) return true;
    this.last.set(src, seq);
    return false;
  }
}

// ---------------------------------------------------------------------------
// Frame assembler (UART byte stream -> frames)
// ---------------------------------------------------------------------------

/**
 * The E32 outputs a radio packet as consecutive UART bytes. The assembler uses
 * the LEN field to find the end of a frame and drops a partial frame when the
 * gap between two bytes exceeds gapMs (resynchronisation).
 */
export class FrameAssembler {
  private buf: number[] = [];
  private lastByteMs = 0;

  constructor(private readonly gapMs = 50) {}

  /** Feeds bytes and returns every frame completed by them. */
  push(bytes: Uint8Array, nowMs: number): Uint8Array[] {
    const frames: Uint8Array[] = [];
    for (const byte of bytes) {
      if (this.buf.length > 0 && nowMs - this.lastByteMs > this.gapMs) this.buf = [];
      this.lastByteMs = nowMs;

      if (this.buf.length === 0 && byte !== PROTOCOL_VERSION) continue; // not a frame start
      this.buf.push(byte);

      if (this.buf.length >= FRAME_HEADER) {
        const len = this.buf[5]!;
        if (len > MAX_PAYLOAD) {
          this.buf = [];
        } else if (this.buf.length === FRAME_OVERHEAD + len) {
          frames.push(Uint8Array.from(this.buf));
          this.buf = [];
        }
      }
    }
    return frames;
  }
}

// ---------------------------------------------------------------------------
// Stop-and-wait reliable sender (ARQ)
// ---------------------------------------------------------------------------

export type SendFn = (dst: number, data: Uint8Array) => void | Promise<void>;

export type SenderEvent = { kind: 'none' } | { kind: 'retried' } | { kind: 'failed'; seq: number };

export class ReliableSender {
  private active = false;
  private dst = 0;
  private seq = 0;
  private tries = 0;
  private lastSendMs = 0;
  private bytes: Uint8Array = new Uint8Array();

  constructor(
    private readonly send: SendFn,
    private readonly ackTimeoutMs = 2000,
    private readonly maxTries = 3,
  ) {}

  get busy(): boolean {
    return this.active;
  }

  /** Sends a frame; frames that need an ACK are tracked until acknowledged. */
  async sendFrame(dst: number, frame: Frame, nowMs: number): Promise<boolean> {
    if (this.active) return false;
    const bytes = encodeFrame(frame);
    await this.send(dst, bytes);
    if (requiresAck(frame.type)) {
      this.active = true;
      this.dst = dst;
      this.seq = frame.seq;
      this.tries = 1;
      this.lastSendMs = nowMs;
      this.bytes = bytes;
    }
    return true;
  }

  /** Handles an incoming ACK. Returns true if it was the one we waited for. */
  onAck(from: number, ackSeq: number): boolean {
    if (!this.active || from !== this.dst || ackSeq !== this.seq) return false;
    this.active = false;
    return true;
  }

  /** Call often: retransmits on timeout and reports failure after maxTries. */
  async poll(nowMs: number): Promise<SenderEvent> {
    if (!this.active || nowMs - this.lastSendMs < this.ackTimeoutMs) return { kind: 'none' };
    if (this.tries >= this.maxTries) {
      this.active = false;
      return { kind: 'failed', seq: this.seq };
    }
    await this.send(this.dst, this.bytes);
    this.tries++;
    this.lastSendMs = nowMs;
    return { kind: 'retried' };
  }
}
