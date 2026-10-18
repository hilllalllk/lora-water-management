// Frame tool: decodes and builds LoRa protocol frames from the command line.
// Useful for reading the bytes printed on a node's serial monitor.
//
//   npm run frame -- decode "01 01 10 01 2A 07 01 02 04 E2 02 01 01 45 CB"
//   npm run frame -- telemetry --src 0x1001 --seq 1 --level 1250 --valve 1
//   npm run frame -- command --seq 7 --cmd-id 15 --op VANA_AC
import { realpathSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { parseArgs } from 'node:util';

import { toProtocolCommand } from '../commands.ts';
import {
  decodeCmdResult,
  decodeCommand,
  DecodeError,
  decodeFrame,
  decodeTelemetry,
  encodeCommand,
  encodeFrame,
  encodeTelemetry,
  GATEWAY_ADDRESS,
  MsgType,
  Opcode,
  Result,
  type Frame,
  type Telemetry,
} from '../protocol/protocol.ts';

export const toHex = (b: Uint8Array) =>
  Array.from(b, (x) => x.toString(16).padStart(2, '0').toUpperCase()).join(' ');

export function parseHex(s: string): Uint8Array {
  const clean = s.replace(/0x/gi, '').replace(/[^0-9a-f]/gi, '');
  if (clean.length % 2 !== 0) throw new Error('odd number of hex digits');
  return Uint8Array.from(clean.match(/../g) ?? [], (h) => parseInt(h, 16));
}

const num = (s: string | undefined, fallback?: number): number | undefined => {
  if (s === undefined) return fallback;
  const n = Number(s); // accepts 0x1001 as well as 4097
  if (!Number.isInteger(n)) throw new Error(`not an integer: ${s}`);
  return n;
};

/** Returns a human-readable description of a frame. Throws DecodeError. */
export function describe(bytes: Uint8Array): string {
  const f: Frame = decodeFrame(bytes);
  const lines = [
    `type    ${MsgType[f.type] ?? '?'} (0x${f.type.toString(16).padStart(2, '0')})`,
    `src     0x${f.src.toString(16).padStart(4, '0')}`,
    `seq     ${f.seq}`,
    `payload ${toHex(f.payload) || '(empty)'}`,
  ];
  switch (f.type) {
    case MsgType.Telemetry:
      lines.push(`data    ${JSON.stringify(decodeTelemetry(f.payload))}`);
      break;
    case MsgType.Command: {
      const c = decodeCommand(f.payload);
      lines.push(`command id=${c.cmdId} op=${Opcode[c.op] ?? c.op} args=${toHex(c.args ?? new Uint8Array()) || '-'}`);
      break;
    }
    case MsgType.CmdResult: {
      const r = decodeCmdResult(f.payload);
      lines.push(`result  id=${r.cmdId} ${Result[r.result] ?? r.result} state=${JSON.stringify(r.state)}`);
      break;
    }
    case MsgType.Ack:
      lines.push(`acks    seq ${f.payload[0]}`);
      break;
    case MsgType.Heartbeat:
      if (f.payload.length >= 4) {
        const uptime = ((f.payload[0]! << 24) | (f.payload[1]! << 16) | (f.payload[2]! << 8) | f.payload[3]!) >>> 0;
        lines.push(`uptime  ${uptime} s, fw ${new TextDecoder().decode(f.payload.subarray(4))}`);
      }
      break;
  }
  return lines.join('\n');
}

const USAGE = `usage:
  frame decode "<hex bytes>"
  frame telemetry --src 0x1001 --seq 1 [--level mm] [--valve 0-3] [--pressure cbar] [--flow dl/min] [--alarms n] [--supply mV]
  frame command --seq 7 --cmd-id 15 --op VANA_AC|VANA_KAPAT|CIHAZ_ETKIN|SIMDI_OKU|PERIYOT_AYARLA [--seconds n] [--enabled true|false]`;

export function run(argv: string[]): string {
  const { values, positionals } = parseArgs({
    args: argv,
    allowPositionals: true,
    options: {
      src: { type: 'string' },
      seq: { type: 'string' },
      level: { type: 'string' },
      valve: { type: 'string' },
      pressure: { type: 'string' },
      flow: { type: 'string' },
      alarms: { type: 'string' },
      supply: { type: 'string' },
      'cmd-id': { type: 'string' },
      op: { type: 'string' },
      seconds: { type: 'string' },
      enabled: { type: 'string' },
    },
  });
  const [sub, ...rest] = positionals;

  switch (sub) {
    case 'decode':
      return describe(parseHex(rest.join(' ')));
    case 'telemetry': {
      const t: Telemetry = {
        levelMm: num(values.level),
        valve: num(values.valve),
        pressureCbar: num(values.pressure),
        flowDlMin: num(values.flow),
        alarms: num(values.alarms),
        supplyMv: num(values.supply),
      };
      for (const k of Object.keys(t) as (keyof Telemetry)[]) if (t[k] === undefined) delete t[k];
      const payload = encodeTelemetry(t);
      return toHex(encodeFrame({ type: MsgType.Telemetry, src: num(values.src, 0x1001)!, seq: num(values.seq, 0)!, payload }));
    }
    case 'command': {
      const args: Record<string, unknown> = {};
      if (values.seconds !== undefined) args['seconds'] = num(values.seconds);
      if (values.enabled !== undefined) args['enabled'] = values.enabled !== 'false';
      const proto = toProtocolCommand(values.op ?? '', args);
      if (!proto) throw new Error(`unknown --op ${values.op ?? '(missing)'}`);
      const payload = encodeCommand({ cmdId: num(values['cmd-id'], 1)!, op: proto.op, args: proto.args });
      return toHex(encodeFrame({ type: MsgType.Command, src: GATEWAY_ADDRESS, seq: num(values.seq, 0)!, payload }));
    }
    default:
      throw new Error(USAGE);
  }
}

// Run only when started directly, not when imported by the tests (Node 22 has no import.meta.main).
const isMain = process.argv[1] !== undefined && realpathSync(process.argv[1]) === fileURLToPath(import.meta.url);
if (isMain) {
  try {
    console.log(run(process.argv.slice(2)));
  } catch (e) {
    console.error(e instanceof DecodeError ? `invalid frame: ${e.code}` : e instanceof Error ? e.message : String(e));
    process.exit(1);
  }
}
