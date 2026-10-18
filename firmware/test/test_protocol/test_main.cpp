// Protocol unit tests. Run on a PC with: pio test -e native
#include <unity.h>
#include <string.h>

#include "LoraProtocol.h"

using namespace lwm;

void setUp() {}
void tearDown() {}

static void test_crc16_check_value() {
  const uint8_t data[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
  TEST_ASSERT_EQUAL_HEX16(0x29B1, crc16(data, sizeof(data)));
}

static void test_frame_roundtrip() {
  Frame f;
  f.type = MsgType::Telemetry;
  f.src = 0x1001;
  f.seq = 42;
  f.len = 3;
  f.payload[0] = 0xAA; f.payload[1] = 0xBB; f.payload[2] = 0xCC;

  uint8_t buf[MAX_FRAME];
  const size_t n = encodeFrame(f, buf, sizeof(buf));
  TEST_ASSERT_EQUAL(FRAME_OVERHEAD + 3, n);
  TEST_ASSERT_EQUAL_HEX8(0x01, buf[0]);
  TEST_ASSERT_EQUAL_HEX8(0x10, buf[2]);
  TEST_ASSERT_EQUAL_HEX8(0x01, buf[3]);

  Frame d;
  TEST_ASSERT_EQUAL(DecodeError::None, decodeFrame(buf, n, d));
  TEST_ASSERT_EQUAL(MsgType::Telemetry, d.type);
  TEST_ASSERT_EQUAL_HEX16(0x1001, d.src);
  TEST_ASSERT_EQUAL(42, d.seq);
  TEST_ASSERT_EQUAL(3, d.len);
  TEST_ASSERT_EQUAL_MEMORY(f.payload, d.payload, 3);
}

static void test_frame_fits_e32_packet() {
  Frame f;
  f.len = MAX_PAYLOAD;
  uint8_t buf[MAX_FRAME];
  const size_t n = encodeFrame(f, buf, sizeof(buf));
  TEST_ASSERT_EQUAL(MAX_FRAME, n);
  TEST_ASSERT_TRUE(n + E32_FIXED_HEADER <= E32_MAX_PACKET);

  f.len = MAX_PAYLOAD + 1;
  TEST_ASSERT_EQUAL(0, encodeFrame(f, buf, sizeof(buf)));
}

static void test_frame_detects_corruption() {
  Frame f;
  f.len = 2;
  f.payload[0] = 1; f.payload[1] = 2;
  uint8_t buf[MAX_FRAME];
  const size_t n = encodeFrame(f, buf, sizeof(buf));
  Frame d;

  buf[6] ^= 0x01;
  TEST_ASSERT_EQUAL(DecodeError::BadCrc, decodeFrame(buf, n, d));
  buf[6] ^= 0x01;

  TEST_ASSERT_EQUAL(DecodeError::BadLength, decodeFrame(buf, n - 1, d));
  TEST_ASSERT_EQUAL(DecodeError::TooShort, decodeFrame(buf, 4, d));
  buf[0] = 0x02;
  TEST_ASSERT_EQUAL(DecodeError::BadVersion, decodeFrame(buf, n, d));
}

static void test_telemetry_doc_example() {
  // docs/03-protokol.md: level 1250 mm, valve open -> 01 02 04E2 02 01 01
  Telemetry t;
  t.hasLevel = true; t.levelMm = 1250;
  t.hasValve = true; t.valve = 1;
  uint8_t buf[16];
  const size_t n = encodeTelemetry(t, buf, sizeof(buf));
  const uint8_t expected[] = {0x01, 0x02, 0x04, 0xE2, 0x02, 0x01, 0x01};
  TEST_ASSERT_EQUAL(sizeof(expected), n);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, buf, n);
}

static void test_full_frame_vector() {
  // Same result as the gateway frame tool (cd gateway && npm run frame -- telemetry --src 0x1001 --seq 42 --level 1250 --valve 1)
  Frame f;
  f.type = MsgType::Telemetry;
  f.src = 0x1001;
  f.seq = 42;
  Telemetry t;
  t.hasLevel = true; t.levelMm = 1250;
  t.hasValve = true; t.valve = 1;
  f.len = static_cast<uint8_t>(encodeTelemetry(t, f.payload, sizeof(f.payload)));
  uint8_t buf[MAX_FRAME];
  const size_t n = encodeFrame(f, buf, sizeof(buf));
  const uint8_t expected[] = {0x01, 0x01, 0x10, 0x01, 0x2A, 0x07, 0x01, 0x02,
                              0x04, 0xE2, 0x02, 0x01, 0x01, 0x45, 0xCB};
  TEST_ASSERT_EQUAL(sizeof(expected), n);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, buf, n);
}

static void test_telemetry_roundtrip_and_unknown_tag() {
  Telemetry t;
  t.hasLevel = true; t.levelMm = 900;
  t.hasPressure = true; t.pressureCbar = 210;
  t.hasFlow = true; t.flowDlMin = 35;
  t.hasAlarms = true; t.alarms = alarm::Overflow;
  uint8_t buf[32];
  size_t n = encodeTelemetry(t, buf, sizeof(buf));
  // Append an unknown tag at the end (like a sensor added in the future)
  buf[n++] = 0x40; buf[n++] = 2; buf[n++] = 0x12; buf[n++] = 0x34;

  Telemetry d;
  TEST_ASSERT_TRUE(decodeTelemetry(buf, n, d));
  TEST_ASSERT_TRUE(d.hasLevel);   TEST_ASSERT_EQUAL(900, d.levelMm);
  TEST_ASSERT_FALSE(d.hasValve);
  TEST_ASSERT_TRUE(d.hasPressure); TEST_ASSERT_EQUAL(210, d.pressureCbar);
  TEST_ASSERT_TRUE(d.hasFlow);    TEST_ASSERT_EQUAL(35, d.flowDlMin);
  TEST_ASSERT_TRUE(d.hasAlarms);  TEST_ASSERT_EQUAL(alarm::Overflow, d.alarms);
}

static void test_telemetry_rejects_truncated() {
  const uint8_t bad[] = {0x01, 0x02, 0x04};
  Telemetry d;
  TEST_ASSERT_FALSE(decodeTelemetry(bad, sizeof(bad), d));
}

static void test_command_roundtrip() {
  Command c;
  c.cmdId = 0xBEEF;
  c.op = Opcode::SetPeriod;
  c.argLen = 2; c.args[0] = 0x00; c.args[1] = 0x3C;
  uint8_t buf[16];
  const size_t n = encodeCommand(c, buf, sizeof(buf));
  TEST_ASSERT_EQUAL(5, n);
  Command d;
  TEST_ASSERT_TRUE(decodeCommand(buf, n, d));
  TEST_ASSERT_EQUAL_HEX16(0xBEEF, d.cmdId);
  TEST_ASSERT_EQUAL(Opcode::SetPeriod, d.op);
  TEST_ASSERT_EQUAL(2, d.argLen);
  TEST_ASSERT_EQUAL_HEX8(0x3C, d.args[1]);
}

static void test_cmd_result_roundtrip() {
  CmdResult r;
  r.cmdId = 7;
  r.result = Result::Ok;
  r.state.hasValve = true; r.state.valve = 1;
  uint8_t buf[16];
  const size_t n = encodeCmdResult(r, buf, sizeof(buf));
  TEST_ASSERT_EQUAL(6, n);
  CmdResult d;
  TEST_ASSERT_TRUE(decodeCmdResult(buf, n, d));
  TEST_ASSERT_EQUAL(7, d.cmdId);
  TEST_ASSERT_EQUAL(Result::Ok, d.result);
  TEST_ASSERT_TRUE(d.state.hasValve);
  TEST_ASSERT_EQUAL(1, d.state.valve);
}

static void test_duplicate_filter() {
  DuplicateFilter f;
  TEST_ASSERT_FALSE(f.seen(0x1001, 5));
  TEST_ASSERT_TRUE(f.seen(0x1001, 5));   // retransmission
  TEST_ASSERT_FALSE(f.seen(0x1002, 5));  // different sender
  TEST_ASSERT_FALSE(f.seen(0x1001, 6));
  TEST_ASSERT_TRUE(f.seen(0x1001, 6));
}

static void test_assembler_splits_stream() {
  Frame a; a.src = 0x1001; a.seq = 1; a.len = 1; a.payload[0] = 0x55;
  Frame b; b.src = 0x1002; b.seq = 2; b.len = 0;
  uint8_t s[64];
  size_t n = encodeFrame(a, s, sizeof(s));
  n += encodeFrame(b, s + n, sizeof(s) - n);

  FrameAssembler as;
  int frames = 0;
  Frame d;
  for (size_t i = 0; i < n; ++i) {
    if (as.push(s[i], 100 + i)) {
      TEST_ASSERT_EQUAL(DecodeError::None, decodeFrame(as.frame(), as.frameLength(), d));
      ++frames;
    }
  }
  TEST_ASSERT_EQUAL(2, frames);
  TEST_ASSERT_EQUAL_HEX16(0x1002, d.src);
}

static void test_assembler_resyncs_after_gap() {
  Frame a; a.len = 2; a.payload[0] = 1; a.payload[1] = 2;
  uint8_t s[16];
  const size_t n = encodeFrame(a, s, sizeof(s));
  FrameAssembler as(50);
  // Partial frame, then a long silence, then a full frame
  for (size_t i = 0; i < 4; ++i) TEST_ASSERT_FALSE(as.push(s[i], i));
  bool done = false;
  for (size_t i = 0; i < n; ++i) done = as.push(s[i], 1000 + i);
  TEST_ASSERT_TRUE(done);
  TEST_ASSERT_EQUAL(n, as.frameLength());
}

// --- ReliableSender, with a fake radio and clock ---
struct FakeRadio {
  int sends = 0;
  uint16_t lastDst = 0;
};

static bool fakeSend(uint16_t dst, const uint8_t*, size_t, void* ctx) {
  auto* r = static_cast<FakeRadio*>(ctx);
  r->sends++;
  r->lastDst = dst;
  return true;
}

static void test_reliable_sender_ack() {
  FakeRadio radio;
  ReliableSender tx(fakeSend, &radio, 2000, 3);
  Frame f; f.type = MsgType::Telemetry; f.seq = 9;
  TEST_ASSERT_TRUE(tx.sendFrame(GATEWAY_ADDRESS, f, 0));
  TEST_ASSERT_TRUE(tx.busy());
  TEST_ASSERT_FALSE(tx.sendFrame(GATEWAY_ADDRESS, f, 10));  // busy
  TEST_ASSERT_FALSE(tx.onAck(GATEWAY_ADDRESS, 8));           // wrong SEQ
  TEST_ASSERT_TRUE(tx.onAck(GATEWAY_ADDRESS, 9));
  TEST_ASSERT_FALSE(tx.busy());
  TEST_ASSERT_EQUAL(1, radio.sends);
}

static void test_reliable_sender_retries_then_fails() {
  FakeRadio radio;
  ReliableSender tx(fakeSend, &radio, 2000, 3);
  Frame f; f.type = MsgType::Command; f.seq = 3;
  tx.sendFrame(0x1001, f, 0);
  TEST_ASSERT_EQUAL(ReliableSender::Event::None, tx.poll(1999));
  TEST_ASSERT_EQUAL(ReliableSender::Event::Retried, tx.poll(2000));
  TEST_ASSERT_EQUAL(ReliableSender::Event::Retried, tx.poll(4000));
  TEST_ASSERT_EQUAL(ReliableSender::Event::Failed, tx.poll(6000));
  TEST_ASSERT_EQUAL(3, radio.sends);
  TEST_ASSERT_EQUAL(3, tx.lastFailedSeq());
  TEST_ASSERT_FALSE(tx.busy());
}

static void test_heartbeat_needs_no_ack() {
  FakeRadio radio;
  ReliableSender tx(fakeSend, &radio);
  Frame f; f.type = MsgType::Heartbeat;
  TEST_ASSERT_TRUE(tx.sendFrame(GATEWAY_ADDRESS, f, 0));
  TEST_ASSERT_FALSE(tx.busy());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_crc16_check_value);
  RUN_TEST(test_frame_roundtrip);
  RUN_TEST(test_frame_fits_e32_packet);
  RUN_TEST(test_frame_detects_corruption);
  RUN_TEST(test_telemetry_doc_example);
  RUN_TEST(test_full_frame_vector);
  RUN_TEST(test_telemetry_roundtrip_and_unknown_tag);
  RUN_TEST(test_telemetry_rejects_truncated);
  RUN_TEST(test_command_roundtrip);
  RUN_TEST(test_cmd_result_roundtrip);
  RUN_TEST(test_duplicate_filter);
  RUN_TEST(test_assembler_splits_stream);
  RUN_TEST(test_assembler_resyncs_after_gap);
  RUN_TEST(test_reliable_sender_ack);
  RUN_TEST(test_reliable_sender_retries_then_fails);
  RUN_TEST(test_heartbeat_needs_no_ack);
  return UNITY_END();
}
