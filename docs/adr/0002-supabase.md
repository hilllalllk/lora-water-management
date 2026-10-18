# ADR-0002: Bulut olarak Supabase

- **Tarih:** 14.10.2026
- **Durum:** Kabul edildi (ön dokümandaki Firebase'in yerini aldı)

## Bağlam
Sahadan gelen verinin saklanması, mobil uygulamaya canlı iletilmesi ve uygulamadan gelen komutların gateway'e ulaşması gerekiyor.

## Seçenekler
1. **Firebase (Realtime Database / Firestore):** ESP32 kütüphaneleri olgun, NoSQL.
2. **Supabase (PostgreSQL + Auth + Realtime):** İlişkisel tablolar, SQL ile raporlama, Row Level Security; ESP32 için resmi kütüphane yok.

## Karar
**Supabase** (ekip kararı).

## Sonuçlar
- Geçmiş verilerden SQL ile rapor üretmek kolay (bitirme raporundaki grafikler için).
- Yetki kuralları tablo bazında açık ve test edilebilir (`backend/supabase/tests`).
- ESP32 gateway Supabase'e doğrudan REST (HTTPS, doğrulanan TLS) ile bağlanır ([ADR-0003](0003-gateway-rest-sorgulama.md), [ADR-0007](0007-gateway-yeniden-esp32.md)). 15–18.10.2026 arasında denenen Raspberry Pi + `supabase-js` gateway'den ([ADR-0005](0005-raspberry-pi-nodejs-gateway.md)) vazgeçildi.
- 18.10.2026: ücretsiz plan, Frankfurt (eu-central-1). Roller, eşik tabanlı alarmlar, pg_cron ile sessiz cihaz kontrolü ve alarm bildirimi için Edge Function (`send-alert-push`) eklendi ([02 §6](../02-sistem-mimarisi.md#6-supabase-veri-modeli)).
- Ücretsiz plan: 500 MB veritabanı, 1 hafta hareketsizlikte proje duraklatılır. Eski telemetri `cleanup_old_telemetry()` ile temizlenebilir.
