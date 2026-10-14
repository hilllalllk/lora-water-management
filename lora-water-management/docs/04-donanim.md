# 04 · Donanım

## 1. Malzeme listesi (prototip)

Liste [`hardware/bom.csv`](../hardware/bom.csv) dosyasında da var.

| # | Parça | Adet | Nerede | Not |
|---|---|---|---|---|
| 1 | ESP32 DevKit v1 (ESP32-WROOM-32) | 2+ | 1 gateway + her düğüm için 1 | |
| 2 | Ebyte E32-433T30D LoRa modülü | 2+ | Her ESP32'ye 1 | UART arayüzlü, düz LoRa (LoRaWAN değil) |
| 3 | 433 MHz anten (SMA) | 2+ | Her E32'ye 1 | **Anten takmadan modülü çalıştırmayın** |
| 4 | 5 V, en az 2 A güç kaynağı | 2+ | Her istasyon | E32 gönderimde ~1 A çekebilir |
| 5 | 470–1000 µF elektrolitik kondansatör | 2+ | E32 besleme hattı | Anlık akım çekişlerini yumuşatır |
| 6 | 10 kΩ potansiyometre | 1 | Düğüm | Depo seviyesi simülasyonu |
| 7 | LED + 220 Ω direnç | 1 | Düğüm | Vana simülasyonu (dahili LED de kullanılabilir) |
| 8 | Breadboard, jumper kablolar | — | — | |
| 9 | (A8) MAX485 / MAX3485 RS485 dönüştürücü | 1 | Düğüm | Gerçek cihaz Modbus RTU ise |

## 2. Bağlantı şeması

Uç düğüm ve gateway aynı şekilde bağlanır. Pinler [`firmware/include/config.h`](../firmware/include/config.h) dosyasında tanımlıdır.

```
        ESP32 DevKit v1                      E32-433T30D
   ┌──────────────────────┐             ┌──────────────────┐
   │                 GPIO4 ├────────────►│ M0               │
   │                 GPIO5 ├────────────►│ M1               │
   │     GPIO16 (UART2 RX) │◄────────────┤ TXD              │
   │     GPIO17 (UART2 TX) ├────────────►│ RXD              │
   │                GPIO18 │◄────────────┤ AUX              │
   │                   GND ├─────┬───────┤ GND              │
   └──────────────────────┘     │   ┌───┤ VCC              │
                                 │   │   └──────────────────┘
         Harici 5 V ─────────────┼───┴── (+ 470 µF kondansatör VCC–GND arası)
         Harici GND ─────────────┘

   Sadece düğümde:
     GPIO2  ── 220 Ω ── LED ── GND          (vana)
     GPIO34 ── potansiyometre orta ucu       (seviye; uçlar 3V3 ve GND)
```

| E32 pini | ESP32 | Açıklama |
|---|---|---|
| M0, M1 | GPIO4, GPIO5 | Çalışma modu: `00` normal, `11` yapılandırma |
| TXD | GPIO16 | Modülün çıkışı → ESP32 RX |
| RXD | GPIO17 | ESP32 TX → modülün girişi |
| AUX | GPIO18 | LOW = modül meşgul, HIGH = hazır |
| VCC | Harici 5 V | 3.3–5.2 V aralığı; tam güç için 5 V önerilir |
| GND | Ortak toprak | ESP32 GND ile birleştirilmeli |

**Dikkat:** E32-433T30D'yi ESP32'nin 3V3 pininden beslemeyin. Kılavuza göre gönderim sırasında anlık akım 1 A civarındadır; ESP32 kartının regülatörü bunu karşılayamaz ve kart yeniden başlar.

## 3. E32 ayarları

Firmware açılışta modülü kendisi yapılandırır ([`E32Radio::begin`](../firmware/lib/E32Radio/src/E32Radio.cpp)). Elle bir şey yapmanıza gerek yok.

| Parametre | Değer | Bayt |
|---|---|---|
| Adres | Gateway `0x0001`, düğüm `0x1001`… | ADDH, ADDL |
| UART | 9600 8N1 | SPED bit7-3 = `00011` |
| Hava hızı | 2.4 kbps | SPED bit2-0 = `010` |
| Kanal | `0x17` → 410 + 23 = **433 MHz** | CHAN |
| İletim | Sabit adresli | OPTION bit7 = 1 |
| FEC | Açık | OPTION bit2 = 1 |
| Güç | **En düşük seviye** | OPTION bit1-0 = `11` |

Yapılandırma komutu: `C0 ADDH ADDL SPED CHAN OPTION` (mod 3'te, 9600 baud). Modül aynı 6 baytı geri yansıtır; firmware bunu kontrol eder ve uyuşmazsa seri monitöre uyarı yazar.

Kaynak: Ebyte E32 kullanım kılavuzu ([PDF](https://atta.szlcsc.com/upload/public/pdf/source/20220808/3C7B3108BF1F7C528E0D6D2B18D3633E.pdf)). Kılavuzda model adları (30D / 33D) karışık geçiyor; kendi modülünüzün etiketine ve satıcının verdiği kılavuza bakın.

## 4. Yasal uyarı: çıkış gücü ve frekans

E32-433T30D yaklaşık **1 W** (30 dBm) sınıfında bir modüldür. Avrupa (CEPT/ERC 70-03) kurallarında 433.05–434.79 MHz kısa mesafe cihaz bandında izin verilen güç bunun çok altındadır (genellikle 10 mW e.r.p. düzeyinde ve görev döngüsü sınırlı). **Türkiye için geçerli değer BTK düzenlemesinden doğrulanmalıdır**; bu not çıkarımdır, resmi kaynakla teyit edilmemiştir.

Bu yüzden:
- Firmware varsayılan olarak modülün **en düşük güç** seviyesini kullanır (`E32Power::Min`).
- Masa testleri kısa mesafede yapılır.
- Saha testinden önce danışmanla konuşulur ([danışman soruları](danisman-sorulari.md)).
- Bitirme raporunda bu kısıt açıkça yazılır.

## 5. Güç ve kurulum notları

- Düğümler şebeke beslemeli kabul edilir; radyo sürekli dinler (E32 mod 0, ~13 mA alım akımı). Pil ile çalışma hedeflenmiyor.
- Anten, modül çalışırken asla çıkarılmamalı.
- Dış ortam kutusu: IP65, anten kutunun dışında.

## 6. Gerçek saha cihazı (aşama A8)

Gerçek vana/terfi kontrolcüsünün arayüzü danışmanla belirlenecek. Seçeneğe göre yeni bir `FieldDevice` adaptörü yazılır; LoRa, gateway, veritabanı ve uygulama değişmez.

| Arayüz | Gerekli donanım | Adaptör |
|---|---|---|
| RS485 / Modbus RTU | MAX485 | [`ModbusRtuDevice`](../firmware/lib/FieldDevice/src/ModbusRtuDevice.h) (taslak) |
| 4–20 mA sensör | 250 Ω direnç + ADC veya ADS1115 | yazılacak |
| Röle ile vana | Röle modülü (optokuplörlü) | yazılacak |
