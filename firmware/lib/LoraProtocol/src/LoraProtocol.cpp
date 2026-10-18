#include "LoraProtocol.h"

#include <string.h>

namespace lwm {

namespace {

void putU16(uint8_t* p, uint16_t v) {
  p[0] = static_cast<uint8_t>(v >> 8);
  p[1] = static_cast<uint8_t>(v & 0xFF);
}

uint16_t getU16(const uint8_t* p) {
  return static_cast<uint16_t>((p[0] << 8) | p[1]);
}

// TLV writer helpers. Return false if the value does not fit.
bool putTlv8(uint8_t* out, size_t cap, size_t& pos, Tag tag, uint8_t v) {
  if (pos + 3 > cap) return false;
  out[pos++] = static_cast<uint8_t>(tag);
  out[pos++] = 1;
  out[pos++] = v;
  return true;
}

bool putTlv16(uint8_t* out, size_t cap, size_t& pos, Tag tag, uint16_t v) {
  if (pos + 4 > cap) return false;
  out[pos++] = static_cast<uint8_t>(tag);
  out[pos++] = 2;
  putU16(out + pos, v);
  pos += 2;
  return true;
}

}  // namespace

bool requiresAck(MsgType type) {
  switch (type) {
    case MsgType::Telemetry:
    case MsgType::Command:
    case MsgType::CmdResult:
      return true;
    default:
      return false;
  }
}

uint16_t crc16(const uint8_t* data, size_t len, uint16_t crc) {
  for (size_t i = 0; i < len; ++i) {
    crc ^= static_cast<uint16_t>(data[i]) << 8;
    for (int b = 0; b < 8; ++b) {
      crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021)
                           : static_cast<uint16_t>(crc << 1);
    }
  }
  return crc;
}

size_t encodeFrame(const Frame& f, uint8_t* out, size_t capacity) {
  if (f.len > MAX_PAYLOAD) return 0;
  const size_t total = FRAME_OVERHEAD + f.len;
  if (capacity < total) return 0;

  out[0] = f.version;
  out[1] = static_cast<uint8_t>(f.type);
  putU16(out + 2, f.src);
  out[4] = f.seq;
  out[5] = f.len;
  if (f.len) memcpy(out + FRAME_HEADER, f.payload, f.len);
  putU16(out + FRAME_HEADER + f.len, crc16(out, FRAME_HEADER + f.len));
  return total;
}

DecodeError decodeFrame(const uint8_t* in, size_t len, Frame& out) {
  if (len < FRAME_OVERHEAD) return DecodeError::TooShort;
  if (in[0] != PROTOCOL_VERSION) return DecodeError::BadVersion;
  const uint8_t payloadLen = in[5];
  if (payloadLen > MAX_PAYLOAD || len != FRAME_OVERHEAD + payloadLen) return DecodeError::BadLength;
  const uint16_t expected = getU16(in + FRAME_HEADER + payloadLen);
  if (crc16(in, FRAME_HEADER + payloadLen) != expected) return DecodeError::BadCrc;

  out.version = in[0];
  out.type = static_cast<MsgType>(in[1]);
  out.src = getU16(in + 2);
  out.seq = in[4];
  out.len = payloadLen;
  memcpy(out.payload, in + FRAME_HEADER, payloadLen);
  return DecodeError::None;
}

const char* decodeErrorName(DecodeError err) {
  switch (err) {
    case DecodeError::None: return "ok";
    case DecodeError::TooShort: return "too_short";
    case DecodeError::BadVersion: return "bad_version";
    case DecodeError::BadLength: return "bad_length";
    case DecodeError::BadCrc: return "bad_crc";
  }
  return "unknown";
}

// --- Telemetry --------------------------------------------------------------

size_t encodeTelemetry(const Telemetry& t, uint8_t* out, size_t cap) {
  size_t pos = 0;
  if (t.hasLevel && !putTlv16(out, cap, pos, Tag::LevelMm, t.levelMm)) return 0;
  if (t.hasValve && !putTlv8(out, cap, pos, Tag::Valve, t.valve)) return 0;
  if (t.hasPressure && !putTlv16(out, cap, pos, Tag::PressureCbar, t.pressureCbar)) return 0;
  if (t.hasFlow && !putTlv16(out, cap, pos, Tag::FlowDlMin, t.flowDlMin)) return 0;
  if (t.hasAlarms && !putTlv8(out, cap, pos, Tag::Alarms, t.alarms)) return 0;
  if (t.hasSupply && !putTlv16(out, cap, pos, Tag::SupplyMv, t.supplyMv)) return 0;
  return pos;
}

bool decodeTelemetry(const uint8_t* in, size_t len, Telemetry& t) {
  t = Telemetry();
  size_t pos = 0;
  while (pos < len) {
    if (pos + 2 > len) return false;
    const uint8_t tag = in[pos];
    const uint8_t l = in[pos + 1];
    pos += 2;
    if (pos + l > len) return false;
    const uint8_t* v = in + pos;
    switch (static_cast<Tag>(tag)) {
      case Tag::LevelMm:
        if (l != 2) return false;
        t.hasLevel = true; t.levelMm = getU16(v);
        break;
      case Tag::Valve:
        if (l != 1) return false;
        t.hasValve = true; t.valve = v[0];
        break;
      case Tag::PressureCbar:
        if (l != 2) return false;
        t.hasPressure = true; t.pressureCbar = getU16(v);
        break;
      case Tag::FlowDlMin:
        if (l != 2) return false;
        t.hasFlow = true; t.flowDlMin = getU16(v);
        break;
      case Tag::Alarms:
        if (l != 1) return false;
        t.hasAlarms = true; t.alarms = v[0];
        break;
      case Tag::SupplyMv:
        if (l != 2) return false;
        t.hasSupply = true; t.supplyMv = getU16(v);
        break;
      default:
        break;  // unknown tag: skip (forward compatibility)
    }
    pos += l;
  }
  return true;
}

// --- Command ----------------------------------------------------------------

size_t encodeCommand(const Command& c, uint8_t* out, size_t cap) {
  if (c.argLen > MAX_COMMAND_ARGS) return 0;
  const size_t total = 3 + c.argLen;
  if (cap < total) return 0;
  putU16(out, c.cmdId);
  out[2] = static_cast<uint8_t>(c.op);
  if (c.argLen) memcpy(out + 3, c.args, c.argLen);
  return total;
}

bool decodeCommand(const uint8_t* in, size_t len, Command& c) {
  if (len < 3 || len - 3 > MAX_COMMAND_ARGS) return false;
  c.cmdId = getU16(in);
  c.op = static_cast<Opcode>(in[2]);
  c.argLen = static_cast<uint8_t>(len - 3);
  memcpy(c.args, in + 3, c.argLen);
  return true;
}

size_t encodeCmdResult(const CmdResult& r, uint8_t* out, size_t cap) {
  if (cap < 3) return 0;
  putU16(out, r.cmdId);
  out[2] = static_cast<uint8_t>(r.result);
  const size_t tlv = encodeTelemetry(r.state, out + 3, cap - 3);
  // encodeTelemetry returns 0 when the state is empty; that is not an error.
  const bool hasState = r.state.hasLevel || r.state.hasValve || r.state.hasPressure ||
                        r.state.hasFlow || r.state.hasAlarms || r.state.hasSupply;
  if (hasState && tlv == 0) return 0;
  return 3 + tlv;
}

bool decodeCmdResult(const uint8_t* in, size_t len, CmdResult& r) {
  if (len < 3) return false;
  r.cmdId = getU16(in);
  r.result = static_cast<Result>(in[2]);
  return decodeTelemetry(in + 3, len - 3, r.state);
}

// --- DuplicateFilter --------------------------------------------------------

bool DuplicateFilter::seen(uint16_t src, uint8_t seq) {
  for (auto& e : entries_) {
    if (e.used && e.src == src) {
      if (e.seq == seq) return true;
      e.seq = seq;
      return false;
    }
  }
  entries_[next_] = {src, seq, true};
  next_ = (next_ + 1) % MAX_PEERS;
  return false;
}

void DuplicateFilter::reset() {
  for (auto& e : entries_) e.used = false;
  next_ = 0;
}

// --- FrameAssembler ---------------------------------------------------------

bool FrameAssembler::push(uint8_t byte, uint32_t nowMs) {
  if (complete_) {  // previous frame was consumed, start a new one
    complete_ = 0;
    pos_ = 0;
  }
  if (pos_ > 0 && nowMs - lastByteMs_ > gapMs_) pos_ = 0;  // long gap: resynchronise
  lastByteMs_ = nowMs;

  if (pos_ == 0 && byte != PROTOCOL_VERSION) return false;  // not a frame start
  buf_[pos_++] = byte;

  if (pos_ >= FRAME_HEADER) {
    const uint8_t payloadLen = buf_[5];
    if (payloadLen > MAX_PAYLOAD) {  // invalid length, drop
      pos_ = 0;
      return false;
    }
    if (pos_ == FRAME_OVERHEAD + payloadLen) {
      complete_ = pos_;
      return true;
    }
  }
  return false;
}

void FrameAssembler::reset() {
  pos_ = 0;
  complete_ = 0;
}

// --- ReliableSender ---------------------------------------------------------

bool ReliableSender::sendFrame(uint16_t dst, const Frame& frame, uint32_t nowMs) {
  if (active_) return false;
  uint8_t buf[MAX_FRAME];
  const size_t len = encodeFrame(frame, buf, sizeof(buf));
  if (!len) return false;
  if (!send_(dst, buf, len, ctx_)) return false;

  if (requiresAck(frame.type)) {
    memcpy(buf_, buf, len);
    len_ = len;
    dst_ = dst;
    seq_ = frame.seq;
    tries_ = 1;
    lastSendMs_ = nowMs;
    active_ = true;
  }
  return true;
}

bool ReliableSender::onAck(uint16_t from, uint8_t ackSeq) {
  if (!active_ || from != dst_ || ackSeq != seq_) return false;
  active_ = false;
  return true;
}

ReliableSender::Event ReliableSender::poll(uint32_t nowMs) {
  if (!active_ || nowMs - lastSendMs_ < timeoutMs_) return Event::None;
  if (tries_ >= maxTries_) {
    active_ = false;
    failedSeq_ = seq_;
    return Event::Failed;
  }
  send_(dst_, buf_, len_, ctx_);
  ++tries_;
  lastSendMs_ = nowMs;
  return Event::Retried;
}

Frame makeAck(uint16_t src, uint8_t seq, uint8_t ackSeq) {
  Frame f;
  f.type = MsgType::Ack;
  f.src = src;
  f.seq = seq;
  f.len = 1;
  f.payload[0] = ackSeq;
  return f;
}

}  // namespace lwm
