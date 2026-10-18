# ADR-0003: Gateway komutları REST ile sorgular

- **Tarih:** 14.10.2026
- **Durum:** Kabul edildi (ADR-0007 ile yeniden; 15.10.2026'da ADR-0005 ile yerini almıştı)

> 15–18.10.2026 arasında gateway Raspberry Pi + Node.js üzerindeydi ve komutları Realtime ile alıyordu ([ADR-0005](0005-raspberry-pi-nodejs-gateway.md)). 18.10.2026'da gateway yeniden ESP32 oldu ([ADR-0007](0007-gateway-yeniden-esp32.md)) ve bu karar tekrar geçerli.

## Bağlam
Gateway'in, uygulamanın eklediği komutları öğrenmesi gerekiyor. Supabase Realtime WebSocket kullanır; ESP32 için resmi istemci yok, topluluk kütüphanesi (ESPSupabase) v0.1.0 sürümünde.

## Seçenekler
1. Realtime (WebSocket) ile anlık bildirim
2. REST ile periyodik sorgulama

## Karar
**REST ile 1.5 s'de bir sorgulama** (`GET /rest/v1/commands?status=eq.pending`).

## Sonuçlar
- Basit, hata ayıklaması kolay, sadece ESP32'nin standart `HTTPClient`'ı gerekiyor.
- Komut gecikmesine ortalama ~0.75 s eklenir; vana kontrolü için kabul edilebilir.
- Supabase'e dakikada ~40 istek gider; ücretsiz plan için sorun değil.
- İleride Realtime eklenirse sadece gateway'in `pollCommands()` kısmı değişir.
