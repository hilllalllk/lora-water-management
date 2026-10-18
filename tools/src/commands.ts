// Database command rows (commands.op + commands.args) -> protocol opcode + argument bytes.
// The ESP32 gateway does the same mapping in C++ (firmware/src/gateway/main.cpp).
import { Opcode } from './protocol/protocol.ts';

const OPCODES: Record<string, Opcode> = {
  VANA_AC: Opcode.ValveOpen,
  VANA_KAPAT: Opcode.ValveClose,
  CIHAZ_ETKIN: Opcode.DeviceEnable,
  SIMDI_OKU: Opcode.ReadNow,
  PERIYOT_AYARLA: Opcode.SetPeriod,
};

/** Converts a database command row into protocol opcode + argument bytes. */
export function toProtocolCommand(op: string, args: Record<string, unknown> | null) {
  const opcode = OPCODES[op];
  if (opcode === undefined) return null;
  if (opcode === Opcode.DeviceEnable) {
    return { op: opcode, args: Uint8Array.of(args?.['enabled'] === false ? 0 : 1) };
  }
  if (opcode === Opcode.SetPeriod) {
    const s = Number(args?.['seconds'] ?? 60);
    return { op: opcode, args: Uint8Array.of((s >> 8) & 0xff, s & 0xff) };
  }
  return { op: opcode, args: new Uint8Array() };
}
