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
- ESP32 tarafı doğrudan REST ile yazıldı ([ADR-0003](0003-gateway-rest-sorgulama.md)).
- Ücretsiz plan: 500 MB veritabanı, 1 hafta hareketsizlikte proje duraklatılır. Eski telemetri `cleanup_old_telemetry()` ile temizlenebilir.
