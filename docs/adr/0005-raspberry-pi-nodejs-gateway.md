# ADR-0005: Gateway Raspberry Pi üzerinde Node.js + TypeScript ile çalışır

- **Tarih:** 15.10.2026
- **Durum:** Yerini aldı: [ADR-0007](0007-gateway-yeniden-esp32.md) (18.10.2026)

> Bu karar 18.10.2026'da geri alındı: gateway yeniden ESP32 + E32 + Wi-Fi, komutlar yeniden REST sorgulamayla alınıyor. Aşağıdaki metin tarihsel kayıttır; bahsedilen `gateway/` klasörü, systemd birimi ve Pi kurulum adımları artık repoda yok.

## Bağlam
İlk tasarımda gateway, uç düğümlerle aynı donanımdı: ESP32 + E32 + Wi-Fi, C++ ile yazılmış, Supabase'e düz HTTPS (REST) istekleriyle bağlanıyor ve yeni komutları 1.5 s'de bir sorguluyordu ([ADR-0003](0003-gateway-rest-sorgulama.md)). Bu yolun sorunları:

- ESP32 için resmi Supabase istemcisi yok; giriş, token yenileme, REST çağrıları ve JSON elle yazılıyordu.
- Supabase Realtime (WebSocket) ESP32'de pratik değildi, bu yüzden komutlar sürekli sorgulanıyordu.
- TLS sertifika doğrulaması kapalıydı (`setInsecure`), ortadaki adam saldırısına açıktı ([08](../08-guvenlik.md)).
- Gateway mantığını bilgisayarda test etmek zordu; donanım olmadan sadece protokol kütüphanesi test edilebiliyordu.

Gateway tek bir yerde, şebeke elektriği ve internet olan bir ortamda durduğu için düşük güç veya düşük maliyet uç düğümdeki kadar kritik değil.

## Seçenekler
1. **ESP32 gateway'de kalmak** (C++, REST sorgulama), sadece sertifika doğrulamasını eklemek.
2. **Raspberry Pi + Python** (`supabase-py`, `pyserial`).
3. **Raspberry Pi + Node.js 22 + TypeScript** (`supabase-js`, `serialport`).

## Karar
**Seçenek 3: Raspberry Pi (3/4/5 veya Zero 2 W, Raspberry Pi OS Bookworm) + Node.js 22 + TypeScript** (ekip kararı, 15.10.2026). Kod `gateway/` klasöründeydi (18.10.2026'da kaldırıldı).

- E32, Pi'ye 3.3 V lojikli bir USB-UART dönüştürücüyle (`/dev/ttyUSB0`) veya Pi'nin UART'ıyla (`/dev/serial0`) bağlanır; E32 ayrı 5 V kaynaktan beslenir. M0/M1/AUX isteğe bağlı olarak BCM GPIO 23/24/25'e bağlanır ve `pinctrl` ile sürülür; bağlanmazsa M0/M1 GND'de kalır ve modül bir kez EBYTE PC aracıyla ayarlanır ([04 §2.2](../04-donanim.md)).
- Gateway Supabase'e resmi **`supabase-js`** ile bağlanır. Yeni komutlar **Realtime** ile (`commands` tablosunda INSERT) anında gelir; Realtime koparsa diye `status = pending` komutlar 10 s'de bir sorgulanır. REST sorgulaması artık ana yol değil, yedektir.
- Gateway, `app_metadata.role = gateway` olan normal bir kullanıcıyla giriş yapar; `service_role` anahtarı kullanılmaz.
- Protokol TypeScript'e taşındı (`src/protocol/protocol.ts`); C++ kütüphanesiyle bayt bayt uyumlu ve iki taraf aynı test vektörleriyle test edilir ([03](../03-protokol.md)).
- Köprü mantığı (`src/bridge.ts`) G/Ç içermez ve sahte radyo/veritabanıyla test edilir.
- Pi'de systemd servisi olarak çalışır (`deploy/lora-gateway.service`).
- Python yerine TypeScript seçildi, çünkü mobil uygulama da TypeScript ve `supabase-js` kullanıyor: ekip tek bir dil ve aynı Supabase istemcisiyle iki bileşeni yazabiliyor ([ADR-0006](0006-bilesen-dilleri.md)).

## Sonuçlar
- Komut gecikmesinden sorgulama aralığı kalktı; Supabase'e giden istek sayısı azaldı (dakikada ~40 yerine ~6 yedek sorgu + 1 sağlık güncellemesi).
- TLS sertifikaları Node.js tarafından doğrulanır; `setInsecure` açığı kapandı.
- Gateway mantığı donanım olmadan bilgisayarda test edilebiliyor (`cd gateway && npm test`).
- Çerçeve aracı da TypeScript'e taşındı (`npm run frame`); projede Python kalmadı.
- Firmware sadece uç düğümü içerir (PlatformIO ortamları `node`, `node2`, `native`); uç düğümde Wi-Fi/Supabase bilgisi, dolayısıyla `secrets.h` yok.
- Ek maliyet: Raspberry Pi, microSD kart, resmi güç kaynağı ve USB-UART dönüştürücü ([`hardware/bom.csv`](../../hardware/bom.csv)).
- Pi bir işletim sistemi çalıştırdığı için kurulum adımı uzadı (05-kurulum.md'nin o tarihteki §4'ü) ve ani elektrik kesintisinde SD kart bozulma riski var.
- Protokol artık iki dilde var; bir değişiklik iki uygulamada ve iki test grubunda birlikte yapılmalı.
