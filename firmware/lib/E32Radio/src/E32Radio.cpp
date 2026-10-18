#include "E32Radio.h"

namespace {
constexpr uint32_t UART_BAUD = 9600;     // kept the same for configuration and normal mode
constexpr uint8_t SPED_UART_9600_8N1 = 0b00011000;  // bit7-6: 8N1, bit5-3: 9600
}  // namespace

bool E32Radio::begin(const E32Config& cfg) {
  cfg_ = cfg;
  pinMode(pins_.m0, OUTPUT);
  pinMode(pins_.m1, OUTPUT);
  pinMode(pins_.aux, INPUT);
  serial_.begin(UART_BAUD, SERIAL_8N1, pins_.rx, pins_.tx);

  setMode(Mode::Sleep);  // mode 3: configuration
  const uint8_t sped = SPED_UART_9600_8N1 | static_cast<uint8_t>(cfg.airRate);
  const uint8_t option = 0x80                        // bit7: fixed transmission
                         | 0x40                      // bit6: IO push-pull
                         | (cfg.fec ? 0x04 : 0x00)   // bit2: FEC
                         | static_cast<uint8_t>(cfg.power);
  const uint8_t cmd[6] = {
      0xC0,  // save settings (persist across power loss)
      static_cast<uint8_t>(cfg.address >> 8),
      static_cast<uint8_t>(cfg.address & 0xFF),
      sped,
      static_cast<uint8_t>(cfg.channel & 0x1F),
      option,
  };
  while (serial_.available()) serial_.read();
  serial_.write(cmd, sizeof(cmd));
  serial_.flush();

  // The module echoes the settings back (6 bytes). Verify them.
  uint8_t echo[6] = {};
  const uint32_t start = millis();
  size_t got = 0;
  while (got < sizeof(echo) && millis() - start < 1000) {
    if (serial_.available()) echo[got++] = serial_.read();
  }
  const bool ok = got == sizeof(echo) && memcmp(echo + 1, cmd + 1, 5) == 0;

  setMode(Mode::Normal);
  return ok;
}

bool E32Radio::send(uint16_t dst, const uint8_t* data, size_t len) {
  if (len + 3 > 58) return false;
  if (!waitAux(1000)) return false;  // module busy
  const uint8_t header[3] = {
      static_cast<uint8_t>(dst >> 8),
      static_cast<uint8_t>(dst & 0xFF),
      static_cast<uint8_t>(cfg_.channel & 0x1F),
  };
  serial_.write(header, sizeof(header));
  serial_.write(data, len);
  serial_.flush();
  return true;
}

bool E32Radio::readConfig(uint8_t out[6]) {
  setMode(Mode::Sleep);
  while (serial_.available()) serial_.read();
  const uint8_t cmd[3] = {0xC1, 0xC1, 0xC1};
  serial_.write(cmd, sizeof(cmd));
  serial_.flush();
  const uint32_t start = millis();
  size_t got = 0;
  while (got < 6 && millis() - start < 1000) {
    if (serial_.available()) out[got++] = serial_.read();
  }
  setMode(Mode::Normal);
  return got == 6 && out[0] == 0xC0;
}

void E32Radio::setMode(Mode m) {
  const uint8_t v = static_cast<uint8_t>(m);
  digitalWrite(pins_.m0, (v & 0x01) ? HIGH : LOW);
  digitalWrite(pins_.m1, (v & 0x02) ? HIGH : LOW);
  delay(10);       // manual: wait for AUX to go high after a mode change
  waitAux(1000);
  delay(2);
}

bool E32Radio::waitAux(uint32_t timeoutMs) {
  const uint32_t start = millis();
  while (digitalRead(pins_.aux) == LOW) {
    if (millis() - start > timeoutMs) return false;
    delay(1);
  }
  return true;
}
