// Minimal Modbus RTU master: frame building and response parsing.
//
// Pure functions only, no Arduino headers and no I/O, so the same code runs on
// the ESP32 and in the native unit tests (pio test -e native). The serial /
// RS-485 transport lives in the FieldDevice library (ModbusRtuDevice).
//
// RTU frame: [slave][function][data...][CRC lo][CRC hi]
// Supported functions:
//   0x03 Read Holding Registers   request: addr(2) count(2)   response: byteCount(1) regs(2*n)
//   0x06 Write Single Register    request: addr(2) value(2)   response: echo of the request
// Exception response: [slave][function | 0x80][exception code][CRC]
//
// Reference: "MODBUS over Serial Line Specification V1.02" and
// "MODBUS Application Protocol Specification V1.1b3" (modbus.org).
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace modbus {

constexpr uint8_t FC_READ_HOLDING = 0x03;
constexpr uint8_t FC_WRITE_SINGLE = 0x06;
constexpr uint8_t EXCEPTION_FLAG = 0x80;

constexpr uint16_t MAX_READ_COUNT = 125;                 // spec limit for function 0x03
constexpr size_t REQUEST_LENGTH = 8;                     // both 0x03 and 0x06 requests
constexpr size_t EXCEPTION_LENGTH = 5;
constexpr size_t MAX_ADU = 256;                          // RTU frame upper bound

// Exception codes (Application Protocol §7)
enum class ExceptionCode : uint8_t {
  None = 0x00,
  IllegalFunction = 0x01,
  IllegalDataAddress = 0x02,
  IllegalDataValue = 0x03,
  SlaveDeviceFailure = 0x04,
  Acknowledge = 0x05,
  SlaveDeviceBusy = 0x06,
  GatewayPathUnavailable = 0x0A,
  GatewayTargetFailed = 0x0B,
};

enum class Status : uint8_t {
  Ok = 0,
  TooShort,       // fewer bytes than the smallest valid response
  BadCrc,
  WrongSlave,     // reply from another slave address
  WrongFunction,  // function code does not match the request
  BadLength,      // byte count / frame length inconsistent with the request
  Mismatch,       // write echo differs from what was written
  Exception,      // slave answered with an exception response (see exceptionCode)
  Timeout,        // no (complete) response; set by the transport, never by the parsers
};

const char* statusName(Status s);

// CRC-16/MODBUS (reflected poly 0xA001, init 0xFFFF). "123456789" -> 0x4B37.
// In the frame the CRC is sent low byte first.
uint16_t crc16(const uint8_t* data, size_t len, uint16_t crc = 0xFFFF);

// True if the last two bytes of buf are the correct little-endian CRC of the rest.
bool checkCrc(const uint8_t* buf, size_t len);

// Request builders. Return the number of bytes written (8), or 0 on bad
// arguments (slave 0 / broadcast is not allowed for reads, count 1..125,
// capacity too small).
size_t buildReadHolding(uint8_t slave, uint16_t addr, uint16_t count, uint8_t* out,
                        size_t capacity);
size_t buildWriteSingle(uint8_t slave, uint16_t addr, uint16_t value, uint8_t* out,
                        size_t capacity);

// Expected length of a normal (non-exception) response to the given request.
size_t expectedReadHoldingLength(uint16_t count);  // 5 + 2*count
constexpr size_t expectedWriteSingleLength() { return 8; }

// Transport helper: is the receive buffer a complete response yet? True once
// either the expected normal length or a full exception frame has arrived.
bool responseComplete(const uint8_t* buf, size_t len, size_t expectedLen);

// Response parsers. They validate CRC, slave address, function code and
// lengths. On Status::Exception, *exception (if not null) receives the code.
Status parseReadHolding(const uint8_t* in, size_t len, uint8_t slave, uint16_t count,
                        uint16_t* regsOut, ExceptionCode* exception = nullptr);
Status parseWriteSingle(const uint8_t* in, size_t len, uint8_t slave, uint16_t addr,
                        uint16_t value, ExceptionCode* exception = nullptr);

// Silent interval between frames (t3.5) in microseconds for the given baud
// rate with 11-bit characters; fixed at 1750 us above 19200 baud (spec §2.5.1.1).
uint32_t interFrameDelayUs(uint32_t baud);

}  // namespace modbus
