# ADR-0007: Gateway yeniden ESP32 + E32 + Wi-Fi

- **Tarih:** 18.10.2026
- **Durum:** Kabul edildi (ADR-0005'in yerini aldı; ADR-0003'ü yeniden geçerli kıldı)

## Bağlam
15.10.2026'da gateway Raspberry Pi + Node.js + TypeScript'e taşınmıştı ([ADR-0005](0005-raspberry-pi-nodejs-gateway.md)). 18.10.2026'da ekip ve danışman (Erkan Duman) kapsamı netleştirdi: görev, sahadaki mevcut kontrol cihazlarıyla LoRa üzerinden konuşan altyapıyı kurmak. Bu çerçevede Pi gateway'in getirdikleri tekrar değerlendirildi:

- Ek donanım: Pi, microSD kart, resmi güç kaynağı, USB-UART dönüştürücü, E32 için ayrı 5 V kaynak.
- İşletim sistemi kurulumu, systemd, SD kart bozulma riski.
- Gateway için ikinci bir dil ve ikinci bir protokol uygulaması (TypeScript).
- ESP32 gateway'e karşı ileri sürülen ana gerekçe olan doğrulanmayan TLS (`setInsecure`) kök sertifikaları koda gömülerek çözülebiliyordu.

## Seçenekler
1. **Raspberry Pi + Node.js'te kalmak** ([ADR-0005](0005-raspberry-pi-nodejs-gateway.md)).
2. **ESP32 + E32-433T20D + Wi-Fi**, C++; uç düğümle aynı donanım ve kütüphaneler; REST sorgulama; doğrulanan TLS.

## Karar
**Seçenek 2** (ekip ve danışman görüşü, 18.10.2026).

- Donanım: ESP32 DevKit V1 + E32-433T20D; E32 bağlantısı uç düğümle aynı ([04 §2.2](../04-donanim.md#22-gateway-esp32--e32)). İnternet: 2,4 GHz Wi-Fi.
- Kod: `firmware/src/gateway` (C++, Arduino), PlatformIO ortamı `gateway`. Protokol ve E32 sürücüsü düğümle ortak (`firmware/lib`).
- Komutlar REST ile **1,5 s**'de bir sorgulanır (`commands?status=eq.pending`), yani [ADR-0003](0003-gateway-rest-sorgulama.md) yeniden geçerlidir. ESP32 için Supabase Realtime kütüphaneleri olgun değil.
- Gateway telemetri ekler, komut durumunu günceller (`sent → delivered → done/failed`), HEARTBEAT'te `devices.last_seen` / `fw` yazar, 60 s'de bir `gateways` satırını (`last_seen`, `fw`, `wifi_rssi`) upsert eder, cihaz listesini 5 dakikada bir yeniler.
- Supabase'e `app_metadata.role = 'gateway'` olan normal bir kullanıcıyla girer; `service_role` kullanılmaz.
- **TLS doğrulanır:** `src/gateway/root_ca.h` ISRG Root X1/X2 ve GTS Root R1/R4 köklerini içerir, `setCACert` ile kullanılır. Doğrulama için saat, ilk istekten önce NTP ile ayarlanır.
- Wi-Fi ve Supabase bilgileri `firmware/include/secrets.h` dosyasındadır (`secrets.example.h`'den kopyalanır, `.gitignore`'da).

## Sonuçlar
- Tek platform ve tek araç zinciri (VS Code + PlatformIO); ek donanım ve işletim sistemi bakımı yok. Malzeme listesi: 3× ESP32 + 3× E32 ([`hardware/bom.csv`](../../hardware/bom.csv)).
- `gateway/` klasörü, systemd birimi ve Pi kurulum adımları kaldırıldı. TypeScript protokol uygulaması ve çerçeve aracı `tools/` klasörüne taşındı; C++ ile aynı test vektörlerinden geçmeye devam eder.
- Komut gecikmesine ortalama ~0,75 s sorgulama gecikmesi eklenir; vana kontrolü için kabul edilebilir. Supabase'e dakikada ~40 sorgu gider; ücretsiz plan için sorun değil.
- Supabase'in sertifika zinciri başka bir köke geçerse gateway bağlanamaz; yeni kök `root_ca.h`'ye eklenip yeniden yüklenir.
- Gateway mantığının birim testi yok (sadece protokol ve Modbus kütüphaneleri bilgisayarda test edilir); gateway davranışı donanım kabul testleriyle doğrulanır ([06 §2](../06-test-plani.md#a4--gateway--supabase-ve-tls)).
- Gateway kullanıcısının şifresi ESP32 flash'ında düz metin durur ([08](../08-guvenlik.md)).
