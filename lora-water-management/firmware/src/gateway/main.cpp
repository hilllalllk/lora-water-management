// Gateway (merkez) yazılımı: LoRa <-> Supabase köprüsü.
//
// Görevleri (docs/02-sistem-mimarisi.md §5):
//   1. LoRa alıcı döngüsü: çerçeve -> CRC -> tekrar ayıklama -> ACK -> Supabase
//   2. Komut sorgulayıcı: bekleyen komutları REST ile çek -> LoRa'ya çevir -> durum güncelle
//   3. Çevrimiçi takip: devices.last_seen (tetikleyici ve heartbeat ile)
//   4. Kendi sağlığı: gateways.last_seen her 60 s
//
// Gateway veriyi yorumlamaz; sadece protokol ile veritabanı arasında çeviri yapar.
#include <Arduino.h>
#include <ArduinoJson.h>
#include <E32Radio.h>
#include <LoraProtocol.h>
#include <WiFi.h>

#include "SupabaseRest.h"
#include "config.h"
#include "secrets.h"

using namespace lwm;

namespace {

constexpr uint32_t COMMAND_POLL_MS = 1500;
constexpr uint32_t DEVICE_REFRESH_MS = 5UL * 60UL * 1000UL;
constexpr uint32_t GATEWAY_HEARTBEAT_MS = 60UL * 1000UL;
constexpr size_t MAX_DEVICES = 32;
constexpr size_t RECENT_COMMANDS = 8;

E32Radio radio(Serial2, {PIN_E32_RX, PIN_E32_TX, PIN_E32_M0, PIN_E32_M1, PIN_E32_AUX});
SupabaseRest db(SUPABASE_URL, SUPABASE_ANON_KEY);

bool radioSend(uint16_t dst, const uint8_t* data, size_t len, void*) {
  return radio.send(dst, data, len);
}

ReliableSender sender(radioSend, nullptr, ACK_TIMEOUT_MS, MAX_TRIES);
FrameAssembler assembler(FRAME_GAP_MS);
DuplicateFilter duplicates;
uint8_t txSeq = 0;

// LoRa adresi -> devices.id eşlemesi (Supabase'den okunur)
struct DeviceEntry {
  uint16_t address;
  String id;
};
DeviceEntry devices[MAX_DEVICES];
size_t deviceCount = 0;

// Gönderilen komutlar: LoRa'daki 16 bitlik cmdId'den veritabanı satırına dönmek için
struct SentCommand {
  bool used;
  int64_t dbId;
  uint16_t address;
};
SentCommand sentCommands[RECENT_COMMANDS] = {};
size_t sentNext = 0;
int64_t inFlightDbId = -1;  // ACK beklenen komut

uint32_t nextCommandPollMs = 0;
uint32_t nextDeviceRefreshMs = 0;
uint32_t nextGatewayHeartbeatMs = 0;

// --- Yardımcılar -------------------------------------------------------------

const String* deviceIdFor(uint16_t address) {
  for (size_t i = 0; i < deviceCount; ++i) {
    if (devices[i].address == address) return &devices[i].id;
  }
  return nullptr;
}

bool opFromText(const char* op, Opcode& out) {
  if (!strcmp(op, "VANA_AC")) { out = Opcode::ValveOpen; return true; }
  if (!strcmp(op, "VANA_KAPAT")) { out = Opcode::ValveClose; return true; }
  if (!strcmp(op, "CIHAZ_ETKIN")) { out = Opcode::DeviceEnable; return true; }
  if (!strcmp(op, "SIMDI_OKU")) { out = Opcode::ReadNow; return true; }
  if (!strcmp(op, "PERIYOT_AYARLA")) { out = Opcode::SetPeriod; return true; }
  return false;
}

void setCommandStatus(int64_t dbId, const char* status, int result = -1) {
  JsonDocument body;
  body["status"] = status;
  if (result >= 0) body["result"] = result;
  db.update("commands", "id=eq." + String(static_cast<long long>(dbId)), body);
}

void putTelemetry(JsonDocument& row, const Telemetry& t) {
  if (t.hasLevel) row["level_mm"] = t.levelMm;
  if (t.hasValve) row["valve"] = t.valve;
  if (t.hasPressure) row["pressure_cbar"] = t.pressureCbar;
  if (t.hasFlow) row["flow_dl_min"] = t.flowDlMin;
  if (t.hasAlarms) row["alarms"] = t.alarms;
}

void sendAck(uint16_t dst, uint8_t ackSeq) {
  uint8_t buf[MAX_FRAME];
  const size_t n = encodeFrame(makeAck(GATEWAY_ADDRESS, txSeq++, ackSeq), buf, sizeof(buf));
  radio.send(dst, buf, n);
}

// --- Supabase -> LoRa ---------------------------------------------------------

void refreshDevices() {
  JsonDocument doc;
  if (!db.get("devices?select=id,address", doc)) return;
  deviceCount = 0;
  for (JsonObject d : doc.as<JsonArray>()) {
    if (deviceCount >= MAX_DEVICES) break;
    devices[deviceCount++] = {static_cast<uint16_t>(d["address"].as<int>()), d["id"].as<String>()};
  }
  Serial.printf("[gw] %u cihaz yuklendi\n", static_cast<unsigned>(deviceCount));
}

void pollCommands() {
  JsonDocument doc;
  if (!db.get("commands?status=eq.pending&order=id.asc&limit=1"
              "&select=id,op,args,device_id,devices(address)",
              doc)) {
    return;
  }
  JsonArray rows = doc.as<JsonArray>();
  if (rows.size() == 0) return;
  JsonObject row = rows[0];

  const int64_t dbId = row["id"].as<long long>();
  const uint16_t address = static_cast<uint16_t>(row["devices"]["address"].as<int>());

  Command cmd;
  cmd.cmdId = static_cast<uint16_t>(dbId & 0xFFFF);
  if (!opFromText(row["op"] | "", cmd.op)) {
    setCommandStatus(dbId, "failed", static_cast<int>(Result::UnknownCommand));
    return;
  }
  if (cmd.op == Opcode::DeviceEnable) {
    cmd.argLen = 1;
    cmd.args[0] = (row["args"]["enabled"] | true) ? 1 : 0;
  } else if (cmd.op == Opcode::SetPeriod) {
    const uint16_t s = row["args"]["seconds"] | TELEMETRY_PERIOD_S_DEFAULT;
    cmd.argLen = 2;
    cmd.args[0] = s >> 8;
    cmd.args[1] = s & 0xFF;
  }

  Frame f;
  f.type = MsgType::Command;
  f.src = GATEWAY_ADDRESS;
  f.seq = txSeq;
  f.len = static_cast<uint8_t>(encodeCommand(cmd, f.payload, sizeof(f.payload)));
  if (!sender.sendFrame(address, f, millis())) return;
  ++txSeq;

  inFlightDbId = dbId;
  sentCommands[sentNext] = {true, dbId, address};
  sentNext = (sentNext + 1) % RECENT_COMMANDS;
  setCommandStatus(dbId, "sent");
  Serial.printf("[gw] komut %lld -> 0x%04X (%s)\n", static_cast<long long>(dbId), address,
                row["op"].as<const char*>());
}

// --- LoRa -> Supabase ---------------------------------------------------------

void insertTelemetry(const String& deviceId, const Telemetry& t) {
  JsonDocument row;
  row["device_id"] = deviceId;
  putTelemetry(row, t);
  db.insert("telemetry", row);
}

void handleFrame(const Frame& f) {
  if (f.type == MsgType::Ack) {
    if (f.len == 1 && sender.onAck(f.src, f.payload[0]) && inFlightDbId >= 0) {
      setCommandStatus(inFlightDbId, "delivered");
      inFlightDbId = -1;
    }
    return;
  }

  if (requiresAck(f.type)) sendAck(f.src, f.seq);
  if (duplicates.seen(f.src, f.seq)) return;  // yeniden gönderim: ACK verildi, işleme

  const String* deviceId = deviceIdFor(f.src);
  if (!deviceId) {
    Serial.printf("[gw] bilinmeyen adres 0x%04X (devices tablosuna ekleyin)\n", f.src);
    return;
  }

  switch (f.type) {
    case MsgType::Telemetry: {
      Telemetry t;
      if (decodeTelemetry(f.payload, f.len, t)) insertTelemetry(*deviceId, t);
      break;
    }
    case MsgType::CmdResult: {
      CmdResult r;
      if (!decodeCmdResult(f.payload, f.len, r)) break;
      for (auto& s : sentCommands) {
        if (s.used && s.address == f.src && static_cast<uint16_t>(s.dbId & 0xFFFF) == r.cmdId) {
          const bool ok = r.result == Result::Ok || r.result == Result::AlreadyInState;
          setCommandStatus(s.dbId, ok ? "done" : "failed", static_cast<int>(r.result));
          s.used = false;
          break;
        }
      }
      insertTelemetry(*deviceId, r.state);
      break;
    }
    case MsgType::Heartbeat: {
      JsonDocument body;
      body["last_seen"] = "now";  // Postgres özel değeri: sunucu saati
      if (f.len > 4) {
        char fw[MAX_PAYLOAD + 1] = {};
        memcpy(fw, f.payload + 4, f.len - 4);
        body["fw"] = fw;
      }
      db.update("devices", "id=eq." + *deviceId, body);
      break;
    }
    default:
      break;
  }
}

void pollRadio() {
  while (radio.available()) {
    const uint8_t b = static_cast<uint8_t>(radio.read());
    if (!assembler.push(b, millis())) continue;
    Frame f;
    const DecodeError err = decodeFrame(assembler.frame(), assembler.frameLength(), f);
    if (err != DecodeError::None) {
      Serial.printf("[gw] cerceve hatasi: %s\n", decodeErrorName(err));
      continue;
    }
    handleFrame(f);
  }
}

void connectWifi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.printf("[gw] Wi-Fi'ye baglaniliyor: %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    pollRadio();  // bağlanırken gelen paketleri kaçırma
    delay(50);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[gw] Wi-Fi bagli, IP=%s\n", WiFi.localIP().toString().c_str());
  }
}

void gatewayHeartbeat() {
  JsonDocument row;
  row["id"] = GATEWAY_ID;
  row["fw"] = FIRMWARE_VERSION;
  row["wifi_rssi"] = WiFi.RSSI();
  row["last_seen"] = "now";
  db.insert("gateways", row, true);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.printf("\n[gw] gateway %s fw=%s\n", GATEWAY_ID, FIRMWARE_VERSION);
  randomSeed(esp_random());
  txSeq = static_cast<uint8_t>(random(0, 256));

  E32Config cfg;
  cfg.address = GATEWAY_ADDRESS;
  cfg.channel = LORA_CHANNEL;
  if (!radio.begin(cfg)) Serial.println(F("[gw] UYARI: E32 yapilandirmasi dogrulanamadi"));

  connectWifi();
  if (!db.login(GATEWAY_EMAIL, GATEWAY_PASSWORD)) {
    Serial.println(F("[gw] Supabase girisi basarisiz, tekrar denenecek"));
  }
}

void loop() {
  const uint32_t now = millis();
  pollRadio();

  if (sender.poll(now) == ReliableSender::Event::Failed && inFlightDbId >= 0) {
    Serial.printf("[gw] komut %lld icin ACK alinamadi\n", static_cast<long long>(inFlightDbId));
    setCommandStatus(inFlightDbId, "failed", static_cast<int>(Result::Timeout));
    inFlightDbId = -1;
  }

  if (WiFi.status() != WL_CONNECTED) {
    connectWifi();
    return;
  }
  if (!db.ensureSession()) return;

  if (static_cast<int32_t>(now - nextDeviceRefreshMs) >= 0) {
    refreshDevices();
    nextDeviceRefreshMs = now + DEVICE_REFRESH_MS;
  }
  if (static_cast<int32_t>(now - nextGatewayHeartbeatMs) >= 0) {
    gatewayHeartbeat();
    nextGatewayHeartbeatMs = now + GATEWAY_HEARTBEAT_MS;
  }
  if (!sender.busy() && static_cast<int32_t>(now - nextCommandPollMs) >= 0) {
    pollCommands();
    nextCommandPollMs = now + COMMAND_POLL_MS;
  }
}
