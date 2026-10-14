# 06 · Test Planı ve Sonuçlar

Her geliştirme aşamasının (bkz. [02 §9](02-sistem-mimarisi.md#9-geliştirme-aşamaları-ve-kabul-kriterleri)) "bitti" sayılması için yapılacak testler. Sonuç tablolarını test yaptıkça doldurun; bitirme raporunun "Deneysel Sonuçlar" bölümü buradan çıkacak.

## 1. Otomatik testler

| Test | Kapsam | Nerede | Durum |
|---|---|---|---|
| Protokol birim testleri (16) | CRC, çerçeve, TLV, komut, tekrar filtresi, çerçeve toplama, ACK/yeniden gönderim | `firmware/test/test_protocol` | ✅ Geçiyor |
| Veritabanı + RLS testleri (9) | Kim neyi okuyabilir/yazabilir, tetikleyiciler | `backend/supabase/tests` | ✅ Geçiyor |
| Python aracı testleri (5) | C++ ile aynı test vektörleri | `tools/test_frame_tool.py` | ✅ Geçiyor |
| Mobil tip kontrolü | TypeScript | `mobile/` | ✅ Geçiyor |
| Firmware derlemesi | `node`, `gateway` ortamları | CI | CI'da çalışır |

## 2. Donanım testleri

### A1 · Radyo bağlantısı (masa testi)

**Amaç:** İki ESP32 + E32 arasında sabit adresli gönderimin çalıştığını göstermek.
**Yöntem:** Gateway açık, düğüm 1 m mesafede, periyot 10 s (`PERIYOT_AYARLA`). 100 telemetri sayılır; gateway seri monitöründeki satırlar ve `telemetry` tablosundaki satır sayısı karşılaştırılır.
**Kabul:** ≥ 95/100 paket ulaşır.

| Tarih | Mesafe | Gönderilen | Ulaşan | Oran | Not |
|---|---|---|---|---|---|
| | 1 m | 100 | | | |

### A2 · Protokol güvenilirliği

| # | Senaryo | Beklenen | Sonuç |
|---|---|---|---|
| 1 | Gateway kapalıyken düğüm telemetri gönderir | 3 deneme, sonra `ACK alinamadi` | |
| 2 | Gateway ACK'ten hemen önce kapatılıp açılır | Tekrar gelen paket tabloya **ikinci kez yazılmaz** | |
| 3 | Bir çerçevenin baytı elle bozulur (frame_tool ile) | `cerceve hatasi: bad_crc`, işlenmez | |
| 4 | Aynı komut iki kez gönderilir (aynı cmdId) | Vana bir kez değişir, aynı sonuç iki kez döner | |

### A3 · Simüle saha cihazı

| # | Senaryo | Beklenen | Sonuç |
|---|---|---|---|
| 1 | Potansiyometre en düşük / en yüksek | `level_mm` ≈ 0 / ≈ 3000 | |
| 2 | Seviye 2800 mm üstü | `alarms` bit0 = 1, uygulamada "Taşma" | |

### A4 – A6 · Uçtan uca gecikme

**Yöntem:** Uygulamada "Vana aç"a basılır; `commands.created_at` ile `updated_at` (status = `done`) farkı ölçülür. 20 tekrar.

| Ölçüm | Ortalama | En kısa | En uzun |
|---|---|---|---|
| Komut → `done` (s) | | | |
| Seviye değişimi → uygulamada görünme (s) | | | |

Not: gateway komutları 1.5 s'de bir sorguladığı için komut gecikmesinin ~0.75 s'si sorgulama aralığından gelir.

### A7 · Saha / menzil testi

**Yöntem:** Gateway sabit, düğüm farklı mesafelere taşınır. Her noktada 50 telemetri. Güç seviyesi ve anten kayda geçirilir. **Önce [yasal uyarıyı](04-donanim.md#4-yasal-uyarı-çıkış-gücü-ve-frekans) okuyun.**

| Mesafe | Ortam (açık alan / bina içi) | Güç seviyesi | Hava hızı | Ulaşan / 50 | Ortalama deneme sayısı |
|---|---|---|---|---|---|
| | | Min | 2.4k | | |

### A8 · Gerçek cihaz entegrasyonu

Danışmanla cihaz belirlendikten sonra doldurulacak.
