# ADR-0008: Saha cihazı adaptörleri ve Modbus RTU

- **Tarih:** 18.10.2026
- **Durum:** Kabul edildi

## Bağlam
Danışman (Erkan Duman, 18.10.2026): vanalarda, depolarda ve terfi merkezlerinde dijital kontrol cihazları zaten var; biz bu cihazlardan LoRa ile veri okuyup yazacağız; görevimiz altyapıyı hazırlamak, iletişim kurulacak cihaz kısmı esnek. Dolayısıyla sensör veya vana satın alınmayacak, ama gerçek cihazın modeli ve register listesi henüz belli değil. Bu tür saha kontrol cihazlarında en yaygın seri arayüz RS-485 üzerinden Modbus RTU'dur.

## Seçenekler
1. Sadece simülasyon (LED + potansiyometre), gerçek cihaz gelince adaptör yazmak.
2. Hazır bir Arduino Modbus kütüphanesi kullanan Modbus RTU adaptörü.
3. Kendi küçük, bağımlılıksız Modbus RTU master kütüphanemiz + `FieldDevice` arayüzünün arkasında iki adaptör: simüle cihaz ve Modbus RTU.

## Karar
**Seçenek 3.**

- `FieldDevice` arayüzü (`firmware/lib/FieldDevice`) arkasında iki adaptör; derleme ortamıyla seçilir:

  | Adaptör | Ortam | Adres |
  |---|---|---|
  | `SimulatedDevice` | `node`, `node2` | 0x1001, 0x1002 |
  | `ModbusRtuDevice` | `node-modbus` (`-DFIELD_DEVICE_MODBUS`) | 0x1003 |

- Fiziksel katman: MAX3485 (3,3 V) ile RS-485; ESP32 UART1, GPIO25 RX / GPIO26 TX / GPIO27 DE+RE. 9600 baud, 8N1, slave 1, 200 ms cevap zaman aşımı.
- `firmware/lib/ModbusRtu`: donanımdan bağımsız master (CRC-16/MODBUS, 0x03 holding register okuma, 0x06 tek register yazma, istisna cevapları, t3.5 boşluğu); 11 birim testi `pio test -e native` ile bilgisayarda çalışır.
- Register haritası tek dosyada: `firmware/include/modbus_map.h` (register 0–4 telemetri, 10 vana komutu, 11 etkinleştirme). Şimdilik **yer tutucudur**; gerçek cihaz için sadece bu dosya değişir.
- Gerçek cihaz yokken test: bilgisayarda Modbus slave simülatörü (ModRSsim2, Modbus Slave, `diagslave`) + USB-RS485 çevirici.

## Sonuçlar
- Protokol, gateway, veritabanı ve mobil uygulama adaptörden bağımsız kalır; cihaz değişince sadece adaptör veya register haritası değişir.
- Modbus hatası (zaman aşımı, CRC, istisna) komutta `DEVICE_ERROR`, telemetride `SensorFault` alarm biti olarak görünür ve veritabanında `device_alarm` alarmı açar.
- Dış kütüphane bağımlılığı yok; Modbus katmanı CI'da test edilir.
- Malzeme: 2× MAX3485 ve 1× USB-RS485 çevirici ([`hardware/bom.csv`](../../hardware/bom.csv)).
- Açık iş: gerçek cihazın register listesi danışmandan veya kılavuzdan alınacak (A8, [danışman soruları](../danisman-sorulari.md)). Cihaz Modbus TCP, 4–20 mA veya röle kullanıyorsa yeni bir `FieldDevice` adaptörü yazılır.
