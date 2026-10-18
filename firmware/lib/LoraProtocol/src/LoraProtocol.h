// LoRa Water Management System - link protocol v1
// Full specification: docs/03-protokol.md
//
// This library is hardware independent (it uses no Arduino headers), so it
// can be verified with unit tests on a PC (pio test -e native).
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace lwm {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
constexpr uint8_t PROTOCOL_VERSION = 0x01;

constexpr size_t E32_MAX_PACKET = 58;  // E32 carries at most 58 bytes per radio packet
constexpr size_t E32_FIXED_HEADER = 3; // ADDH, ADDL, CHAN (consumed by the module)
constexpr size_t FRAME_HEADER = 6;     // VER, TYPE, SRC(2), SEQ, LEN
constexpr size_t FRAME_CRC = 2;
constexpr size_t FRAME_OVERHEAD = FRAME_HEADER + FRAME_CRC;          // 8
constexpr size_t MAX_FRAME = E32_MAX_PACKET - E32_FIXED_HEADER;       // 55
constexpr size_t MAX_PAYLOAD = MAX_FRAME - FRAME_OVERHEAD;            // 47

constexpr uint16_t GATEWAY_ADDRESS = 0x0001;
constexpr uint16_t BROADCAST_ADDRESS = 0xFFFF;

enum class MsgType : uint8_t {
  Telemetry = 0x01,
  Heartbeat = 0x02,
  Command = 0x10,
  CmdResult = 0x11,
  Ack = 0x7F,
};

enum class Opcode : uint8_t {
  ValveOpen = 0x01,
  ValveClose = 0x02,
  DeviceEnable = 0x03,
  ReadNow = 0x04,
  SetPeriod = 0x05,
};

enum class Result : uint8_t {
  Ok = 0x00,
  UnknownCommand = 0x01,
  DeviceError = 0x02,
  Timeout = 0x03,
  AlreadyInState = 0x04,
};

enum class Tag : uint8_t {
  LevelMm = 0x01,
  Valve = 0x02,
  PressureCbar = 0x03,
  FlowDlMin = 0x04,
  Alarms = 0x05,
  SupplyMv = 0x06,
};

enum class ValveState : uint8_t { Closed = 0, Open = 1, Moving = 2, Fault = 3 };

namespace alarm {
constexpr uint8_t Overflow = 1 << 0;
constexpr uint8_t DryRun = 1 << 1;
constexpr uint8_t SensorFault = 1 << 2;
}  // namespace alarm

// Does this message type expect an ACK from the receiver?
bool requiresAck(MsgType type);

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------
struct Frame {
  uint8_t version = PROTOCOL_VERSION;
  MsgType type = MsgType::Telemetry;
  uint16_t src = 0;
  uint8_t seq = 0;
  uint8_t len = 0;
  uint8_t payload[MAX_PAYLOAD] = {};
};

enum class DecodeError : uint8_t {
  None = 0,
  TooShort,
  BadVersion,
  BadLength,
  BadCrc,
};

// CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF). "123456789" -> 0x29B1
uint16_t crc16(const uint8_t* data, size_t len, uint16_t crc = 0xFFFF);

// Serialises the frame into a byte buffer. Returns the number of bytes written, or 0 on error.
size_t encodeFrame(const Frame& frame, uint8_t* out, size_t capacity);

// Parses a byte buffer into a frame and verifies the CRC.
DecodeError decodeFrame(const uint8_t* in, size_t len, Frame& out);

const char* decodeErrorName(DecodeError err);

// ---------------------------------------------------------------------------
// Telemetry (TLV)
// ---------------------------------------------------------------------------
struct Telemetry {
  bool hasLevel = false;     uint16_t levelMm = 0;
  bool hasValve = false;     uint8_t valve = 0;
  bool hasPressure = false;  uint16_t pressureCbar = 0;
  bool hasFlow = false;      uint16_t flowDlMin = 0;
  bool hasAlarms = false;    uint8_t alarms = 0;
  bool hasSupply = false;    uint16_t supplyMv = 0;
};

// Returns the number of bytes written, or 0 if it does not fit.
size_t encodeTelemetry(const Telemetry& t, uint8_t* out, size_t capacity);

// Unknown tags are skipped. Returns false on malformed TLV.
bool decodeTelemetry(const uint8_t* in, size_t len, Telemetry& out);

// ---------------------------------------------------------------------------
// Command and command result
// ---------------------------------------------------------------------------
constexpr size_t MAX_COMMAND_ARGS = 8;

struct Command {
  uint16_t cmdId = 0;
  Opcode op = Opcode::ReadNow;
  uint8_t argLen = 0;
  uint8_t args[MAX_COMMAND_ARGS] = {};
};

size_t encodeCommand(const Command& c, uint8_t* out, size_t capacity);
bool decodeCommand(const uint8_t* in, size_t len, Command& out);

struct CmdResult {
  uint16_t cmdId = 0;
  Result result = Result::Ok;
  Telemetry state;  // state after the command
};

size_t encodeCmdResult(const CmdResult& r, uint8_t* out, size_t capacity);
bool decodeCmdResult(const uint8_t* in, size_t len, CmdResult& out);

// ---------------------------------------------------------------------------
// Duplicate packet filter
// ---------------------------------------------------------------------------
// Stores the last SEQ seen from each sender. If the same SEQ arrives again the
// packet is a retransmission: the receiver ACKs it again but does not process it.
class DuplicateFilter {
 public:
  static constexpr size_t MAX_PEERS = 32;
  // true: seen before (duplicate). false: new packet, now recorded.
  bool seen(uint16_t src, uint8_t seq);
  void reset();

 private:
  struct Entry {
    uint16_t src;
    uint8_t seq;
    bool used;
  };
  Entry entries_[MAX_PEERS] = {};
  size_t next_ = 0;  // when full, the oldest entry is overwritten
};

// ---------------------------------------------------------------------------
// Frame assembly from the UART byte stream
// ---------------------------------------------------------------------------
// The E32 outputs a radio packet on the UART as consecutive bytes. The assembler
// uses the LEN field to find where the frame ends; if there is a long gap between
// bytes (gapMs) it drops the partial frame and resynchronises.
class FrameAssembler {
 public:
  explicit FrameAssembler(uint32_t gapMs = 50) : gapMs_(gapMs) {}

  // Adds one byte. Returns true when a frame is complete; read it with frame().
  bool push(uint8_t byte, uint32_t nowMs);
  const uint8_t* frame() const { return buf_; }
  size_t frameLength() const { return complete_; }
  void reset();

 private:
  uint8_t buf_[MAX_FRAME] = {};
  size_t pos_ = 0;
  size_t complete_ = 0;
  uint32_t lastByteMs_ = 0;
  uint32_t gapMs_;
};

// ---------------------------------------------------------------------------
// Stop-and-wait reliable sending (ARQ)
// ---------------------------------------------------------------------------
// Hardware independent: sending is done through a callback and the time is
// passed in, so it can be tested on a PC with a fake clock.
class ReliableSender {
 public:
  using SendFn = bool (*)(uint16_t dst, const uint8_t* data, size_t len, void* ctx);

  ReliableSender(SendFn send, void* ctx, uint32_t ackTimeoutMs = 2000, uint8_t maxTries = 3)
      : send_(send), ctx_(ctx), timeoutMs_(ackTimeoutMs), maxTries_(maxTries) {}

  bool busy() const { return active_; }

  // Sends the frame; for types that require an ACK, tracks it until the ACK arrives.
  // Returns false while busy (previous message still waiting for its ACK).
  bool sendFrame(uint16_t dst, const Frame& frame, uint32_t nowMs);

  // Handles an incoming ACK. Returns true if it matches the expected SEQ.
  bool onAck(uint16_t from, uint8_t ackSeq);

  enum class Event : uint8_t { None, Retried, Failed };
  // Call often from the loop: retransmits on timeout, returns Failed when tries run out.
  Event poll(uint32_t nowMs);

  uint8_t lastFailedSeq() const { return failedSeq_; }
  uint8_t tries() const { return tries_; }

 private:
  SendFn send_;
  void* ctx_;
  uint32_t timeoutMs_;
  uint8_t maxTries_;

  bool active_ = false;
  uint16_t dst_ = 0;
  uint8_t seq_ = 0;
  uint8_t tries_ = 0;
  uint32_t lastSendMs_ = 0;
  uint8_t buf_[MAX_FRAME] = {};
  size_t len_ = 0;
  uint8_t failedSeq_ = 0;
};

// Shortcut for building an ACK frame
Frame makeAck(uint16_t src, uint8_t seq, uint8_t ackSeq);

}  // namespace lwm
