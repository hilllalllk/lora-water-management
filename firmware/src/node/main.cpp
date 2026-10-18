// End node (field station) firmware.
//
// Responsibilities (docs/02-sistem-mimarisi.md §2, §4):
//   - Read the field device periodically and send TELEMETRY
//   - Send immediately, without waiting for the period, when valve state / alarms change
//   - Execute COMMANDs from the gateway and reply with CMD_RESULT
//   - Wait for ACKs, retransmit when needed, and filter out duplicate packets
//
// Field device adapter, chosen at build time (platformio.ini):
//   default                  -> SimulatedDevice (LED + potentiometer)
//   -DFIELD_DEVICE_MODBUS    -> ModbusRtuDevice (existing controller over RS-485)
#include <Arduino.h>
#include <E32Radio.h>
#include <LoraProtocol.h>

#include "config.h"

#ifdef FIELD_DEVICE_MODBUS
#include <ModbusRtuDevice.h>

#include "modbus_map.h"
#else
#include <SimulatedDevice.h>
#endif

using namespace lwm;

namespace {

E32Radio radio(Serial2, {PIN_E32_RX, PIN_E32_TX, PIN_E32_M0, PIN_E32_M1, PIN_E32_AUX});
#ifdef FIELD_DEVICE_MODBUS
ModbusRegisterMap modbusMap() {
  ModbusRegisterMap m;
  m.slave = MODBUS_SLAVE_ID;
  m.levelMm = MODBUS_REG_LEVEL_MM;
  m.valveState = MODBUS_REG_VALVE_STATE;
  m.pressureCbar = MODBUS_REG_PRESSURE_CBAR;
  m.flowDlMin = MODBUS_REG_FLOW_DL_MIN;
  m.alarms = MODBUS_REG_ALARMS;
  m.valveCommand = MODBUS_REG_VALVE_CMD;
  m.enable = MODBUS_REG_ENABLE;
  return m;
}
ModbusRtuDevice device(Serial1, {PIN_RS485_RX, PIN_RS485_TX, PIN_RS485_DE}, MODBUS_BAUD,
                       modbusMap());
#define DEVICE_NAME "modbus-rtu"
#else
SimulatedDevice device(PIN_VALVE_LED, PIN_LEVEL_ADC);
#define DEVICE_NAME "simulated"
#endif

bool radioSend(uint16_t dst, const uint8_t* data, size_t len, void*) {
  return radio.send(dst, data, len);
}

ReliableSender sender(radioSend, nullptr, ACK_TIMEOUT_MS, MAX_TRIES);
FrameAssembler assembler(FRAME_GAP_MS);
DuplicateFilter duplicates;

uint8_t txSeq = 0;
uint16_t periodS = TELEMETRY_PERIOD_S_DEFAULT;
uint32_t nextTelemetryMs = 0;
uint32_t nextHeartbeatMs = 0;
uint32_t nextSampleMs = 0;
bool telemetryNow = true;  // send a reading right after boot

uint8_t lastValve = 0xFF;
uint8_t lastAlarms = 0xFF;

// Recent results so commands are not executed twice (cmdId -> result)
struct RecentResult {
  bool used;
  CmdResult result;
};
RecentResult recent[4] = {};
size_t recentNext = 0;

// Command result waiting to be sent (takes priority over telemetry)
bool resultPending = false;
CmdResult pendingResult;

uint32_t jitteredPeriodMs() {
  const int32_t base = static_cast<int32_t>(periodS) * 1000;
  const int32_t span = base * TELEMETRY_JITTER_PERCENT / 100;
  return static_cast<uint32_t>(base + random(-span, span + 1));
}

void sendAck(uint8_t ackSeq) {
  uint8_t buf[MAX_FRAME];
  const size_t n = encodeFrame(makeAck(NODE_ADDRESS, txSeq++, ackSeq), buf, sizeof(buf));
  radio.send(GATEWAY_ADDRESS, buf, n);
}

const CmdResult* findRecent(uint16_t cmdId) {
  for (auto& r : recent) {
    if (r.used && r.result.cmdId == cmdId) return &r.result;
  }
  return nullptr;
}

void remember(const CmdResult& r) {
  recent[recentNext] = {true, r};
  recentNext = (recentNext + 1) % (sizeof(recent) / sizeof(recent[0]));
}

void handleCommand(const Frame& f) {
  Command cmd;
  if (!decodeCommand(f.payload, f.len, cmd)) {
    Serial.println(F("[node] malformed command"));
    return;
  }

  if (const CmdResult* prev = findRecent(cmd.cmdId)) {
    // Same command arrived again (e.g. the previous CMD_RESULT was lost): don't execute, resend the result
    pendingResult = *prev;
    resultPending = true;
    return;
  }

  CmdResult res;
  res.cmdId = cmd.cmdId;
  switch (cmd.op) {
    case Opcode::ReadNow:
      res.result = Result::Ok;
      telemetryNow = true;
      break;
    case Opcode::SetPeriod:
      if (cmd.argLen == 2) {
        const uint16_t s = static_cast<uint16_t>((cmd.args[0] << 8) | cmd.args[1]);
        if (s >= 10) {
          periodS = s;
          nextTelemetryMs = millis() + jitteredPeriodMs();
          res.result = Result::Ok;
        } else {
          res.result = Result::UnknownCommand;
        }
      } else {
        res.result = Result::UnknownCommand;
      }
      break;
    default:
      res.result = device.execute(cmd.op, cmd.args, cmd.argLen);
      break;
  }
  device.read(res.state);
  if (res.state.hasValve) lastValve = res.state.valve;

  Serial.printf("[node] command %u op=0x%02X result=%u\n", cmd.cmdId,
                static_cast<unsigned>(cmd.op), static_cast<unsigned>(res.result));
  remember(res);
  pendingResult = res;
  resultPending = true;
}

void handleFrame(const Frame& f) {
  if (f.src != GATEWAY_ADDRESS) return;  // only listen to the gateway

  if (f.type == MsgType::Ack) {
    if (f.len == 1) sender.onAck(f.src, f.payload[0]);
    return;
  }

  if (requiresAck(f.type)) sendAck(f.seq);
  if (duplicates.seen(f.src, f.seq)) return;  // retransmission: already ACKed, don't process

  if (f.type == MsgType::Command) handleCommand(f);
}

void pollRadio() {
  while (radio.available()) {
    const uint8_t b = static_cast<uint8_t>(radio.read());
    if (!assembler.push(b, millis())) continue;
    Frame f;
    const DecodeError err = decodeFrame(assembler.frame(), assembler.frameLength(), f);
    if (err != DecodeError::None) {
      Serial.printf("[node] frame error: %s\n", decodeErrorName(err));
      continue;
    }
    handleFrame(f);
  }
}

bool sendPayload(MsgType type, const uint8_t* payload, size_t len) {
  Frame f;
  f.type = type;
  f.src = NODE_ADDRESS;
  f.seq = txSeq;
  f.len = static_cast<uint8_t>(len);
  memcpy(f.payload, payload, len);
  if (!sender.sendFrame(GATEWAY_ADDRESS, f, millis())) return false;
  ++txSeq;
  return true;
}

void sendTelemetry(const Telemetry& t) {
  uint8_t payload[MAX_PAYLOAD];
  const size_t n = encodeTelemetry(t, payload, sizeof(payload));
  if (sendPayload(MsgType::Telemetry, payload, n)) {
    Serial.printf("[node] telemetry: level=%u valve=%u alarms=0x%02X\n", t.levelMm, t.valve, t.alarms);
  }
}

void sendHeartbeat() {
  // Content: uptime (s, 4 bytes) + firmware version (text)
  uint8_t payload[MAX_PAYLOAD];
  const uint32_t up = millis() / 1000;
  payload[0] = up >> 24; payload[1] = up >> 16; payload[2] = up >> 8; payload[3] = up;
  const size_t vlen = strnlen(FIRMWARE_VERSION, MAX_PAYLOAD - 4);
  memcpy(payload + 4, FIRMWARE_VERSION, vlen);
  sendPayload(MsgType::Heartbeat, payload, 4 + vlen);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.printf("\n[node] address=0x%04X fw=%s device=%s\n", NODE_ADDRESS, FIRMWARE_VERSION,
                DEVICE_NAME);

  randomSeed(esp_random());
  txSeq = static_cast<uint8_t>(random(0, 256));  // avoid SEQ collisions after a reboot

  E32Config cfg;
  cfg.address = NODE_ADDRESS;
  cfg.channel = LORA_CHANNEL;
  if (!radio.begin(cfg)) Serial.println(F("[node] WARNING: could not verify E32 configuration"));

  if (!device.begin()) Serial.println(F("[node] WARNING: field device did not initialise"));
  nextTelemetryMs = millis() + jitteredPeriodMs();
  nextHeartbeatMs = millis() + HEARTBEAT_PERIOD_S * 1000UL;
}

void loop() {
  const uint32_t now = millis();
  pollRadio();

  if (sender.poll(now) == ReliableSender::Event::Failed) {
    Serial.printf("[node] no ACK received for SEQ %u\n", sender.lastFailedSeq());
  }

  // Read the device once per second; send immediately if valve or alarms changed
  Telemetry current;
  bool haveSample = false;
  if (static_cast<int32_t>(now - nextSampleMs) >= 0) {
    nextSampleMs = now + 1000;
    haveSample = device.read(current);
    if (haveSample && (current.valve != lastValve || current.alarms != lastAlarms)) {
      telemetryNow = true;
    }
  }

  if (sender.busy()) return;  // stop-and-wait: waiting for the previous message's ACK

  if (resultPending) {
    uint8_t payload[MAX_PAYLOAD];
    const size_t n = encodeCmdResult(pendingResult, payload, sizeof(payload));
    if (sendPayload(MsgType::CmdResult, payload, n)) resultPending = false;
    return;
  }

  if (telemetryNow || static_cast<int32_t>(now - nextTelemetryMs) >= 0) {
    if (!haveSample) haveSample = device.read(current);
    if (haveSample) {
      sendTelemetry(current);
      lastValve = current.valve;
      lastAlarms = current.alarms;
    }
    telemetryNow = false;
    nextTelemetryMs = now + jitteredPeriodMs();
    return;
  }

  if (static_cast<int32_t>(now - nextHeartbeatMs) >= 0) {
    sendHeartbeat();
    nextHeartbeatMs = now + HEARTBEAT_PERIOD_S * 1000UL;
  }
}
