// Field device adapter for an existing digital controller that speaks
// Modbus RTU over RS-485 (the sites already have such controllers; the
// advisor asked the node to read/write them instead of replacing them).
//
// Hardware: ESP32 UART1 -> MAX3485 (3.3 V RS-485 transceiver) -> A/B bus.
// DE and /RE are tied together on one GPIO and driven manually around each
// request. Framing (CRC, request/response validation) comes from the
// hardware-independent ModbusRtu library; this class only does the transport
// and the mapping between registers and the LoRa protocol.
//
// The register map is passed in (see include/modbus_map.h), so a different
// controller only needs a different map, not a different class.
#pragma once

#include <Arduino.h>
#include <ModbusRtu.h>

#include "FieldDevice.h"

struct Rs485Pins {
  int8_t rx;  // <- RO
  int8_t tx;  // -> DI
  int8_t de;  // -> DE + /RE
};

struct ModbusRegisterMap {
  uint8_t slave = 1;
  // Telemetry registers (function 0x03); -1 = not provided by the device
  int32_t levelMm = -1;
  int32_t valveState = -1;
  int32_t pressureCbar = -1;
  int32_t flowDlMin = -1;
  int32_t alarms = -1;
  // Command registers (function 0x06); -1 = not supported
  int32_t valveCommand = -1;  // 1 = open, 0 = close
  int32_t enable = -1;        // 1 = enabled, 0 = disabled
};

class ModbusRtuDevice : public FieldDevice {
 public:
  static constexpr uint32_t RESPONSE_TIMEOUT_MS = 200;

  ModbusRtuDevice(HardwareSerial& serial, const Rs485Pins& pins, uint32_t baud,
                  const ModbusRegisterMap& map)
      : serial_(serial), pins_(pins), baud_(baud), map_(map) {}

  bool begin() override;

  // On a bus error the telemetry carries only alarm::SensorFault (and true is
  // returned) so the cloud sees that the controller is unreachable.
  bool read(lwm::Telemetry& out) override;

  lwm::Result execute(lwm::Opcode op, const uint8_t* args, uint8_t argLen) override;

  modbus::Status lastStatus() const { return lastStatus_; }
  modbus::ExceptionCode lastException() const { return lastException_; }

 private:
  modbus::Status readRegisters(uint16_t addr, uint16_t count, uint16_t* out);
  modbus::Status writeRegister(uint16_t addr, uint16_t value);
  // Sends a request and collects the response until it is complete or times out.
  size_t transact(const uint8_t* req, size_t reqLen, uint8_t* resp, size_t expectedLen);
  bool readValve(uint16_t& state);
  void logError(const char* what);

  HardwareSerial& serial_;
  Rs485Pins pins_;
  uint32_t baud_;
  ModbusRegisterMap map_;
  uint32_t frameGapUs_ = 1750;
  uint32_t lastActivityUs_ = 0;
  modbus::Status lastStatus_ = modbus::Status::Ok;
  modbus::ExceptionCode lastException_ = modbus::ExceptionCode::None;
};
