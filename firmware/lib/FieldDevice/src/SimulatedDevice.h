// Prototype field device: an LED and a potentiometer on the ESP32 stand in
// for a real valve/tank (docs/02-sistem-mimarisi.md, phase A3).
//
//   LED on                    -> valve open
//   Potentiometer             -> tank level (0..maxLevelMm)
//   Level above overflow mark -> alarm::Overflow
#pragma once

#include <Arduino.h>

#include "FieldDevice.h"

class SimulatedDevice : public FieldDevice {
 public:
  SimulatedDevice(int valveLedPin, int levelAdcPin, uint16_t maxLevelMm = 3000,
                  uint16_t overflowMm = 2800)
      : ledPin_(valveLedPin), adcPin_(levelAdcPin), maxLevelMm_(maxLevelMm),
        overflowMm_(overflowMm) {}

  bool begin() override;
  bool read(lwm::Telemetry& out) override;
  lwm::Result execute(lwm::Opcode op, const uint8_t* args, uint8_t argLen) override;

 private:
  int ledPin_;
  int adcPin_;
  uint16_t maxLevelMm_;
  uint16_t overflowMm_;
  bool valveOpen_ = false;
};
