# Donanım

- Malzeme listesi: [bom.csv](bom.csv) (toplam ~3.640 TL, Ekim 2026 tahmini)
- Bağlantı şemaları (ESP32 + E32-433T20D, MAX3485), E32 ayarları ve yasal güç uyarısı: [docs/04-donanim.md](../docs/04-donanim.md)
- Kurulum: [docs/05-kurulum.md](../docs/05-kurulum.md)
- Kararlar ve gerekçeler: [docs/09-teknoloji-kararlari.md §9](../docs/09-teknoloji-kararlari.md#9-malzeme-listesi)

Prototip 2 uç düğüm ve 1 gateway'den oluşur. Sensör veya vana satın alınmaz; sahadaki mevcut kontrol cihazlarıyla Modbus RTU üzerinden konuşulur.

| Bileşen | Donanım |
|---|---|
| Uç düğüm (simüle cihaz) | ESP32 DevKit V1 + E32-433T20D + LED + potansiyometre, USB adaptörle beslenir |
| Uç düğüm (Modbus) | ESP32 DevKit V1 + E32-433T20D + MAX3485 (RS-485), USB adaptörle beslenir |
| Gateway | ESP32 DevKit V1 + E32-433T20D (düğümle aynı bağlantı), Wi-Fi, USB adaptörle beslenir |
| Modbus testi | Bilgisayar + USB-RS485 çevirici + Modbus slave simülatörü |

Devre çizimleri (Fritzing / KiCad) ve kurulum fotoğrafları bu klasöre eklenecek:

```
hardware/
├── bom.csv
├── schematics/   # devre çizimleri
└── photos/       # prototip fotoğrafları
```
