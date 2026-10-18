// Small client for Supabase REST (PostgREST + Auth).
//
// There is no official Supabase library for the ESP32 (docs/02-sistem-mimarisi.md K8),
// so plain HTTPS requests are used. The endpoints used are listed in §6.2.
#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

class SupabaseRest {
 public:
  SupabaseRest(const char* url, const char* anonKey) : url_(url), anonKey_(anonKey) {}

  // Signs in with email/password and stores the access token.
  bool login(const char* email, const char* password);

  // Refreshes the token when it is about to expire. Call it from the loop.
  bool ensureSession();

  bool loggedIn() const { return accessToken_.length() > 0; }

  // GET /rest/v1/<pathAndQuery>; the JSON response is written into doc.
  bool get(const String& pathAndQuery, JsonDocument& doc);

  // POST /rest/v1/<table>. With upsert=true a row with the same key is updated.
  bool insert(const char* table, const JsonDocument& body, bool upsert = false);

  // PATCH /rest/v1/<table>?<filter>
  bool update(const char* table, const String& filter, const JsonDocument& body);

  int lastStatus() const { return lastStatus_; }

 private:
  bool request(const char* method, const String& url, const String& body, String* response,
               const char* prefer = nullptr);
  bool tokenRequest(const char* grantType, const String& body);

  String url_;
  String anonKey_;
  String email_;
  String password_;
  String accessToken_;
  String refreshToken_;
  uint32_t expiresAtMs_ = 0;
  int lastStatus_ = 0;
};
