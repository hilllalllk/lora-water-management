// Thin driver for the Ebyte E32 (E32-433T20D, 100 mW; 3.3-5.2 V supply, ~130 mA while transmitting).
//
// Uses the module in fixed transmission mode: the first 3 bytes of every
// packet sent are the target ADDH, ADDL, CHAN; the receiving module strips
// these 3 bytes and only outputs the data on its UART.
//
// Reference: Ebyte E32 user manual (linked in docs/04-donanim.md).
#pragma once

#include <Arduino.h>

struct E32Pins {
  int8_t rx;   // ESP32 RX  <- E32 TXD
  int8_t tx;   // ESP32 TX  -> E32 RXD
  int8_t m0;
  int8_t m1;
  int8_t aux;
};

// Power level in the OPTION byte (bits 1-0). On the E32-433T20D:
//   Max = 0 -> 20 dBm (100 mW), High = 1 -> 17 dBm, Medium = 2 -> 14 dBm,
//   Min = 3 -> 10 dBm (~10 mW, the Turkish limit for 433 MHz short-range devices).
// Other E32 variants (e.g. T30D) map the same codes to different powers.
enum class E32Power : uint8_t { Max = 0, High = 1, Medium = 2, Min = 3 };

// Air data rate in the SPED byte (bits 2-0).
enum class E32AirRate : uint8_t {
  R0_3k = 0, R1_2k = 1, R2_4k = 2, R4_8k = 3, R9_6k = 4, R19_2k = 5,
};

struct E32Config {
  uint16_t address = 0x0001;
  uint8_t channel = 0x17;            // 410 + 23 = 433 MHz
  E32AirRate airRate = E32AirRate::R2_4k;
  E32Power power = E32Power::Min;    // lowest power to stay within legal limits (docs/04-donanim.md)
  bool fec = true;
};

class E32Radio {
 public:
  E32Radio(HardwareSerial& serial, const E32Pins& pins) : serial_(serial), pins_(pins) {}

  // Sets up the pins, configures the module (mode 3) and switches it to normal mode (mode 0).
  bool begin(const E32Config& cfg);

  // Fixed-address send. len must be <= 55 (fits in 58 bytes with the 3-byte header).
  bool send(uint16_t dst, const uint8_t* data, size_t len);

  // Access to bytes received on the UART (fed into the FrameAssembler).
  int available() { return serial_.available(); }
  int read() { return serial_.read(); }

  // Reads the module configuration (mode 3). For debugging.
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
