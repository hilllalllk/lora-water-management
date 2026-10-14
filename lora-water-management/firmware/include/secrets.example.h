// Bu dosyayı "secrets.h" adıyla kopyalayıp kendi bilgilerinizi yazın.
// secrets.h .gitignore içindedir, repoya GİTMEZ.
//
//   cp include/secrets.example.h include/secrets.h
#pragma once

// Wi-Fi (sadece gateway)
#define WIFI_SSID     "wifi-adi"
#define WIFI_PASSWORD "wifi-sifresi"

// Supabase projesi: Project Settings > API
#define SUPABASE_URL      "https://xxxxxxxxxxxx.supabase.co"
#define SUPABASE_ANON_KEY "anon-veya-publishable-key"
// ÖNEMLİ: service_role anahtarını ASLA buraya yazmayın.

// Gateway kullanıcısı (Supabase Auth). app_metadata.role = "gateway" olmalı.
// Kurulum: docs/05-kurulum.md
#define GATEWAY_EMAIL    "gateway@example.com"
#define GATEWAY_PASSWORD "gateway-sifresi"
#define GATEWAY_ID       "gw-01"
