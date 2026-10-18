#include "SupabaseRest.h"

#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "root_ca.h"

namespace {
constexpr uint32_t REFRESH_MARGIN_MS = 5UL * 60UL * 1000UL;  // refresh 5 min before expiry
constexpr uint16_t HTTP_TIMEOUT_MS = 8000;
}  // namespace

bool SupabaseRest::login(const char* email, const char* password) {
  email_ = email;
  password_ = password;
  JsonDocument body;
  body["email"] = email;
  body["password"] = password;
  String s;
  serializeJson(body, s);
  return tokenRequest("password", s);
}

bool SupabaseRest::ensureSession() {
  if (!loggedIn()) return login(email_.c_str(), password_.c_str());
  if (static_cast<int32_t>(expiresAtMs_ - millis()) > static_cast<int32_t>(REFRESH_MARGIN_MS)) {
    return true;
  }
  JsonDocument body;
  body["refresh_token"] = refreshToken_;
  String s;
  serializeJson(body, s);
  if (tokenRequest("refresh_token", s)) return true;
  // Refresh failed: sign in from scratch
  accessToken_ = "";
  return login(email_.c_str(), password_.c_str());
}

bool SupabaseRest::tokenRequest(const char* grantType, const String& body) {
  String response;
  const String url = url_ + "/auth/v1/token?grant_type=" + grantType;
  if (!request("POST", url, body, &response)) return false;

  JsonDocument doc;
  if (deserializeJson(doc, response)) return false;
  accessToken_ = doc["access_token"].as<String>();
  refreshToken_ = doc["refresh_token"].as<String>();
  const uint32_t expiresIn = doc["expires_in"] | 3600;
  expiresAtMs_ = millis() + expiresIn * 1000UL;
  return accessToken_.length() > 0;
}

bool SupabaseRest::get(const String& pathAndQuery, JsonDocument& doc) {
  String response;
  if (!request("GET", url_ + "/rest/v1/" + pathAndQuery, "", &response)) return false;
  return !deserializeJson(doc, response);
}

bool SupabaseRest::insert(const char* table, const JsonDocument& body, bool upsert) {
  String s;
  serializeJson(body, s);
  const char* prefer = upsert ? "return=minimal,resolution=merge-duplicates" : "return=minimal";
  return request("POST", url_ + "/rest/v1/" + table, s, nullptr, prefer);
}

bool SupabaseRest::update(const char* table, const String& filter, const JsonDocument& body) {
  String s;
  serializeJson(body, s);
  return request("PATCH", url_ + "/rest/v1/" + table + "?" + filter, s, nullptr, "return=minimal");
}

bool SupabaseRest::request(const char* method, const String& url, const String& body,
                           String* response, const char* prefer) {
  WiFiClientSecure client;
  // The server certificate is verified against the bundled root CAs (root_ca.h).
  // This needs a correct system clock, which main.cpp sets via NTP before the
  // first request; with the clock at 1970 every certificate looks "not yet valid".
  client.setCACert(ROOT_CA_BUNDLE);

  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);
  if (!http.begin(client, url)) {
    lastStatus_ = -1;
    return false;
  }
  http.addHeader("apikey", anonKey_);
  http.addHeader("Content-Type", "application/json");
  if (accessToken_.length()) http.addHeader("Authorization", "Bearer " + accessToken_);
  if (prefer) http.addHeader("Prefer", prefer);

  lastStatus_ = http.sendRequest(method, body);
  if (response && lastStatus_ > 0) *response = http.getString();
  http.end();

  if (lastStatus_ < 200 || lastStatus_ >= 300) {
    Serial.printf("[supabase] %s %s -> %d\n", method, url.c_str(), lastStatus_);
    if (lastStatus_ == 401) accessToken_ = "";  // the next ensureSession signs in again
    return false;
  }
  return true;
}
