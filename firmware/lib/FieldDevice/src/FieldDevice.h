// Field device adapter interface.
//
// The end-node firmware talks to the field device (valve, tank, pumping station
// controller) only through this interface. When the device changes, only a new
// adapter is written; the LoRa protocol, gateway and cloud stay the same.
#pragma once

#include <LoraProtocol.h>

class FieldDevice {
 public:
  virtual ~FieldDevice() {}

  virtual bool begin() = 0;

  // Reads the current measurements.
  virtual bool read(lwm::Telemetry& out) = 0;

  // Executes a command and returns the result code.
  virtual lwm::Result execute(lwm::Opcode op, const uint8_t* args, uint8_t argLen) = 0;

  // Commands are rejected while the device is disabled (toggled by the CIHAZ_ETKIN command).
  bool enabled() const { return enabled_; }

 protected:
  bool enabled_ = true;
};
