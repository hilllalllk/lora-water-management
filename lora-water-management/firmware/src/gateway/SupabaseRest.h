// Supabase REST (PostgREST + Auth) için küçük istemci.
//
// ESP32 için resmi Supabase kütüphanesi olmadığından (docs/02-sistem-mimarisi.md K8)
// doğrudan HTTPS istekleri kullanılır. Kullanılan uç noktalar §6.2'de listelidir.
#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

class SupabaseRest {
 public:
  SupabaseRest(const char* url, const char* anonKey) : url_(url), anonKey_(anonKey) {}

  // E-posta/şifre ile giriş yapar, erişim belirtecini (token) saklar.
  bool login(const char* email, const char* password);

  // Belirtecin süresi dolmak üzereyse yeniler. Döngüde çağrılmalı.
  bool ensureSession();

  bool loggedIn() const { return accessToken_.length() > 0; }

  // GET /rest/v1/<pathAndQuery>; JSON cevabı doc içine yazılır.
  bool get(const String& pathAndQuery, JsonDocument& doc);

  // POST /rest/v1/<table>. upsert=true ise aynı anahtarlı satır güncellenir.
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
