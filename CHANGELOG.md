# Değişiklik Günlüğü

Biçim: [Keep a Changelog](https://keepachangelog.com/tr-TR/1.1.0/). Sürümler: [SemVer](https://semver.org/lang/tr/).

## [Yayınlanmadı]

### Değişti (2026-10-18)
- Proje başlığı danışman onayıyla "LoRaWAN Tabanlı Mobil Su Yönetim Sisteminin Geliştirilmesi" oldu
- Gateway yeniden **ESP32 + E32** (Wi-Fi) oldu; Raspberry Pi + Node.js gateway kaldırıldı ([ADR-0007](docs/adr/0007-gateway-yeniden-esp32.md))
- Gateway TLS sertifikasını doğruluyor (`setInsecure()` kaldırıldı)
- Radyo modülü E32-433T20D (100 mW, 10 dBm'ye inebiliyor)
- TypeScript protokol kodu ve çerçeve aracı `tools/` klasörüne taşındı

### Eklendi (2026-10-18)
- Modbus RTU (RS-485) saha cihazı adaptörü ve birim testleri ([ADR-0008](docs/adr/0008-saha-cihazi-modbus.md))
- Kullanıcı rolleri (yönetici, operatör, izleyici), eşik ayarları, alarmlar ve push bildirimi (Edge Function)
- Mobil uygulamada geçmiş grafiği, alarm listesi, eşik ayarları ve kullanıcı yönetimi; EAS ile Android APK
- `docs/09-teknoloji-kararlari.md` ve `scripts/github-issues.sh`

### Değişti (2026-10-15)
- Gateway, ESP32 yerine **Raspberry Pi + Node.js (TypeScript)** ile yeniden yazıldı (`gateway/`). Komutlar artık Supabase Realtime ile anında alınıyor; sorgulama yedek olarak kaldı ([ADR-0005](docs/adr/0005-raspberry-pi-nodejs-gateway.md))
- Protokol TypeScript'e taşındı; C++ ile aynı test vektörlerinden geçiyor
- Çerçeve aracı Python'dan TypeScript'e taşındı: `cd gateway && npm run frame -- decode "..."`
- Kod yorumları İngilizceye çevrildi; README Türkçe + İngilizce oldu ([ADR-0006](docs/adr/0006-bilesen-dilleri.md))

### Kaldırıldı
- ESP32 gateway firmware'i (`firmware/src/gateway`), `secrets.h` ve `tools/frame_tool.py`

### Eklendi
- Mimari araştırması, sistem mimarisi, protokol, donanım, kurulum, test planı ve güvenlik dokümanları
- Mimari karar kayıtları (ADR 0001–0006)
- `LoraProtocol` kütüphanesi (çerçeve, CRC16, TLV, tekrar filtresi, ACK/yeniden gönderim) ve 16 birim testi
- E32 sürücüsü, simüle saha cihazı, uç düğüm ve gateway firmware'i
- Supabase şeması, tetikleyiciler, RLS kuralları ve testleri
- React Native (Expo) mobil uygulama: giriş, cihaz listesi, cihaz detayı ve komut gönderme
- `tools/frame_tool.py` çerçeve hata ayıklama aracı
- GitHub Actions CI
