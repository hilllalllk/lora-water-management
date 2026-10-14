# 00 · Proje Özeti

> Kaynak: Ekibin "LoRaWAN Tabanlı Mobil Su Yönetim Sistemi – Ön Tasarım Dokümanı" (danışman görüşü doğrultusunda hazırlanmış ilk kapsam). Sonraki kararlarla değişen noktalar işaretlendi.

## Amaç

Su depoları, vanalar ve terfi merkezlerindeki dijital kontrol cihazları ile uzak bir merkez arasında LoRa tabanlı, **çift yönlü** bir haberleşme altyapısı kurmak. Sistem sadece izleme yapmaz; mobil uygulamadan sahadaki cihaza komut da gönderir.

Projenin amacı fiziksel vana veya pompa üretmek değil, **farklı saha cihazlarıyla konuşabilen esnek bir iletişim altyapısı** geliştirmektir. Saha cihazı değişse bile haberleşme mimarisi aynı kalmalıdır.

## Veri akışı

```
Saha Cihazı ⇄ ESP32 + LoRa ⇄ Gateway ⇄ Bulut ⇄ Mobil Uygulama
```

## Sistem ne yapacak?

- Sahadaki cihazdan seviye, vana durumu, basınç, debi gibi verileri okuyacak.
- Verileri LoRa ile gateway'e, gateway de internet üzerinden buluta aktaracak.
- Mobil uygulama güncel saha verilerini gösterecek.
- Uygulamadan verilen aç/kapat komutları ters yönde sahadaki cihaza iletilecek.

## İlk prototip

Gerçek endüstriyel cihaz olmadan, saha cihazı ESP32 üzerinde simüle edilir: LED vana durumunu, potansiyometre depo seviyesini temsil eder. Uygulamadan gelen `VANA_AÇ` / `VANA_KAPAT` komutu düğüme ulaşınca durum değişir ve yeni durum merkeze bildirilir.

## Ön dokümandan bu yana değişenler

| Konu | Ön doküman | Güncel karar |
|---|---|---|
| Radyo | "LoRaWAN" başlığı, E32 modülleri | Düz LoRa (E32), LoRaWAN ağ sunucusu yok ([ADR-0001](adr/0001-duz-lora.md)) |
| Bulut | Firebase / Cloud Firestore | Supabase ([ADR-0002](adr/0002-supabase.md)) |
| Komut adları | `VANA_AÇ`, `VANA_KAPAT` | `VANA_AC`, `VANA_KAPAT` (ASCII) |
