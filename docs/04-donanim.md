# 04 · Donanım

Prototip 2 uç düğüm ve 1 gateway'den oluşur. Uç düğüm ve gateway aynı donanımdır: **ESP32 DevKit V1 + Ebyte E32-433T20D**. Sensör, vana veya pompa satın alınmaz; sahadaki mevcut kontrol cihazlarıyla konuşulur ([ADR-0008](adr/0008-saha-cihazi-modbus.md)). Kararlar: [09 · Teknoloji ve Donanım Kararları](09-teknoloji-kararlari.md).

## 1. Malzeme listesi (prototip)

Liste [`hardware/bom.csv`](../hardware/bom.csv) dosyasında da var. Fiyatlar Ekim 2026 tahminidir.

| # | Parça | Adet | Birim (TL) | Toplam (TL) | Nerede |
|---|---|---|---|---|---|
| 1 | ESP32 DevKit V1 (ESP32-WROOM-32) | 3 | 250 | 750 | 2 düğüm + 1 gateway |
| 2 | Ebyte E32-433T20D (100 mW, 433 MHz) | 3 | 350 | 1.050 | Her ESP32'ye 1 |
| 3 | 433 MHz anten (SMA) | 3 | 80 | 240 | Her E32'ye 1. **Anten takmadan modülü çalıştırmayın** |
| 4 | MAX3485 RS-485 modülü (3,3 V) | 2 | 50 | 100 | `node-modbus` düğümü (+1 yedek) |
| 5 | USB-RS485 çevirici | 1 | 150 | 150 | Bilgisayarda Modbus slave simülatörü için |
| 6 | USB adaptör (5 V, en az 1 A) ve kablo | 3 | 150 | 450 | Her ESP32'ye 1 |
| 7 | Breadboard, jumper, LED, potansiyometre (10 kΩ), 100 µF kondansatör | 1 set | 300 | 300 | Simüle cihaz, bağlantılar |
| 8 | Yedek (bir ESP32 + bir E32) | 1 | 600 | 600 | |
| | **Toplam** | | | **~3.640** | |

E32-433T20D iletimde yaklaşık 130 mA çeker; ayrı 1 A'lik güç kaynağı gerekmez, ESP32 ile aynı USB adaptörden beslenir.

## 2. Bağlantı şeması

Pinler [`firmware/include/config.h`](../firmware/include/config.h) dosyasında tanımlıdır.

### 2.1 Uç düğüm (ESP32 + E32 + saha cihazı)

```
        ESP32 DevKit V1                      E32-433T20D
   ┌──────────────────────┐             ┌──────────────────┐
   │                 GPIO4 ├────────────►│ M0               │
   │                 GPIO5 ├────────────►│ M1               │
   │     GPIO16 (UART2 RX) │◄────────────┤ TXD              │
   │     GPIO17 (UART2 TX) ├────────────►│ RXD              │
   │                GPIO18 │◄────────────┤ AUX              │
   │              VIN (5V) ├────────┬───►│ VCC              │
   │                   GND ├──────┬─┼───►│ GND              │
   └──────────────────────┘      │ │    └──────────────────┘
                                  └─┴── 100 µF (VCC–GND, modüle yakın)

   ESP32 USB adaptörden (5 V) beslenir; VIN pini bu 5 V'u E32'ye verir.

   Simüle cihaz (env node, node2):
     GPIO2  ── 220 Ω ── LED ── GND          (vana; kart üstündeki LED de olur)
     GPIO34 ── potansiyometre orta ucu       (seviye; uçlar 3V3 ve GND)
```

| E32 pini | ESP32 | Açıklama |
|---|---|---|
| M0, M1 | GPIO4, GPIO5 | Çalışma modu: `00` normal, `11` yapılandırma |
| TXD | GPIO16 | Modülün çıkışı → ESP32 RX |
| RXD | GPIO17 | ESP32 TX → modülün girişi |
| AUX | GPIO18 | LOW = modül meşgul, HIGH = hazır |
| VCC | VIN (5 V) | 3,3–5,2 V aralığı; 3V3 regülatörü gerilim düşürürse 5 V kullanın |
| GND | GND | Ortak toprak |

#### Modbus RTU düğümü (env `node-modbus`, MAX3485)

```
        ESP32 DevKit V1                MAX3485 (3,3 V)            Kontrol cihazı
   ┌──────────────────────┐        ┌──────────────────┐        ┌──────────────┐
   │     GPIO25 (UART1 RX) │◄───────┤ RO             A ├────────┤ A (+)        │
   │     GPIO26 (UART1 TX) ├───────►│ DI             B ├────────┤ B (−)        │
   │                GPIO27 ├───────►│ DE + /RE         │        │              │
   │                   3V3 ├───────►│ VCC              │        │              │
   │                   GND ├───────►│ GND          GND ├────────┤ GND          │
   └──────────────────────┘        └──────────────────┘        └──────────────┘
```

| MAX3485 pini | Bağlantı | Not |
|---|---|---|
| RO | GPIO25 (UART1 RX, `PIN_RS485_RX`) | |
| DI | GPIO26 (UART1 TX, `PIN_RS485_TX`) | |
| DE + /RE (birleştirilmiş) | GPIO27 (`PIN_RS485_DE`) | HIGH = gönder |
| VCC / GND | 3V3 / GND | Toprak kontrol cihazıyla ortak |
| A / B | Cihazın A(+) / B(−) | Uzun hatlarda iki uca 120 Ω sonlandırma |

Hat ayarları: 9600 baud, 8N1, slave 1, 200 ms cevap zaman aşımı. Register haritası: [`firmware/include/modbus_map.h`](../firmware/include/modbus_map.h) (yer tutucu; gerçek cihazın listesiyle değiştirilecek). E32 bağlantısı §2.1'deki gibidir.

**Gerçek cihaz yokken test:** Bilgisayara takılan USB-RS485 çeviricinin A/B uçları MAX3485'in A/B uçlarına (GND ortak) bağlanır; bilgisayarda Modbus slave simülatörü (ModRSsim2, Modbus Slave veya `diagslave`) 9600 8N1, slave 1, holding register 0–11 ile çalıştırılır. Adımlar: [05 §4.3](05-kurulum.md#43-modbus-düğümü-node-modbus), [`firmware/README.md`](../firmware/README.md#testing-modbus-without-the-real-controller).

### 2.2 Gateway (ESP32 + E32)

Gateway'in E32 bağlantısı **uç düğümle aynıdır** (§2.1, aynı pinler ve aynı besleme). Simüle cihaz ve MAX3485 bağlanmaz. Gateway internete Wi-Fi (2,4 GHz) ile bağlanır; ayrıca donanım gerekmez.

## 3. E32 ayarları

Firmware açılışta modülü kendisi yapılandırır ([`E32Radio::begin`](../firmware/lib/E32Radio/src/E32Radio.cpp)); düğümde ve gateway'de elle bir şey yapmanıza gerek yok. İki uç **aynı kanal, hava hızı ve iletim tipiyle** çalışır.

| Parametre | Değer | Bayt |
|---|---|---|
| Adres | Gateway `0x0001`, düğüm `0x1001`… | ADDH, ADDL |
| UART | 9600 8N1 | SPED bit7-3 = `00011` |
| Hava hızı | 2,4 kbps | SPED bit2-0 = `010` |
| Kanal | `0x17` → 410 + 23 = **433 MHz** | CHAN |
| İletim | Sabit adresli | OPTION bit7 = 1 |
| FEC | Açık | OPTION bit2 = 1 |
| Güç | **En düşük kademe** (`E32Power::Min`) | OPTION bit1-0 = `11` |

E32-433T20D güç kademeleri (`E32Power`, `E32Radio.h`):

| Kod | `E32Power` | Çıkış |
|---|---|---|
| `00` | `Max` | 20 dBm (100 mW) |
| `01` | `High` | 17 dBm (~50 mW) |
| `10` | `Medium` | 14 dBm (~25 mW) |
| `11` | `Min` (**varsayılan**) | 10 dBm (~10 mW) |

Yapılandırma komutu: `C0 ADDH ADDL SPED CHAN OPTION` (mod 3'te, 9600 baud). Modül aynı 6 baytı geri yansıtır; uyuşmazsa seri monitöre `WARNING: could not verify E32 configuration` yazılır.

Kaynak: Ebyte E32 kullanım kılavuzu ([PDF](https://atta.szlcsc.com/upload/public/pdf/source/20220808/3C7B3108BF1F7C528E0D6D2B18D3633E.pdf)). Kılavuzda model adları karışık geçebiliyor; modülün etiketine ve satıcının verdiği kılavuza bakın.

## 4. Yasal uyarı: çıkış gücü ve frekans

Avrupa (CEPT/ERC 70-03) kurallarında 433,05–434,79 MHz kısa mesafe cihaz bandında izin verilen güç genellikle **10 mW e.r.p.** düzeyindedir (görev döngüsü sınırlı). **Türkiye için geçerli değer BTK düzenlemesinden doğrulanmalıdır**; bu not çıkarımdır ([danışman soruları](danisman-sorulari.md)).

E32-433T20D bu yüzden seçildi: en yüksek gücü 100 mW (20 dBm), en düşük kademesi 10 dBm ≈ 10 mW. Ön dokümandaki E32-433T30D (~1 W) en düşük kademede bile bu sınırın üstündeydi.

- Firmware varsayılan olarak `E32Power::Min` (10 dBm) kullanır; düğüm ve gateway aynı.
- e.r.p. anten kazancını da içerir; yüksek kazançlı anten 10 mW'ı aşabilir. Prototipte modülle gelen standart anten kullanılır.
- Masa testleri kısa mesafede yapılır; saha testinden önce danışmanla konuşulur.
- Bitirme raporunda bu kısıt açıkça yazılır.

## 5. Güç ve kurulum notları

- Uç düğümler ve gateway **USB adaptörle** (5 V, en az 1 A) beslenir. Sahaya özel güç tasarımı (pil, güneş paneli) kapsam dışıdır.
- Radyo sürekli dinler (E32 mod 0); vana komutları gecikmesiz ulaşır.
- Gateway iç mekânda, 2,4 GHz Wi-Fi erişimi olan bir yerde durur; anteni mümkün olduğunca yüksekte olmalı.
- Anten, modül çalışırken asla çıkarılmamalı.
- Dış ortam kutusu (ileride): IP65, anten kutunun dışında.

## 6. Saha cihazı arayüzleri

Sahadaki kontrol cihazına göre `FieldDevice` adaptörü seçilir; LoRa, gateway, veritabanı ve uygulama değişmez.

| Arayüz | Gerekli donanım | Adaptör | Durum |
|---|---|---|---|
| Simülasyon | LED + potansiyometre | [`SimulatedDevice`](../firmware/lib/FieldDevice/src/SimulatedDevice.h) | Hazır (`node`, `node2`) |
| RS-485 / Modbus RTU | MAX3485 | [`ModbusRtuDevice`](../firmware/lib/FieldDevice/src/ModbusRtuDevice.h) | Hazır (`node-modbus`); gerçek register haritası bekleniyor (A8) |
| 4–20 mA sensör | 250 Ω direnç + ADC veya ADS1115 | Yazılacak | Gerekirse |
| Röle ile vana | Röle modülü (optokuplörlü) | Yazılacak | Gerekirse |
