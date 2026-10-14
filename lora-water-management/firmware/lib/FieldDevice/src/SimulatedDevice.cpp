#include "SimulatedDevice.h"

using lwm::Opcode;
using lwm::Result;

bool SimulatedDevice::begin() {
  pinMode(ledPin_, OUTPUT);
  digitalWrite(ledPin_, LOW);
  analogReadResolution(12);
  return true;
}

bool SimulatedDevice::read(lwm::Telemetry& t) {
  const uint32_t raw = analogRead(adcPin_);  // 0..4095
  const uint16_t level = static_cast<uint16_t>((raw * maxLevelMm_) / 4095);

  t = lwm::Telemetry();
  t.hasLevel = true;
  t.levelMm = level;
  t.hasValve = true;
  t.valve = static_cast<uint8_t>(valveOpen_ ? lwm::ValveState::Open : lwm::ValveState::Closed);
  t.hasAlarms = true;
  t.alarms = level >= overflowMm_ ? lwm::alarm::Overflow : 0;
  return true;
}

Result SimulatedDevice::execute(Opcode op, const uint8_t* args, uint8_t argLen) {
  if (op == Opcode::DeviceEnable) {
    if (argLen != 1) return Result::UnknownCommand;
    enabled_ = args[0] != 0;
    return Result::Ok;
  }
  if (!enabled_) return Result::DeviceError;

  switch (op) {
    case Opcode::ValveOpen:
      if (valveOpen_) return Result::AlreadyInState;
      valveOpen_ = true;
      digitalWrite(ledPin_, HIGH);
      return Result::Ok;
    case Opcode::ValveClose:
      if (!valveOpen_) return Result::AlreadyInState;
      valveOpen_ = false;
      digitalWrite(ledPin_, LOW);
      return Result::Ok;
    default:
      return Result::UnknownCommand;
  }
}
