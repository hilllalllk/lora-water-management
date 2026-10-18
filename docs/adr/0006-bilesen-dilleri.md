# ADR-0006: Bileşen başına programlama dili ve doküman dili

- **Tarih:** 15.10.2026
- **Durum:** Kabul edildi (18.10.2026: dil tablosu güncellendi)

## Bağlam
Proje dört bileşenden oluşuyor: uç düğüm, gateway, bulut ve mobil uygulama. İlk iskelette uç düğüm ve gateway C++, mobil TypeScript, yardımcı çerçeve aracı Python ile yazılmıştı. Ekip her bileşen için dili açıkça seçmek, gereksiz dilleri azaltmak ve dokümanlarla koddaki dil kullanımını (Türkçe / İngilizce) tek bir kurala bağlamak istedi.

## Seçenekler
1. Her bileşende en yaygın araç (ör. gateway Python, araçlar Python).
2. Mümkün olduğunca az dil: donanıma yakın yerde C++, geri kalan her yerde TypeScript.

## Karar
**Seçenek 2** (ekip kararı, 15.10.2026; 18.10.2026'da gateway C++'a döndüğü için tablo güncellendi, bkz. [ADR-0007](0007-gateway-yeniden-esp32.md)).

| Bileşen | Platform | Dil | Klasör |
|---|---|---|---|
| Uç düğüm | ESP32 DevKit V1 + E32-433T20D | C++ (Arduino, PlatformIO) | `firmware/src/node` |
| Gateway | ESP32 DevKit V1 + E32-433T20D, Wi-Fi | C++ (Arduino, PlatformIO) | `firmware/src/gateway` |
| Protokol, Modbus, E32, saha cihazı kütüphaneleri | ESP32 / bilgisayar (testler) | C++ | `firmware/lib` |
| Veritabanı | Supabase (PostgreSQL) | SQL (tablolar, RLS, tetikleyiciler, pg_cron) | `backend/supabase/migrations` |
| Alarm bildirimi | Supabase Edge Function (Deno) | TypeScript | `backend/supabase/functions` |
| Mobil uygulama | Android / iOS | TypeScript (React Native, Expo) | `mobile/` |
| Protokolün ikinci uygulaması ve çerçeve aracı | Bilgisayar (Node.js 22) | TypeScript | `tools/` |

Python projede yok.

**Doküman ve kod dili kuralı:**

| Ne | Dil |
|---|---|
| `README.md` | Türkçe; İngilizce karşılığı `README.en.md` |
| `docs/` altındaki ayrıntılı dokümanlar ve ADR'ler | Türkçe |
| Kod yorumları, log mesajları, bileşen README'leri (ör. `firmware/README.md`) | İngilizce |
| Mobil uygulama arayüzü ve alarm mesajları | Türkçe |

## Sonuçlar
- Donanıma yakın her şey (düğüm, gateway) tek dilde ve aynı kütüphanelerle yazılır; geri kalan (mobil, Edge Function, araçlar) TypeScript'tir.
- Protokol iki dilde var (C++ ve TypeScript); ikisi aynı test vektörleriyle test edilir ([03](../03-protokol.md)).
- Geliştirme ortamı için VS Code + PlatformIO ve Node.js 22 yeterli; Python kurmak gerekmiyor. Edge Function tip kontrolü için CI'da Deno kullanılır.
- Bitirme raporu ve `docs/` Türkçe kalır; kodu okuyacak dış katkıcılar için yorumlar İngilizcedir.
