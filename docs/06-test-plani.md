# 06 · Test Planı ve Sonuçlar

Her geliştirme aşamasının (bkz. [02 §9](02-sistem-mimarisi.md#9-geliştirme-aşamaları-ve-kabul-kriterleri)) "bitti" sayılması için yapılacak testler. Sonuç tablolarını test yaptıkça doldurun; bitirme raporunun "Deneysel Sonuçlar" bölümü buradan çıkacak.

## 1. Otomatik testler

Hepsi her gönderimde [CI](../.github/workflows/ci.yml) ile çalışır. Komutlar: [05 §8](05-kurulum.md#8-testler-ve-iş-takibi-geliştiriciler-için).

| Test | Kapsam | Nerede | CI işi | Durum |
|---|---|---|---|---|
| Protokol, C++ (16) | CRC, çerçeve, TLV, komut, tekrar filtresi, çerçeve toplama, ACK/yeniden gönderim | `firmware/test/test_protocol` | firmware | ✅ Geçiyor |
| Modbus RTU, C++ (11) | CRC-16/MODBUS, 0x03 / 0x06 istek ve cevap çerçeveleri, hatalı argüman, bozuk CRC, istisna cevabı, çerçeve tamamlanma, t3.5 bekleme süresi | `firmware/test/test_modbus` | firmware | ✅ Geçiyor |
| Firmware derlemesi | `node`, `node2`, `node-modbus`, `gateway` (örnek `secrets.h` ile) | `firmware/` | firmware | ✅ Geçiyor |
| Araçlar, TypeScript (19) | Protokolün TS karşılığı (C++ ile **aynı test vektörleri**, [03 §7](03-protokol.md#7-örnek-konuşma-bayt-bayt) dahil), çerçeve aracı + tip kontrolü | `tools/test` | tools | ✅ Geçiyor |
| Veritabanı (2 dosya, 35 kontrol) | `rls_test.sql`: kim neyi okur/yazar, tetikleyiciler. `roles_alarms_test.sql`: roller, son yönetici koruması, eşik yetkisi, alarm açma/kapama, sessiz cihaz, push token'ları | `backend/supabase/tests` | database | ✅ Geçiyor |
| Edge Function tip kontrolü | `deno check` | `backend/supabase/functions` | edge-functions | ✅ Geçiyor |
| Mobil tip kontrolü | TypeScript | `mobile/` | mobile | ✅ Geçiyor |

## 2. Donanım testleri

### A1 · Radyo bağlantısı (masa testi)

**Amaç:** İki ESP32 + E32-433T20D (düğüm ve gateway) arasında sabit adresli gönderimin çalıştığını göstermek.
**Yöntem:** Gateway açık, düğüm 1 m mesafede, güç `Min` (10 dBm), periyot 10 s (`PERIYOT_AYARLA`). 100 telemetri sayılır; düğümün seri monitöründeki gönderimler ile `telemetry` tablosundaki satır sayısı karşılaştırılır.
**Kabul:** ≥ 95/100 paket ulaşır.

| Tarih | Mesafe | Gönderilen | Ulaşan | Oran | Not |
|---|---|---|---|---|---|
| | 1 m | 100 | | | |

### A2 · Protokol güvenilirliği

| # | Senaryo | Beklenen | Sonuç |
|---|---|---|---|
| 1 | Gateway kapalıyken düğüm telemetri gönderir | 3 deneme, sonra `no ACK received` | |
| 2 | Gateway ACK'ten hemen önce kapatılıp açılır | Tekrar gelen paket tabloya **ikinci kez yazılmaz** | |
| 3 | Bir çerçevenin baytı elle bozulur (`cd tools && npm run frame` ile üretilip bir baytı değiştirilir) | Seri monitörde `frame error: bad_crc`, işlenmez | |
| 4 | Aynı komut iki kez gönderilir (aynı cmdId) | Vana bir kez değişir, aynı sonuç iki kez döner | |
| 5 | Gateway çalışırken Wi-Fi kesilip geri gelir | Gateway yeniden bağlanır; arada eklenen `pending` komutlar bağlantı gelince 1,5 s içinde gönderilir | |
| 6 | Gateway'in güç kablosu çekilip takılır | Açılışta Wi-Fi → NTP → giriş sırası tamamlanır, bekleyen komutlar gönderilir | |

### A3 · Saha cihazı adaptörleri

| # | Senaryo | Beklenen | Sonuç |
|---|---|---|---|
| 1 | (`node`) Potansiyometre en düşük / en yüksek | `level_mm` ≈ 0 / ≈ 3000 | |
| 2 | (`node`) Seviye 2800 mm üstü | `alarms` bit0 = 1, uygulamada "Taşma" alarmı | |
| 3 | (`node-modbus`) PC simülatöründe register 0 = 1500, 1 = 1 | Telemetride seviye 1500 mm, vana açık | |
| 4 | (`node-modbus`) Uygulamadan "Vana kapat" | Simülatörde register 10 = 0, komut `done` | |
| 5 | (`node-modbus`) Vana zaten kapalıyken "Vana kapat" | Yazma yapılmaz, sonuç `ALREADY_IN_STATE` (`done`) | |
| 6 | (`node-modbus`) `CIHAZ_ETKIN` 0, sonra "Vana aç" | Register 11 = 0; vana komutu `DEVICE_ERROR` (`failed`) | |
| 7 | (`node-modbus`) Simülatör durdurulur / A-B kablosu çıkarılır | Komut `DEVICE_ERROR`; telemetride `SensorFault`, uygulamada "Sensör hatası" alarmı | |

### A4 · Gateway → Supabase ve TLS

| # | Senaryo | Beklenen | Sonuç |
|---|---|---|---|
| 1 | Normal açılış | `clock set` → `signed in to Supabase` → `loaded N devices` | |
| 2 | Seviye değişimi | 5 s içinde `telemetry` tablosunda | |
| 3 | Sağlık kaydı | `gateways.last_seen` ve `wifi_rssi` dakikada bir yenileniyor | |
| 4 | `root_ca.h` içindeki kökler geçici olarak boşaltılıp derlenir | TLS el sıkışması başarısız, hiçbir istek gitmez (doğrulamanın gerçekten yapıldığını gösterir) | |
| 5 | NTP engelli ağ | `NTP sync failed, will retry`; saat ayarlanmadan istek gönderilmez | |
| 6 | `secrets.h`'de gateway rolü olmayan bir kullanıcı | Telemetri eklemesi RLS ile reddedilir | |

### A5 – A6 · Uçtan uca gecikme

**Yöntem:** Uygulamada "Vana aç"a basılır; `commands.created_at` ile `updated_at` (status = `done`) farkı ölçülür. 20 tekrar. Gateway 1,5 s'de bir sorguladığı için ortalama ~0,75 s sorgulama gecikmesi beklenir (çıkarım).

| Ölçüm | Ortalama | En kısa | En uzun |
|---|---|---|---|
| Komut → `done` (s) | | | |
| Seviye değişimi → uygulamada görünme (s) | | | |
| Eşik aşımı → telefonda push (s) | | | |

### A6 · Uygulama kabul senaryoları

| # | Senaryo | Beklenen | Sonuç |
|---|---|---|---|
| 1 | **Roller:** İzleyici olarak giriş | Komut butonları ve eşik ayarları görünmüyor; REST ile komut ekleme denemesi RLS hatası verir | |
| 2 | Operatör olarak giriş | Komut gönderebiliyor; eşik ayarları ve "Kullanıcılar" görünmüyor | |
| 3 | Yönetici bir kullanıcıyı operatör yapar | Kullanıcı yeniden açınca komut butonlarını görüyor | |
| 4 | Tek yönetici kendi rolünü düşürmeye çalışır | "En az bir yönetici kalmalıdır." hatası | |
| 5 | **Eşikler:** Yönetici `level_low_mm` = 500 yapar | Değer kaydedilir; grafikte kesikli eşik çizgisi | |
| 6 | **Alarmlar:** Seviye eşiğin altına iner | "Alarmlar" listesinde açık alarm, cihazda rozet | |
| 7 | Seviye normale döner | Alarm kapanır (`resolved_at` dolar), rozet kalkar | |
| 8 | Düğümün fişi çekilir | `offline_after_s` (600 s) + en fazla 1 dk sonra `offline` alarmı; düğüm geri gelince kapanır | |
| 9 | **Push:** APK ile giriş, eşik aşımı | Telefona "Alarm: <cihaz adı>" bildirimi; dokununca cihaz detayı açılır | |
| 10 | Android'de Expo Go ile giriş | Push kaydı atlanır (`[push] skipped`), uygulama çalışmaya devam eder | |
| 11 | Çıkış yapılır | Telefonun token'ı `push_tokens`'tan silinir, bildirim gelmez | |
| 12 | **Grafik:** 24 saat / 7 gün geçişi | Seviye ve basınç grafikleri, veri olmayan aralıklar boşluk | |

### A7 · Saha / menzil testi

**Yöntem:** Gateway sabit, düğüm farklı mesafelere taşınır. Her noktada 50 telemetri. Güç kademesi ve anten kayda geçirilir. **Önce [yasal uyarıyı](04-donanim.md#4-yasal-uyarı-çıkış-gücü-ve-frekans) okuyun.**

| Mesafe | Ortam (açık alan / bina içi) | Güç | Hava hızı | Ulaşan / 50 | Ortalama deneme sayısı |
|---|---|---|---|---|---|
| | | Min (10 dBm) | 2,4k | | |

### A8 · Gerçek kontrol cihazı entegrasyonu

Register listesi danışmandan veya cihazın kılavuzundan alındıktan sonra `modbus_map.h` güncellenir ve A3 senaryo 3–7 gerçek cihazla tekrarlanır.

| Tarih | Cihaz | Okunan register | Yazılan register | Sonuç |
|---|---|---|---|---|
| | | | | |

## 3. Çerçeve aracıyla elle test

Çerçeve aracı protokol çerçevelerini üretmek ve çözmek için kullanılır (A2 senaryo 3, hata ayıklama, rapordaki örnekler):

```bash
cd tools
npm ci
npm run frame -- decode "01 01 10 01 2A 07 01 02 04 E2 02 01 01 45 CB"
npm run frame -- telemetry --src 0x1001 --seq 1 --level 1250 --valve 1
npm run frame -- command --seq 7 --cmd-id 15 --op VANA_AC
```

Araç TypeScript `protocol.ts` kodunu çağırır; çıktısı [03 §7](03-protokol.md#7-örnek-konuşma-bayt-bayt) tablosundaki baytlarla aynı olmalıdır (bu, `tools/test` içinde de otomatik kontrol edilir).
