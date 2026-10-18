# 07 · Geliştirme Günlüğü

Her çalışma oturumundan sonra kısa bir kayıt ekleyin. Bitirme raporunda "proje süreci" bölümü ve danışman görüşmeleri için kullanılır. En yeni kayıt en üstte.

## Şablon

```markdown
## GG.AA.YYYY – Kısa başlık
**Kim:** ...
**Ne yapıldı:** ...
**Sorunlar / öğrenilenler:** ...
**Sonraki adım:** ...
```

---

## 18.10.2026 – Danışman yönlendirmesi, gateway yeniden ESP32, Modbus, roller ve alarmlar

**Danışman e-postası (Erkan Duman):**
- Başlık "LoRaWAN Tabanlı Mobil Su Yönetim Sisteminin Geliştirilmesi" olarak onaylandı; "LoRaWAN" ifadesi artık açık soru değil.
- Vanalarda, depolarda ve terfi merkezlerinde dijital kontrol cihazları zaten var; bu cihazlardan LoRa ile veri okunup yazılacak. Görevimiz altyapı; iletişim kurulacak cihaz kısmı esnek.

**Ne yapıldı / karar verildi** (hepsi: [09](09-teknoloji-kararlari.md)):
- Gateway Raspberry Pi + Node.js'ten yeniden **ESP32 DevKit V1 + E32 + Wi-Fi**'ye döndü (C++, `firmware/src/gateway`, ortam `gateway`) ([ADR-0007](adr/0007-gateway-yeniden-esp32.md), ADR-0005'in yerini aldı). `gateway/` klasörü, systemd birimi ve Pi kurulum adımları kaldırıldı.
- Komutlar yeniden REST ile **1,5 s**'de bir sorgulanıyor ([ADR-0003](adr/0003-gateway-rest-sorgulama.md) tekrar geçerli). Gateway telemetri ekliyor, komut durumunu güncelliyor, 60 s'de bir `gateways` satırını (`wifi_rssi` dahil) upsert ediyor.
- Gateway TLS'i artık **doğrulanıyor**: `root_ca.h` ISRG Root X1/X2 ve GTS Root R1/R4 köklerini içeriyor (`setCACert`); TLS'ten önce NTP ile saat ayarlanıyor. Wi-Fi ve Supabase bilgileri `firmware/include/secrets.h`'de (Git'e girmiyor).
- Radyo modülü **E32-433T20D** (100 mW, kademeler 20/17/14/10 dBm, varsayılan `Min` ≈ 10 mW, ~130 mA). Ayrı 1 A güç kaynağı gerekmiyor; düğümler USB adaptörle besleniyor.
- Sensör / vana satın alınmayacak. Saha cihazı adaptörleri: `SimulatedDevice` (`node`, `node2`) ve **`ModbusRtuDevice`** (`node-modbus`, 0x1003, MAX3485 ile RS-485, 9600 8N1, register haritası `modbus_map.h`). Bağımlılıksız `ModbusRtu` kütüphanesi ve 11 birim testi ([ADR-0008](adr/0008-saha-cihazi-modbus.md)). Test için PC Modbus slave simülatörü + USB-RS485 çevirici.
- TypeScript protokol ve çerçeve aracı `tools/` klasörüne taşındı (19 test, `cd tools && npm run frame -- ...`).
- Supabase: ücretsiz plan, Frankfurt. Yeni migration `20261018000000_roles_alarms.sql`: roller (`profiles`, yönetici/operatör/izleyici; komutu sadece operatör ve yönetici ekler), cihaz eşikleri, `alerts` tablosu ve tetikleyicileri (Türkçe mesajlar), pg_cron ile dakikada bir `check_offline_devices()`, `push_tokens` + `register_push_token()`. Edge Function `send-alert-push` (Expo Push, `alerts` INSERT webhook'u). Veritabanı testleri 2 dosyada 35 kontrol.
- Mobil: 24 saat / 7 gün geçmiş grafiği, alarm listesi + rozet + push, roller (izleyici komut butonu görmez), yönetici için eşik ve kullanıcı ekranları. Android APK EAS ile (`npm run build:apk`), iPhone'da Expo Go; mağaza yayını yok.
- Geliştirme ortamı: VS Code + PlatformIO, Windows desteği ([05 §6](05-kurulum.md#6-windows)). İş takibi GitHub Projects + Issues (`bash scripts/github-issues.sh`).
- CI işleri: firmware (native testler + `node`, `node2`, `node-modbus`, `gateway` derlemesi), tools, database, edge-functions, mobile.
- Aşamalar yeniden tanımlandı: A3 saha cihazı adaptörleri, A6 roller/grafik/alarm/eşikler, A8 gerçek kontrol cihazı entegrasyonu ([02 §9](02-sistem-mimarisi.md#9-geliştirme-aşamaları-ve-kabul-kriterleri)).
- Malzeme listesi güncellendi: 3× ESP32, 3× E32-433T20D, 2× MAX3485, USB-RS485, ~3.640 TL ([04](04-donanim.md), [`hardware/bom.csv`](../hardware/bom.csv)).

**Sorunlar / öğrenilenler:**
- Raspberry Pi gateway güç kaynağı, SD kart, işletim sistemi kurulumu ve ikinci bir dil getiriyordu; danışman yönlendirmesinden sonra tek platformda kalmak daha basit.
- E32-433T30D en düşük kademede bile 10 mW'ın üstünde; T20D ile yasal sınıra inmek mümkün (BTK teyidi hâlâ açık).
- Android'de Expo Go push bildirimini desteklemiyor; push APK ile test edilmeli.

**Sonraki adım:** A1 masa testi (iki ESP32 + E32-433T20D), gerçek cihazın register listesinin danışmandan istenmesi.

---

## 15.10.2026 – Bileşen dilleri ve Raspberry Pi gateway

**Ne yapıldı:**
- Ekip her bileşen için dil seçti ([ADR-0006](adr/0006-bilesen-dilleri.md)): uç düğüm C++ (Arduino/PlatformIO), gateway TypeScript (Node.js 22), bulut Supabase (SQL), mobil React Native (Expo) + TypeScript.
- Gateway ESP32'den **Raspberry Pi + Node.js + TypeScript**'e taşındı ([ADR-0005](adr/0005-raspberry-pi-nodejs-gateway.md)). E32, Pi'ye USB-UART dönüştürücüyle (`/dev/ttyUSB0`) veya Pi'nin UART'ıyla (`/dev/serial0`) bağlanıyor; M0/M1/AUX isteğe bağlı olarak BCM GPIO 23/24/25'e (`pinctrl`).
- Yeni `gateway/` klasörü: protokolün TypeScript karşılığı (`protocol.ts`, C++ ile aynı test vektörleri), G/Ç içermeyen köprü mantığı (`bridge.ts`), E32 sürücüsü, `supabase-js` ile veri katmanı, systemd birimi. 29 birim testi (`npm test`).
- Komutlar artık REST sorgulaması yerine **Supabase Realtime** ile alınıyor; 10 s'lik sorgulama yedek olarak kaldı ([ADR-0003](adr/0003-gateway-rest-sorgulama.md) yerini ADR-0005'e bıraktı). Gateway sağlığı 60 s'de bir, cihaz listesi 5 dakikada bir güncelleniyor.
- Firmware'den gateway kodu çıkarıldı; PlatformIO ortamları `node`, `node2`, `native`. Uç düğüm internete bağlanmadığı için `secrets.h` kaldırıldı.
- Python ile yazılmış `tools/frame_tool.py` kaldırıldı; yerine `cd gateway && npm run frame -- ...` geldi. Projede Python kalmadı.
- TLS sertifikaları artık Node.js tarafından doğrulanıyor; eski ESP32 gateway'deki `setInsecure` açığı kapandı ([08](08-guvenlik.md)).
- Dokümantasyon dili kuralı: README iki dilli (Türkçe + İngilizce), `docs/` Türkçe, kod yorumları İngilizce, uygulama arayüzü Türkçe.
- Donanım listesi ve bağlantı şemaları Raspberry Pi'ye göre güncellendi ([04](04-donanim.md), [`hardware/bom.csv`](../hardware/bom.csv)).

**Sorunlar / öğrenilenler:**
- ESP32'de resmi Supabase istemcisi olmadığı için REST sorgulama ve sertifika doğrulaması elle yazılmak zorundaydı; Node.js'te resmi `supabase-js` bu işleri hazır veriyor.
- Pi'nin GPIO'ları 3.3 V; USB-UART dönüştürücü 3.3 V lojikte olmalı ve E32 ayrı 5 V kaynaktan beslenmeli.

**Sonraki adım:** Raspberry Pi kurulumu ([05 §4](05-kurulum.md)) ve A1 masa testi.

---

## 14.10.2026 – Proje başlangıcı, mimari ve repo iskeleti

**Ne yapıldı:**
- Benzer LoRa/LoRaWAN projeleri araştırıldı ([01](01-mimari-arastirmasi.md)).
- Düz LoRa (E32) ile ilerleme kararı alındı ([ADR-0001](adr/0001-duz-lora.md)).
- Bulut olarak Firebase yerine Supabase seçildi ([ADR-0002](adr/0002-supabase.md)).
- Sistem mimarisi ve LoRa protokolü tasarlandı ([02](02-sistem-mimarisi.md), [03](03-protokol.md)).
- Repo iskeleti kuruldu: protokol kütüphanesi ve testleri, düğüm/gateway firmware'i, Supabase şeması ve RLS testleri, mobil uygulama, CI.

**Açık konular:**
- Başlıktaki "LoRaWAN" ifadesi ve E32 çıkış gücü danışmanla konuşulacak ([danışman soruları](danisman-sorulari.md)).
- Firmware henüz gerçek kartta denenmedi (A1).

**Sonraki adım:** A1 masa testi.
