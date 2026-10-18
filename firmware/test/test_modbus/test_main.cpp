// Modbus RTU unit tests. Run on a PC with: pio test -e native
//
// Expected CRCs were cross-checked with an independent Python implementation
// of CRC-16/MODBUS; 01 03 00 00 00 05 85 C9 and 01 83 02 C0 F1 are also
// common reference frames.
#include <unity.h>
#include <string.h>

#include "ModbusRtu.h"

using namespace modbus;

void setUp() {}
void tearDown() {}

static void test_crc16_check_value() {
  const uint8_t data[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
  TEST_ASSERT_EQUAL_HEX16(0x4B37, crc16(data, sizeof(data)));
}

static void test_build_read_holding() {
  uint8_t buf[REQUEST_LENGTH];
  TEST_ASSERT_EQUAL(8, buildReadHolding(1, 0x0000, 5, buf, sizeof(buf)));
  const uint8_t expected[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x05, 0x85, 0xC9};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, buf, sizeof(expected));
  TEST_ASSERT_TRUE(checkCrc(buf, sizeof(buf)));
}

static void test_build_read_rejects_bad_args() {
  uint8_t buf[REQUEST_LENGTH];
  TEST_ASSERT_EQUAL(0, buildReadHolding(0, 0, 1, buf, sizeof(buf)));    // broadcast read
  TEST_ASSERT_EQUAL(0, buildReadHolding(1, 0, 0, buf, sizeof(buf)));    // count 0
  TEST_ASSERT_EQUAL(0, buildReadHolding(1, 0, 126, buf, sizeof(buf)));  // count > 125
  TEST_ASSERT_EQUAL(0, buildReadHolding(1, 0, 1, buf, 7));              // buffer too small
}

static void test_build_write_single() {
  uint8_t buf[REQUEST_LENGTH];
  TEST_ASSERT_EQUAL(8, buildWriteSingle(1, 0x000A, 0x0001, buf, sizeof(buf)));
  const uint8_t open[] = {0x01, 0x06, 0x00, 0x0A, 0x00, 0x01, 0x68, 0x08};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(open, buf, sizeof(open));

  TEST_ASSERT_EQUAL(8, buildWriteSingle(1, 0x000A, 0x0000, buf, sizeof(buf)));
  const uint8_t close[] = {0x01, 0x06, 0x00, 0x0A, 0x00, 0x00, 0xA9, 0xC8};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(close, buf, sizeof(close));
}

static void test_parse_read_holding_ok() {
  // level 1000 mm, valve open, 100 cbar, 4.2 l/min, no alarms
  const uint8_t resp[] = {0x01, 0x03, 0x0A, 0x03, 0xE8, 0x00, 0x01, 0x00, 0x64,
                          0x00, 0x2A, 0x00, 0x00, 0x3A, 0x7D};
  TEST_ASSERT_EQUAL(sizeof(resp), expectedReadHoldingLength(5));
  uint16_t regs[5] = {};
  ExceptionCode ex = ExceptionCode::SlaveDeviceBusy;
  TEST_ASSERT_EQUAL(Status::Ok, parseReadHolding(resp, sizeof(resp), 1, 5, regs, &ex));
  TEST_ASSERT_EQUAL(ExceptionCode::None, ex);
  TEST_ASSERT_EQUAL_UINT16(1000, regs[0]);
  TEST_ASSERT_EQUAL_UINT16(1, regs[1]);
  TEST_ASSERT_EQUAL_UINT16(100, regs[2]);
  TEST_ASSERT_EQUAL_UINT16(42, regs[3]);
  TEST_ASSERT_EQUAL_UINT16(0, regs[4]);
}

static void test_parse_read_holding_errors() {
  uint8_t resp[] = {0x01, 0x03, 0x0A, 0x03, 0xE8, 0x00, 0x01, 0x00, 0x64,
                    0x00, 0x2A, 0x00, 0x00, 0x3A, 0x7D};
  uint16_t regs[5];
  // asked for a different count than the slave returned
  TEST_ASSERT_EQUAL(Status::BadLength, parseReadHolding(resp, sizeof(resp), 1, 4, regs));
  // answer from another slave
  TEST_ASSERT_EQUAL(Status::WrongSlave, parseReadHolding(resp, sizeof(resp), 2, 5, regs));
  // truncated
  TEST_ASSERT_EQUAL(Status::TooShort, parseReadHolding(resp, 4, 1, 5, regs));
  // single bit flip -> bad CRC
  resp[4] ^= 0x01;
  TEST_ASSERT_EQUAL(Status::BadCrc, parseReadHolding(resp, sizeof(resp), 1, 5, regs));
}

static void test_parse_write_single() {
  const uint8_t echo[] = {0x01, 0x06, 0x00, 0x0A, 0x00, 0x01, 0x68, 0x08};
  TEST_ASSERT_EQUAL(Status::Ok, parseWriteSingle(echo, sizeof(echo), 1, 0x000A, 1));
  // valid frame, but not the value we wrote
  TEST_ASSERT_EQUAL(Status::Mismatch, parseWriteSingle(echo, sizeof(echo), 1, 0x000A, 0));
  // a read response is not a write response
  TEST_ASSERT_EQUAL(Status::WrongFunction, parseReadHolding(echo, sizeof(echo), 1, 1, nullptr));
}

static void test_bad_crc() {
  uint8_t echo[] = {0x01, 0x06, 0x00, 0x0A, 0x00, 0x01, 0x68, 0x09};
  TEST_ASSERT_FALSE(checkCrc(echo, sizeof(echo)));
  TEST_ASSERT_EQUAL(Status::BadCrc, parseWriteSingle(echo, sizeof(echo), 1, 0x000A, 1));
}

static void test_exception_response() {
  // slave 1, function 0x03 | 0x80, exception 02 (illegal data address)
  const uint8_t resp[] = {0x01, 0x83, 0x02, 0xC0, 0xF1};
  TEST_ASSERT_TRUE(responseComplete(resp, sizeof(resp), expectedReadHoldingLength(5)));
  uint16_t regs[5];
  ExceptionCode ex = ExceptionCode::None;
  TEST_ASSERT_EQUAL(Status::Exception, parseReadHolding(resp, sizeof(resp), 1, 5, regs, &ex));
  TEST_ASSERT_EQUAL(ExceptionCode::IllegalDataAddress, ex);
  // the same exception does not belong to a 0x06 request
  TEST_ASSERT_EQUAL(Status::WrongFunction, parseWriteSingle(resp, sizeof(resp), 1, 0x000A, 1));
}

static void test_response_complete() {
  const uint8_t partial[] = {0x01, 0x03, 0x0A, 0x03, 0xE8};
  TEST_ASSERT_FALSE(responseComplete(partial, sizeof(partial), 15));
  TEST_ASSERT_TRUE(responseComplete(partial, sizeof(partial), 5));
}

static void test_inter_frame_delay() {
  TEST_ASSERT_EQUAL_UINT32(4011, interFrameDelayUs(9600));  // 3.5 * 11 bits / 9600 baud
  TEST_ASSERT_EQUAL_UINT32(1750, interFrameDelayUs(115200));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_crc16_check_value);
  RUN_TEST(test_build_read_holding);
  RUN_TEST(test_build_read_rejects_bad_args);
  RUN_TEST(test_build_write_single);
  RUN_TEST(test_parse_read_holding_ok);
  RUN_TEST(test_parse_read_holding_errors);
  RUN_TEST(test_parse_write_single);
  RUN_TEST(test_bad_crc);
  RUN_TEST(test_exception_response);
  RUN_TEST(test_response_complete);
  RUN_TEST(test_inter_frame_delay);
  return UNITY_END();
}
