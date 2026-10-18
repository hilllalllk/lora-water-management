#include "ModbusRtu.h"

namespace modbus {

namespace {

void putU16BE(uint8_t* p, uint16_t v) {
  p[0] = static_cast<uint8_t>(v >> 8);
  p[1] = static_cast<uint8_t>(v & 0xFF);
}

uint16_t getU16BE(const uint8_t* p) { return static_cast<uint16_t>((p[0] << 8) | p[1]); }

void appendCrc(uint8_t* frame, size_t lenWithoutCrc) {
  const uint16_t crc = crc16(frame, lenWithoutCrc);
  frame[lenWithoutCrc] = static_cast<uint8_t>(crc & 0xFF);  // low byte first
  frame[lenWithoutCrc + 1] = static_cast<uint8_t>(crc >> 8);
}

size_t buildRequest(uint8_t slave, uint8_t function, uint16_t a, uint16_t b, uint8_t* out,
                    size_t capacity) {
  if (!out || capacity < REQUEST_LENGTH) return 0;
  out[0] = slave;
  out[1] = function;
  putU16BE(out + 2, a);
  putU16BE(out + 4, b);
  appendCrc(out, 6);
  return REQUEST_LENGTH;
}

// Common checks for every response: CRC, slave, function / exception.
Status checkHeader(const uint8_t* in, size_t len, uint8_t slave, uint8_t function,
                   ExceptionCode* exception) {
  if (exception) *exception = ExceptionCode::None;
  if (!in || len < EXCEPTION_LENGTH) return Status::TooShort;
  if (!checkCrc(in, len)) return Status::BadCrc;
  if (in[0] != slave) return Status::WrongSlave;
  if (in[1] == static_cast<uint8_t>(function | EXCEPTION_FLAG)) {
    if (len != EXCEPTION_LENGTH) return Status::BadLength;
    if (exception) *exception = static_cast<ExceptionCode>(in[2]);
    return Status::Exception;
  }
  if (in[1] != function) return Status::WrongFunction;
  return Status::Ok;
}

}  // namespace

const char* statusName(Status s) {
  switch (s) {
    case Status::Ok: return "ok";
    case Status::TooShort: return "too short";
    case Status::BadCrc: return "bad CRC";
    case Status::WrongSlave: return "wrong slave";
    case Status::WrongFunction: return "wrong function";
    case Status::BadLength: return "bad length";
    case Status::Mismatch: return "echo mismatch";
    case Status::Exception: return "exception";
    case Status::Timeout: return "timeout";
  }
  return "?";
}

uint16_t crc16(const uint8_t* data, size_t len, uint16_t crc) {
  for (size_t i = 0; i < len; ++i) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 1) ? static_cast<uint16_t>((crc >> 1) ^ 0xA001) : static_cast<uint16_t>(crc >> 1);
    }
  }
  return crc;
}

bool checkCrc(const uint8_t* buf, size_t len) {
  if (!buf || len < 3) return false;
  const uint16_t expected = crc16(buf, len - 2);
  const uint16_t got = static_cast<uint16_t>(buf[len - 2] | (buf[len - 1] << 8));
  return expected == got;
}

size_t buildReadHolding(uint8_t slave, uint16_t addr, uint16_t count, uint8_t* out,
                        size_t capacity) {
  if (slave == 0 || count == 0 || count > MAX_READ_COUNT) return 0;
  return buildRequest(slave, FC_READ_HOLDING, addr, count, out, capacity);
}

size_t buildWriteSingle(uint8_t slave, uint16_t addr, uint16_t value, uint8_t* out,
                        size_t capacity) {
  // slave 0 (broadcast) is valid for writes, but there will be no response.
  return buildRequest(slave, FC_WRITE_SINGLE, addr, value, out, capacity);
}

size_t expectedReadHoldingLength(uint16_t count) { return 5 + 2 * static_cast<size_t>(count); }

bool responseComplete(const uint8_t* buf, size_t len, size_t expectedLen) {
  if (len >= expectedLen) return true;
  return len >= EXCEPTION_LENGTH && (buf[1] & EXCEPTION_FLAG) != 0;
}

Status parseReadHolding(const uint8_t* in, size_t len, uint8_t slave, uint16_t count,
                        uint16_t* regsOut, ExceptionCode* exception) {
  const Status s = checkHeader(in, len, slave, FC_READ_HOLDING, exception);
  if (s != Status::Ok) return s;
  if (len != expectedReadHoldingLength(count) || in[2] != 2 * count) return Status::BadLength;
  for (uint16_t i = 0; i < count; ++i) {
    if (regsOut) regsOut[i] = getU16BE(in + 3 + 2 * i);
  }
  return Status::Ok;
}

Status parseWriteSingle(const uint8_t* in, size_t len, uint8_t slave, uint16_t addr,
                        uint16_t value, ExceptionCode* exception) {
  const Status s = checkHeader(in, len, slave, FC_WRITE_SINGLE, exception);
  if (s != Status::Ok) return s;
  if (len != expectedWriteSingleLength()) return Status::BadLength;
  if (getU16BE(in + 2) != addr || getU16BE(in + 4) != value) return Status::Mismatch;
  return Status::Ok;
}

uint32_t interFrameDelayUs(uint32_t baud) {
  if (baud == 0 || baud > 19200) return 1750;
  // 3.5 characters * 11 bits per character
  return static_cast<uint32_t>((38500000ULL + baud - 1) / baud);
}

}  // namespace modbus
