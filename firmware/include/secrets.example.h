// Copy this file to "secrets.h" and fill in your own values.
// secrets.h is listed in .gitignore and must NEVER be committed.
//
//   cp include/secrets.example.h include/secrets.h
//
// Only the gateway firmware (env:gateway) uses these values.
#pragma once

// Wi-Fi (2.4 GHz; the ESP32 does not support 5 GHz)
#define WIFI_SSID     "wifi-name"
#define WIFI_PASSWORD "wifi-password"

// Supabase project: Project Settings > API
#define SUPABASE_URL      "https://xxxxxxxxxxxx.supabase.co"
#define SUPABASE_ANON_KEY "anon-or-publishable-key"
// IMPORTANT: NEVER put the service_role key here.

// Gateway user (Supabase Auth). Its app_metadata.role must be "gateway".
// Setup: docs/05-kurulum.md
#define GATEWAY_EMAIL    "gateway@example.com"
#define GATEWAY_PASSWORD "gateway-password"
#define GATEWAY_ID       "gw-01"
