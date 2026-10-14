// Gerçek saha cihazı adaptörü (TASLAK - aşama A8).
//
// Danışmanla belirlenecek gerçek kontrol cihazı RS485 / Modbus RTU
// konuşuyorsa bu adaptör doldurulacak. Register haritası cihazın
// dokümanından alınıp aşağıdaki sabitlere yazılacak.
//
// Önerilen donanım: MAX485 / MAX3485 dönüştürücü, ESP32 UART1.
// Önerilen kütüphane: ModbusMaster (4-20mA veya röle tabanlı cihazlar için
// ayrı adaptörler yazılabilir).
#pragma once

#include "FieldDevice.h"

class ModbusRtuDevice : public FieldDevice {
 public:
  // TODO(A8): register adresleri gerçek cihazın dokümanından doldurulacak.
  static constexpr uint16_t REG_LEVEL = 0x0000;
  static constexpr uint16_t REG_VALVE = 0x0001;
  static constexpr uint16_t COIL_VALVE = 0x0000;

  bool begin() override { return false; }
  bool read(lwm::Telemetry&) override { return false; }
  lwm::Result execute(lwm::Opcode, const uint8_t*, uint8_t) override {
    return lwm::Result::UnknownCommand;
  }
};
