// Ebyte E32 (E32-433T30D) için ince sürücü.
//
// Modülü sabit adresli (fixed transmission) modda kullanır: gönderilen her
// paketin ilk 3 baytı hedef ADDH, ADDL, CHAN'dır; alıcı modül bu 3 baytı
// UART'a vermez, sadece veriyi verir.
//
// Kaynak: Ebyte E32 kullanım kılavuzu (docs/04-donanim.md içinde bağlantı).
#pragma once

#include <Arduino.h>

struct E32Pins {
  int8_t rx;   // ESP32 RX  <- E32 TXD
  int8_t tx;   // ESP32 TX  -> E32 RXD
  int8_t m0;
  int8_t m1;
  int8_t aux;
};

// OPTION baytındaki güç seviyesi (bit1-0). 30D modelinde 0 en yüksek, 3 en düşük güçtür.
enum class E32Power : uint8_t { Max = 0, High = 1, Medium = 2, Min = 3 };

// SPED baytındaki hava hızı (bit2-0).
enum class E32AirRate : uint8_t {
  R0_3k = 0, R1_2k = 1, R2_4k = 2, R4_8k = 3, R9_6k = 4, R19_2k = 5,
};

struct E32Config {
  uint16_t address = 0x0001;
  uint8_t channel = 0x17;            // 410 + 23 = 433 MHz
  E32AirRate airRate = E32AirRate::R2_4k;
  E32Power power = E32Power::Min;    // yasal sınırlar için en düşük güç (docs/04-donanim.md)
  bool fec = true;
};

class E32Radio {
 public:
  E32Radio(HardwareSerial& serial, const E32Pins& pins) : serial_(serial), pins_(pins) {}

  // Pinleri ayarlar, modülü yapılandırır (mod 3) ve normal moda (mod 0) alır.
  bool begin(const E32Config& cfg);

  // Sabit adresli gönderim. len <= 55 olmalı (3 baytlık başlıkla 58'e sığar).
  bool send(uint16_t dst, const uint8_t* data, size_t len);

  // UART'tan gelen baytlara erişim (FrameAssembler'a beslenir).
  int available() { return serial_.available(); }
  int read() { return serial_.read(); }

  // Modülün yapılandırmasını okur (mod 3). Hata ayıklama için.
  bool readConfig(uint8_t out[6]);

  const E32Config& config() const { return cfg_; }

 private:
  enum class Mode : uint8_t { Normal = 0, WakeUp = 1, PowerSaving = 2, Sleep = 3 };
  void setMode(Mode m);
  bool waitAux(uint32_t timeoutMs);

  HardwareSerial& serial_;
  E32Pins pins_;
  E32Config cfg_;
};
