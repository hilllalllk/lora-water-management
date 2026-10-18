# Firmware (ESP32 + E32)

PlatformIO project for both ends of the LoRa link. Every board is an ESP32 DevKit V1 with an Ebyte **E32-433T20D** (100 mW, 433 MHz) LoRa module:

- **End node**: reads the field device, sends telemetry, executes commands.
- **Gateway**: bridges LoRa to Supabase over Wi-Fi (HTTPS REST).

```
firmware/
├── platformio.ini            # envs: node, node2, node-modbus, gateway, native (tests)
├── include/
│   ├── config.h              # pins, channel, timings
│   ├── modbus_map.h          # Modbus register map of the field controller (node-modbus)
│   └── secrets.example.h     # copy to secrets.h (gateway Wi-Fi + Supabase credentials)
├── lib/
│   ├── LoraProtocol/         # protocol v1: frame, CRC, TLV, ACK/retransmission (hardware independent)
│   ├── ModbusRtu/            # minimal Modbus RTU master: CRC, 0x03/0x06 frames (hardware independent)
│   ├── E32Radio/             # E32 driver: modes, configuration, fixed-address sending
│   └── FieldDevice/          # field device interface + SimulatedDevice + ModbusRtuDevice
├── src/
│   ├── node/main.cpp         # end node
│   └── gateway/              # gateway: main.cpp, SupabaseRest, root_ca.h
└── test/
    ├── test_protocol/        # LoraProtocol unit tests
    └── test_modbus/          # ModbusRtu unit tests
```

## Environments

| env | Role | LoRa address | Field device |
|---|---|---|---|
| `node` | end node | 0x1001 | `SimulatedDevice` (LED GPIO2 = valve, potentiometer GPIO34 = level) |
| `node2` | end node | 0x1002 | `SimulatedDevice` |
| `node-modbus` | end node | 0x1003 | `ModbusRtuDevice` (existing controller over RS-485, `-DFIELD_DEVICE_MODBUS`) |
| `gateway` | gateway | 0x0001 | – (Wi-Fi + Supabase) |
| `native` | unit tests on the PC | – | – |

```bash
pio test -e native                   # protocol + Modbus tests (no board needed)
pio run -e node -t upload            # node 0x1001
pio run -e node2 -t upload           # node 0x1002
pio run -e node-modbus -t upload     # node 0x1003 with a Modbus controller
pio run -e gateway -t upload         # gateway (needs include/secrets.h)
pio device monitor                   # serial monitor, 115200 baud
```

## Radio: E32-433T20D

- Supply 3.3–5.2 V, about 130 mA while transmitting (power the module from 5 V/VIN with a 100 µF capacitor close to it, not from the ESP32 3.3 V regulator if it browns out).
- Power levels (`E32Power` in `E32Radio.h`): `Max` 20 dBm, `High` 17 dBm, `Medium` 14 dBm, `Min` 10 dBm (≈10 mW). The firmware defaults to `Min`, which matches the 10 mW limit for 433 MHz short-range devices in Turkey.

## Gateway

1. Create the gateway user in Supabase Auth and set its `app_metadata.role` to `"gateway"` (docs/05-kurulum.md). The gateway signs in as this user; RLS lets it update commands, insert telemetry and update devices/gateways. **Never** put the `service_role` key on the device.
2. Copy the secrets template and fill it in (`secrets.h` is gitignored):
   ```bash
   cp include/secrets.example.h include/secrets.h
   ```
   `WIFI_SSID`, `WIFI_PASSWORD` (2.4 GHz), `SUPABASE_URL`, `SUPABASE_ANON_KEY`, `GATEWAY_EMAIL`, `GATEWAY_PASSWORD`, `GATEWAY_ID`.
3. `pio run -e gateway -t upload`

What it does: polls `commands?status=eq.pending` every 1.5 s and forwards each command over LoRa (`sent` → `delivered` on ACK → `done`/`failed` on CMD_RESULT), inserts telemetry rows, updates `devices.last_seen`/`fw` on node heartbeats, and upserts its own `gateways` row (`last_seen`, `fw`, `wifi_rssi`) every 60 s. The LoRa address → device mapping is reloaded from `devices` every 5 minutes.

### TLS

HTTPS certificates are verified (no `setInsecure()`). `src/gateway/root_ca.h` bundles the root CAs Supabase's Cloudflare edge chains up to: ISRG Root X1/X2 (Let's Encrypt) and GTS Root R1/R4 (Google Trust Services). Validation needs the real date, so the gateway syncs its clock via NTP (`pool.ntp.org`, `time.google.com`, `time.cloudflare.com`) before the first request; the network must allow outbound UDP 123.

If the handshake starts failing with a certificate verification error, the chain has probably moved to another root: check it with `openssl s_client -connect <project>.supabase.co:443 -showcerts` and add the new root's PEM to `root_ca.h` (instructions are in the file).

## Field devices

The node talks to the field device only through `lib/FieldDevice/src/FieldDevice.h`. The adapter is chosen at build time: `SimulatedDevice` by default, `ModbusRtuDevice` when `FIELD_DEVICE_MODBUS` is defined (env `node-modbus`). The protocol, gateway, database and app stay the same for every adapter.

### Modbus RTU (`node-modbus`)

The sites already have digital controllers; this adapter reads and writes them over RS-485 with Modbus RTU (9600 baud 8N1, slave 1, 200 ms response timeout, t3.5 silence between frames).

| LoRa command | Modbus action |
|---|---|
| telemetry / `SIMDI_OKU` | read holding registers 0–4 (function 0x03): level_mm, valve state, pressure_cbar, flow_dl_min, alarms |
| `VANA_AC` / `VANA_KAPAT` | read valve state; if already there → `AlreadyInState`, else write register 10 = 1 / 0 (function 0x06) |
| `CIHAZ_ETKIN` | write register 11 = 1 / 0; while disabled, valve commands are rejected |
| `PERIYOT_AYARLA` | handled by the node itself (no bus traffic) |

Any timeout, CRC error or exception response gives `DeviceError` for commands; telemetry then carries only the `SensorFault` alarm. **The register map in `include/modbus_map.h` is a placeholder**: replace it with the addresses from the controller's manual (provided by the advisor / site operator).

Wiring (MAX3485, 3.3 V transceiver):

| MAX3485 pin | Connect to |
|---|---|
| RO | ESP32 GPIO25 (UART1 RX, `PIN_RS485_RX`) |
| DI | ESP32 GPIO26 (UART1 TX, `PIN_RS485_TX`) |
| DE + /RE (tied together) | ESP32 GPIO27 (`PIN_RS485_DE`, HIGH = transmit) |
| VCC / GND | 3.3 V / GND (share GND with the controller) |
| A / B | controller's RS-485 A(+) / B(−); 120 Ω termination at both ends of long lines |

### Testing Modbus without the real controller

Use a PC as the Modbus slave through a USB–RS485 adapter:

1. Connect the adapter's A/B to the MAX3485 A/B (and GND to GND).
2. Start a slave simulator on the adapter's serial port at 9600 8N1, slave ID 1, with holding registers 0–11:
   - **Modbus Slave** (Windows, Witte Software) or **ModRSsim2** (Windows, free): add a holding register table from address 0, length 12.
   - **diagslave** (Linux/Windows/macOS, command line): `diagslave -m rtu -a 1 -b 9600 -d 8 -s 1 -p none /dev/ttyUSB0`
3. Flash `pio run -e node-modbus -t upload` and open the serial monitor. Boot should print `[modbus] slave 1 online`.
4. Change registers 0–4 in the simulator and watch the node's telemetry; send `VANA_AC` from the app and check that register 10 becomes 1. Return an exception (e.g. remove the table) to see `DeviceError` and the `SensorFault` alarm.

If nothing answers: swap A/B, check the baud rate and slave ID, and make sure DE/RE is wired to GPIO27.

## Adding a new node

1. Add a new env to `platformio.ini`: `build_flags = ${esp32.build_flags} -DNODE_ADDRESS=0x1004` (add `-DFIELD_DEVICE_MODBUS` for a Modbus controller).
2. Add a row to the Supabase `devices` table: `address = 4100` (0x1004).
3. The gateway refreshes its device list every 5 minutes (or restart the gateway).

## Adding a new field device type

Write a new class implementing `FieldDevice.h` (for example for 4–20 mA or relay-based devices) and select it in `src/node/main.cpp` with its own build flag.

Details: [docs/02](../docs/02-sistem-mimarisi.md), [docs/03](../docs/03-protokol.md), [docs/04](../docs/04-donanim.md), [docs/05](../docs/05-kurulum.md).
