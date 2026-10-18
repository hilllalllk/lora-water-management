#include "ModbusRtuDevice.h"

using lwm::Opcode;
using lwm::Result;
using modbus::Status;

namespace {
constexpr uint16_t MAX_BLOCK = 16;  // read telemetry in one request if it spans <= 16 registers
}  // namespace

bool ModbusRtuDevice::begin() {
  pinMode(pins_.de, OUTPUT);
  digitalWrite(pins_.de, LOW);  // receive by default
  serial_.begin(baud_, SERIAL_8N1, pins_.rx, pins_.tx);
  frameGapUs_ = modbus::interFrameDelayUs(baud_);
  lastActivityUs_ = micros();

  // Probe the controller once so wiring problems show up in the boot log.
  lwm::Telemetry t;
  read(t);
  if (lastStatus_ != Status::Ok) {
    Serial.printf("[modbus] slave %u not responding at boot (%s)\n", map_.slave,
                  modbus::statusName(lastStatus_));
    return false;
  }
  Serial.printf("[modbus] slave %u online\n", map_.slave);
  return true;
}

size_t ModbusRtuDevice::transact(const uint8_t* req, size_t reqLen, uint8_t* resp,
                                 size_t expectedLen) {
  // Keep the bus silent for at least t3.5 since the last frame.
  while (static_cast<uint32_t>(micros() - lastActivityUs_) < frameGapUs_) {
  }
  while (serial_.available()) serial_.read();  // drop stale bytes / echoes

  digitalWrite(pins_.de, HIGH);
  serial_.write(req, reqLen);
  serial_.flush();  // returns once the last stop bit has left the UART
  digitalWrite(pins_.de, LOW);

  size_t n = 0;
  uint32_t start = millis();
  while (millis() - start < RESPONSE_TIMEOUT_MS) {
    while (serial_.available() && n < modbus::MAX_ADU) {
      resp[n++] = static_cast<uint8_t>(serial_.read());
    }
    if (n > 0 && modbus::responseComplete(resp, n, expectedLen)) break;
    delay(1);
  }
  lastActivityUs_ = micros();
  return n;
}

Status ModbusRtuDevice::readRegisters(uint16_t addr, uint16_t count, uint16_t* out) {
  uint8_t req[modbus::REQUEST_LENGTH];
  uint8_t resp[modbus::MAX_ADU];
  const size_t reqLen = modbus::buildReadHolding(map_.slave, addr, count, req, sizeof(req));
  if (reqLen == 0) return lastStatus_ = Status::BadLength;
  const size_t n = transact(req, reqLen, resp, modbus::expectedReadHoldingLength(count));
  if (n == 0) return lastStatus_ = Status::Timeout;
  lastStatus_ = modbus::parseReadHolding(resp, n, map_.slave, count, out, &lastException_);
  return lastStatus_;
}

Status ModbusRtuDevice::writeRegister(uint16_t addr, uint16_t value) {
  uint8_t req[modbus::REQUEST_LENGTH];
  uint8_t resp[modbus::MAX_ADU];
  const size_t reqLen = modbus::buildWriteSingle(map_.slave, addr, value, req, sizeof(req));
  if (reqLen == 0) return lastStatus_ = Status::BadLength;
  const size_t n = transact(req, reqLen, resp, modbus::expectedWriteSingleLength());
  if (n == 0) return lastStatus_ = Status::Timeout;
  lastStatus_ = modbus::parseWriteSingle(resp, n, map_.slave, addr, value, &lastException_);
  return lastStatus_;
}

void ModbusRtuDevice::logError(const char* what) {
  if (lastStatus_ == Status::Exception) {
    Serial.printf("[modbus] %s: exception 0x%02X\n", what, static_cast<unsigned>(lastException_));
  } else {
    Serial.printf("[modbus] %s: %s\n", what, modbus::statusName(lastStatus_));
  }
}

bool ModbusRtuDevice::read(lwm::Telemetry& t) {
  t = lwm::Telemetry();

  const int32_t addrs[] = {map_.levelMm, map_.valveState, map_.pressureCbar, map_.flowDlMin,
                           map_.alarms};
  constexpr size_t FIELDS = sizeof(addrs) / sizeof(addrs[0]);
  uint16_t values[FIELDS] = {};
  bool present[FIELDS] = {};

  int32_t lo = 0x10000, hi = -1;
  for (size_t i = 0; i < FIELDS; ++i) {
    if (addrs[i] < 0 || addrs[i] > 0xFFFF) continue;
    present[i] = true;
    if (addrs[i] < lo) lo = addrs[i];
    if (addrs[i] > hi) hi = addrs[i];
  }
  if (hi < 0) return false;  // nothing mapped

  bool ok = true;
  const int32_t span = hi - lo + 1;
  if (span <= MAX_BLOCK) {
    uint16_t block[MAX_BLOCK];
    ok = readRegisters(static_cast<uint16_t>(lo), static_cast<uint16_t>(span), block) == Status::Ok;
    for (size_t i = 0; ok && i < FIELDS; ++i) {
      if (present[i]) values[i] = block[addrs[i] - lo];
    }
  } else {
    for (size_t i = 0; ok && i < FIELDS; ++i) {
      if (present[i]) ok = readRegisters(static_cast<uint16_t>(addrs[i]), 1, &values[i]) == Status::Ok;
    }
  }

  if (!ok) {
    logError("read");
    t.hasAlarms = true;
    t.alarms = lwm::alarm::SensorFault;
    return true;
  }

  if (present[0]) { t.hasLevel = true; t.levelMm = values[0]; }
  if (present[1]) {
    t.hasValve = true;
    t.valve = values[1] <= static_cast<uint16_t>(lwm::ValveState::Fault)
                  ? static_cast<uint8_t>(values[1])
                  : static_cast<uint8_t>(lwm::ValveState::Fault);
  }
  if (present[2]) { t.hasPressure = true; t.pressureCbar = values[2]; }
  if (present[3]) { t.hasFlow = true; t.flowDlMin = values[3]; }
  if (present[4]) { t.hasAlarms = true; t.alarms = static_cast<uint8_t>(values[4] & 0xFF); }
  return true;
}

bool ModbusRtuDevice::readValve(uint16_t& state) {
  if (map_.valveState < 0) return false;
  return readRegisters(static_cast<uint16_t>(map_.valveState), 1, &state) == Status::Ok;
}

Result ModbusRtuDevice::execute(Opcode op, const uint8_t* args, uint8_t argLen) {
  if (op == Opcode::DeviceEnable) {
    if (argLen != 1) return Result::UnknownCommand;
    const bool enable = args[0] != 0;
    if (map_.enable >= 0 &&
        writeRegister(static_cast<uint16_t>(map_.enable), enable ? 1 : 0) != Status::Ok) {
      logError("enable");
      return Result::DeviceError;
    }
    enabled_ = enable;
    return Result::Ok;
  }

  switch (op) {
    case Opcode::ReadNow: {
      // Normally handled by the node itself; here it just checks the bus.
      lwm::Telemetry t;
      read(t);
      return lastStatus_ == Status::Ok ? Result::Ok : Result::DeviceError;
    }
    case Opcode::ValveOpen:
    case Opcode::ValveClose: {
      if (!enabled_) return Result::DeviceError;
      if (map_.valveCommand < 0) return Result::UnknownCommand;
      const uint16_t wanted = op == Opcode::ValveOpen ? 1 : 0;
      uint16_t state = 0;
      if (map_.valveState >= 0) {
        if (!readValve(state)) {
          logError("valve state");
          return Result::DeviceError;
        }
        if (state == wanted) return Result::AlreadyInState;
      }
      if (writeRegister(static_cast<uint16_t>(map_.valveCommand), wanted) != Status::Ok) {
        logError("valve command");
        return Result::DeviceError;
      }
      return Result::Ok;
    }
    default:
      return Result::UnknownCommand;  // SetPeriod is handled by the node, not the device
  }
}
