// Prototip saha cihazı: gerçek vana/depo yerine ESP32 üzerindeki LED ve
// potansiyometre kullanılır (docs/02-sistem-mimarisi.md, aşama A3).
//
//   LED yanık   -> vana açık
//   Potansiyometre -> depo seviyesi (0..maxLevelMm)
//   Seviye taşma eşiğini geçerse -> alarm::Overflow
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
