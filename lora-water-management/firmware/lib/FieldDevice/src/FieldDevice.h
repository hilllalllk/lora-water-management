// Saha cihazı adaptör arayüzü.
//
// Uç düğüm yazılımı sahadaki cihazla (vana, depo, terfi merkezi kontrolcüsü)
// sadece bu arayüz üzerinden konuşur. Cihaz değiştiğinde yalnızca yeni bir
// adaptör yazılır; LoRa protokolü, gateway ve bulut aynı kalır.
#pragma once

#include <LoraProtocol.h>

class FieldDevice {
 public:
  virtual ~FieldDevice() {}

  virtual bool begin() = 0;

  // Güncel ölçümleri okur.
  virtual bool read(lwm::Telemetry& out) = 0;

  // Bir komutu uygular ve sonuç kodunu döner.
  virtual lwm::Result execute(lwm::Opcode op, const uint8_t* args, uint8_t argLen) = 0;

  // Cihaz devre dışıysa komutlar reddedilir (CIHAZ_ETKIN komutu ile değişir).
  bool enabled() const { return enabled_; }

 protected:
  bool enabled_ = true;
};
