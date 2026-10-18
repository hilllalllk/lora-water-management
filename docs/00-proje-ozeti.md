# 00 · Proje Özeti

**LoRaWAN Tabanlı Mobil Su Yönetim Sisteminin Geliştirilmesi** (başlık danışman Erkan Duman tarafından 18.10.2026'da onaylandı)

> Kaynak: Ekibin "LoRaWAN Tabanlı Mobil Su Yönetim Sistemi – Ön Tasarım Dokümanı" ve danışmanın 18.10.2026 yönlendirmesi. Bütün güncel kararlar: [09 · Teknoloji ve Donanım Kararları](09-teknoloji-kararlari.md).

## Amaç

Vanalarda, depolarda ve terfi merkezlerinde **zaten bulunan dijital kontrol cihazları** ile uzak bir merkez arasında LoRa tabanlı, **çift yönlü** bir haberleşme altyapısı kurmak. Sistem sadece izleme yapmaz; mobil uygulamadan sahadaki cihaza komut da gönderir.

Projenin amacı vana, pompa veya sensör üretmek ya da satın almak değil, **sahadaki cihazlarla konuşabilen esnek bir iletişim altyapısı** geliştirmektir. Danışmanın ifadesiyle iletişim kurulacak cihaz kısmı esnektir; saha cihazı değişse bile haberleşme mimarisi aynı kalır.

## Veri akışı

```
Mevcut kontrol cihazı ⇄ (Modbus RTU) ⇄ ESP32 + E32 (uç düğüm) ⇄ (LoRa 433 MHz) ⇄ ESP32 + E32 gateway ⇄ (Wi-Fi, HTTPS) ⇄ Supabase ⇄ Mobil uygulama
```

## Sistem ne yapacak?

- Sahadaki kontrol cihazından seviye, vana durumu, basınç, debi ve alarm bilgisini okuyacak.
- Verileri LoRa ile gateway'e, gateway de Wi-Fi üzerinden Supabase'e aktaracak.
- Mobil uygulama güncel durumu, geçmiş grafiklerini ve alarmları gösterecek; eşik aşılınca ya da cihaz sessiz kalınca telefona bildirim gidecek.
- Yetkili kullanıcının (operatör, yönetici) verdiği aç/kapat komutları ters yönde sahadaki cihaza iletilecek.

## Prototip

Sensör veya vana satın alınmaz. Düğüm, saha cihazıyla bir adaptör arayüzü üzerinden konuşur:

- **Simüle cihaz** (`node`, `node2`): LED vanayı, potansiyometre depo seviyesini temsil eder.
- **Modbus RTU cihaz** (`node-modbus`): RS-485 (MAX3485) üzerinden mevcut kontrol cihazının register'larını okur ve yazar. Gerçek cihaz gelene kadar bilgisayarda çalışan bir Modbus slave simülatörüyle test edilir.

## Ön dokümandan bu yana değişenler

| Konu | Ön doküman | Güncel karar |
|---|---|---|
| Başlık | "LoRaWAN Tabanlı Mobil Su Yönetim Sistemi" | "LoRaWAN Tabanlı Mobil Su Yönetim Sisteminin Geliştirilmesi" (danışman onayı, 18.10.2026). Teknik olarak düz LoRa (E32) kullanılır, LoRaWAN ağ sunucusu yok ([ADR-0001](adr/0001-duz-lora.md)) |
| Radyo modülü | E32-433T30D (~1 W) | E32-433T20D (100 mW, en düşük kademe 10 dBm ≈ 10 mW) |
| Saha cihazı | Simülasyon, ileride gerçek cihaz | Simüle cihaz + Modbus RTU adaptörü ([ADR-0008](adr/0008-saha-cihazi-modbus.md)) |
| Bulut | Firebase / Cloud Firestore | Supabase, ücretsiz plan, Frankfurt ([ADR-0002](adr/0002-supabase.md)) |
| Gateway | ESP32 + E32 + Wi-Fi | ESP32 + E32 + Wi-Fi (15.10'da denenen Raspberry Pi'den vazgeçildi, [ADR-0007](adr/0007-gateway-yeniden-esp32.md)) |
| Komut adları | `VANA_AÇ`, `VANA_KAPAT` | `VANA_AC`, `VANA_KAPAT` (ASCII) |
| Uygulama | İzleme ve komut | + roller, geçmiş grafiği, alarm ve push bildirimi, eşik ayarları |

## Bileşenler ve diller

Ayrıntı: [ADR-0006](adr/0006-bilesen-dilleri.md).

| Bileşen | Donanım / platform | Dil |
|---|---|---|
| Uç düğüm | ESP32 DevKit V1 + E32-433T20D (+ MAX3485) | C++ (Arduino, PlatformIO) |
| Gateway | ESP32 DevKit V1 + E32-433T20D, Wi-Fi | C++ (Arduino, PlatformIO) |
| Sunucu | Supabase (PostgreSQL, Auth, Realtime, Edge Functions, pg_cron) | SQL + TypeScript (Edge Function) |
| Mobil uygulama | Android (APK), iOS (Expo Go) | TypeScript (React Native, Expo) |
| Araçlar | Bilgisayar | TypeScript (`tools/`) |
