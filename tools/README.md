# Tools (TypeScript)

Developer tools for the LoRa protocol. They run on a PC (Windows, macOS or Linux) with Node.js 22+.

| File | Purpose |
|---|---|
| `src/protocol/protocol.ts` | Protocol v1 in TypeScript (frames, CRC16, TLV, ACK/retry). Byte-compatible with `firmware/lib/LoraProtocol` |
| `src/commands.ts` | Database command (`VANA_AC`, `PERIYOT_AYARLA`, ...) to protocol opcode + argument bytes |
| `src/cli/frame-tool.ts` | Decodes and builds frames from the command line |
| `test/` | The same byte vectors as the C++ unit tests, so both implementations must agree |

```bash
cd tools
npm ci
npm test
npm run frame -- decode "01 01 10 01 2A 07 01 02 04 E2 02 01 01 45 CB"
npm run frame -- telemetry --src 0x1001 --seq 1 --level 1250 --valve 1
npm run frame -- command --seq 7 --cmd-id 15 --op VANA_AC
```

Use `decode` on the hex bytes printed by a node's or the gateway's serial monitor to see what was sent.
When the protocol changes, update both implementations and the shared vectors in `docs/03-protokol.md`.
