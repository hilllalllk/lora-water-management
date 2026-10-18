# 01 · LoRa/LoRaWAN Uygulamalarında Genel Mimari Araştırması

> Bitirme projesi: **LoRaWAN Tabanlı Mobil Su Yönetim Sistemi**
> Adım 1 çıktısı · 14 Ekim 2026
> Bu doküman doğrudan repodaki `docs/` klasörüne konabilecek şekilde yazıldı.

## Özet

GitHub ve Medium'daki projeler iki ana mimari ailesine ayrılıyor:

1. **Standart LoRaWAN mimarisi** (ağ sunucusu olan, "yıldızların yıldızı" topolojisi). Projelerin büyük çoğunluğu bu.
2. **Ham LoRa noktadan noktaya mimari** (ağ sunucusu yok; ESP32 + LoRa modülü kendi "gateway"ini yapıyor). Ön dokümanımızdaki E32-433T30D tabanlı tasarım bu aileye giriyor.

Bizim için en önemli bulgu: **E32-433T30D modülleri LoRaWAN değil, şeffaf UART LoRa modülüdür** ve tek kanallı ESP32 "gateway"leri LoRaWAN topluluğu tarafından gerçek LoRaWAN gateway'i sayılmıyor. Proje başlığında "LoRaWAN" kaldıkça donanım ve ağ katmanı kararının danışmanla netleşmesi gerekiyor (aşağıda üç seçenek var).

---

## 1. Standart LoRaWAN mimarisi

The Things Network dokümantasyonuna göre LoRaWAN ağları **star-of-stars** topolojisindedir: uç cihazlar belirli bir gateway'e bağlanmaz, mesaj menzildeki tüm gateway'lere ulaşır, gateway'ler de hepsini ağ sunucusuna iletir. [TTN – Architecture](https://www.thethingsnetwork.org/docs/lorawan/architecture/)

```
[Sensör / Saha cihazı]
        │ (UART, Modbus, GPIO...)
[Uç düğüm: MCU + LoRaWAN radyo]          ← sınıf A / B / C
        │  LoRa radyo (EU868)
[Gateway: 8 kanallı konsantratör]        ← paket yönlendirici (UDP / Basics Station)
        │  İnternet (Ethernet, Wi-Fi, 4G)
[Ağ Sunucusu (Network Server)]           ← TTN/The Things Stack veya ChirpStack
        │  Entegrasyon: MQTT / HTTP Webhook
[Uygulama arka ucu]                      ← Node-RED, Node.js, Cloud Function, PHP...
        │
[Veritabanı]                             ← InfluxDB, PostgreSQL/MySQL, Firebase, BigQuery
        │
[Arayüz]                                 ← Grafana, web paneli, mobil uygulama
```

### Katmanların görevleri

| Katman | Görev | Projelerde görülen seçenekler |
|---|---|---|
| Uç düğüm | Sensörü okur, veriyi sıkıştırılmış bayt olarak gönderir, komut alır | ESP32 + SX1276/RFM95, TTGO/Heltec LoRa32, RN2483, Feather M0 LoRa, LoRa-E5 |
| Gateway | Radyo paketlerini ağ sunucusuna aktarır, kendisi karar vermez | RAK/Dragino 8 kanallı gateway'ler, Raspberry Pi + konsantratör |
| Ağ sunucusu | Tekrar eden paketleri ayıklar, cihazı doğrular, ADR yönetir, downlink için en iyi gateway'i seçer | The Things Stack (TTN), ChirpStack (açık kaynak, kendin barındırırsın) |
| Join sunucusu | OTAA ile güvenli katılım, oturum anahtarlarının üretimi | TTN ve ChirpStack'in içinde gelir |
| Uygulama / entegrasyon | Ham baytı anlamlı veriye çevirir (payload decoder), veritabanına yazar, downlink kuyruğuna komut ekler | MQTT, HTTP webhook, Node-RED |
| Veri + arayüz | Saklama ve gösterim | InfluxDB + Grafana, Firebase + web/mobil, MySQL + PHP |

Kaynak: [TTN – Architecture](https://www.thethingsnetwork.org/docs/lorawan/architecture/), [ChirpStack Docker Quickstart](https://www.chirpstack.io/docs/getting-started/docker.html)

### Ağ sunucusu: TTN mi ChirpStack mı?

- **The Things Network (topluluk ağı):** Ücretsiz, kurulum yok. Ama adil kullanım politikası var ve forumda belirtildiği üzere cihaz başına **günde yaklaşık 10 downlink** sınırı bulunuyor. Uzaktan vana kontrolü gibi komut ağırlıklı bir senaryoda bu sınır önemli. [TTN forum – gate control](https://thethingsnetwork.org/forum/t/controlling-a-gate-with-downlink-limitations/23693)
- **ChirpStack (kendin barındırırsın):** Açık kaynak, `chirpstack-docker` reposu ile Docker Compose üzerinden tek komutla ayağa kalkıyor. Gateway Bridge (UDP 1700 / Basics Station 3001), MQTT broker ve web arayüzü (8080) içeriyor. Topluluk ağının downlink sınırı yok; yine de bölgesel görev döngüsü (duty cycle) kuralları geçerli. [ChirpStack Docker](https://www.chirpstack.io/docs/getting-started/docker.html), [github.com/chirpstack/chirpstack-docker](https://github.com/chirpstack/chirpstack-docker)

### Cihaz sınıfları ve komut gönderme (bizim için kritik)

| Sınıf | Downlink gecikmesi | Güç | Bizim senaryoya uygunluğu |
|---|---|---|---|
| A | En yüksek: komut ancak cihaz veri gönderdikten sonraki kısa pencerede iner | En düşük (pil) | Sadece izleme yapan sensörler |
| B | Orta: periyodik zamanlanmış pencereler | Orta | Nadiren kullanılıyor |
| C | En düşük: cihaz sürekli dinler | En yüksek (şebeke) | **Vana/pompa kontrol düğümü** |

Kaynak: [TTN – Device Classes](https://www.thethingsnetwork.org/docs/lorawan/classes/index.html)

Forumdaki kapı kontrolü örneğinde Sınıf A ile komutun 30 saniye ile 20 dakika arasında geciktiği anlatılıyor. Terfi merkezleri ve vanalar genelde şebeke elektriğine bağlı olduğu için kontrol düğümünün **Sınıf C** olması mantıklı görünüyor (bu çıkarım; danışmanla teyit edilmeli).

---

## 2. Ham LoRa (noktadan noktaya) mimarisi

```
[Saha cihazı] ⇄ [ESP32 + E32] ~~LoRa 433 MHz~~ [ESP32 + E32 + Wi-Fi] ⇄ [REST / WebSocket / MQTT / Firebase] ⇄ [Uygulama]
```

- En bilinen örnek Renzo Mischianti'nin **EByte LoRa E32 gateway** projesi: ESP32/ESP8266 üzerinde çalışan basit bir "gateway", REST API (port 8080) ve WebSocket (port 8081) sunuyor; düğümlere şeffaf, sabit adresli (kanal + adres baytları) veya yayın modunda mesaj gönderebiliyor. Arayüz React/Redux ile yazılmış. [mischianti.org – E32 gateway](https://mischianti.org/ebyte-lora-e32-gateway-manage-via-rest-and-websocket-esp8266-esp32-1/), [xreef/EByte-LoRa-Manager-React-Redux-Web-Front-End](https://github.com/xreef/EByte-LoRa-Manager-React-Redux-Web-Front-End), [xreef/LoRa_E32_Series_Library](https://github.com/xreef/LoRa_E32_Series_Library)
- **Artısı:** Ucuz, hızlı başlar, elimizdeki E32 modülleriyle hemen çalışır, komut sayısında sınır yok.
- **Eksisi:** LoRaWAN'ın hazır verdiği şeyleri (cihaz kimliği, AES şifreleme, mesaj sayacı, onay/ACK, çoklu gateway, ADR) kendimiz yazmamız gerekir. Bu sistem teknik olarak "LoRaWAN" değildir.

### Tek kanallı "gateway" uyarısı

ESP32 + tek bir LoRa çipiyle yapılan "tek kanallı LoRaWAN gateway"leri TTN forumunda moderatörler tarafından "LoRaWAN gateway'i değil" diye niteleniyor; standart cihazlar 8 kanal kullandığı için gateway'in de 8 kanalı dinlemesi gerekiyor. [TTN forum – single channel](https://thethingsnetwork.org/forum/t/single-or-dual-channel-forwarder/34041) Yani "ESP32'yi LoRaWAN gateway'i yapalım" yolu jüri karşısında zayıf kalır.

---

## 3. İncelenen örnek projeler

| Proje | Mimari | Bize ne öğretiyor |
|---|---|---|
| [alvarowolfx/gcloud-lora-ttn](https://github.com/alvarowolfx/gcloud-lora-ttn) | Feather M0 LoRa → gateway → TTN → HTTP entegrasyonu → Cloud Run / Cloud Function → **Firebase** (anlık) + BigQuery (geçmiş) → Firebase Hosting web | Ön dokümandaki Firebase fikrine en yakın hazır örnek: TTN ile Firebase arasına küçük bir bulut fonksiyonu konuyor. (Not: tek kanallı gateway kullanıyor.) |
| [Medium – TTS webhook → MySQL](https://lmabele.medium.com/implementation-of-http-integration-from-the-tts-ttn-v3-lorawan-network-server-to-a-custom-mysql-47f1fb9ec4e7) | Arduino + RN2483 → The Things Stack → webhook → PHP → MySQL | Webhook entegrasyonunun adım adım nasıl kurulduğu; dokümantasyon tarzı için iyi örnek. |
| [Drasek/TTN-Water](https://github.com/Drasek/TTN-Water) | Ultrasonik seviye sensörü + Pro Mini + RFM95 → TTN (OTAA) | Su seviyesi düğümü: 15 dk'da bir ölçüm, hızlı değişimde 5 dk'ya inen uyarlamalı gönderim, hatalı okumaları tekrar deneme. Kimlik bilgileri için `ttn_secrets_sample.h` örnek dosyası (repoda anahtar paylaşmamak için iyi pratik). |
| [Medium – TTGO ESP32 + MicroPython → TTN](https://medium.com/@JoooostB/how-i-send-my-first-lorawan-message-to-the-things-network-using-a-ttgo-esp32-micropython-a3fe447fff82) | TTGO ESP32 (SX1276) → TTN, ABP, JavaScript payload decoder | ESP32 ile ilk LoRaWAN mesajı; payload decoder kavramı. Yorumlarda sık gönderimin adil kullanım sınırına takıldığı belirtiliyor. |
| [ivfrost/hydrolink-app](https://github.com/ivfrost/hydrolink-app) | React Native (Expo) → REST API (Spring Boot) → AWS IoT → ESP32 sulama kontrolcüsü | LoRa yok ama **komut akışı** bizim için örnek: uygulama komutu API'ye yollar, API yetki ve güvenlik kontrolü yapar, cihaz yeni durumunu bildirene kadar arayüzde "bekliyor" gösterilir, zaman aşımında temizlenir. |
| [mischianti E32 gateway](https://mischianti.org/ebyte-lora-e32-gateway-manage-via-rest-and-websocket-esp8266-esp32-1/) | ESP32 + E32 → REST/WebSocket → React arayüz | Elimizdeki modüllerle yapılabilecek ham LoRa mimarisinin hazır referansı. |
| [ChirpStack + Node-RED + InfluxDB + Grafana](https://thethingsnetwork.org/community/frankfurt/post/create-your-own-lorawan-mapper-with-node-red-influxdb-and-grafana) | NS → MQTT → Node-RED → InfluxDB → Grafana | Açık kaynak "klasik yığın"; panel/geçmiş grafikleri için hızlı yol. |

---

## 4. Projelerde tekrar eden ortak desenler

1. **Katmanlar birbirinden ayrık.** Gateway veriyi yorumlamaz; anlamlandırma (payload decoder) ağ sunucusunda veya arka uçta yapılır. Bu, ön dokümandaki "saha cihazı değişse de haberleşme katmanı aynı kalsın" hedefiyle birebir örtüşüyor.
2. **Kompakt ikili payload.** JSON yerine birkaç baytlık yapı gönderilir (ör. 1 bayt vana durumu + 2 bayt seviye). Çözücü JavaScript ile ağ sunucusunda yazılır.
3. **Mobil uygulama cihaza doğrudan konuşmaz.** Komut: Uygulama → Arka uç (yetki kontrolü) → Ağ sunucusunun downlink kuyruğu → Cihaz. Cihaz yeni durumu uplink ile bildirince arayüz güncellenir.
4. **Gizli anahtarlar repoya girmez.** `secrets_sample.h` / `.env.example` deseni.
5. **Altyapı Docker Compose ile tarif edilir.** Başkası repoyu indirip aynı sistemi kurabilir; bitirme projesinin "tekrarlanabilir" olması için önemli.
6. **Uyarlamalı gönderim.** Değişim yoksa seyrek, değişim/alarm varsa sık gönderim (TTN-Water örneği).

---

## 5. Bizim proje için seçenekler

| | A) Ham LoRa (mevcut E32) | B) Tam LoRaWAN | C) Hibrit / aşamalı |
|---|---|---|---|
| Uç düğüm | ESP32 + E32-433T30D | ESP32 + SX1276/RFM95 (868 MHz) veya hazır LoRaWAN kartı | Önce A, sonra B |
| Gateway | ESP32 + E32 + Wi-Fi | 8 kanallı LoRaWAN gateway | |
| Ağ sunucusu | Yok (kendi protokolümüz) | ChirpStack (Docker) veya TTN | |
| Bulut | Firebase | ChirpStack → MQTT → küçük servis → Firebase | |
| Mobil | React Native | React Native | React Native |
| Başlıkla uyum | Zayıf ("LoRaWAN" değil) | Tam | Final hâli tam |
| Ek donanım | Yok | Yeni radyo modülleri + gateway | B'deki gibi, sonradan |

**Önerim (danışman onayına bağlı):** B veya C. Başlık "LoRaWAN" olduğu için finalde gerçek bir LoRaWAN ağı göstermek gerekiyor. Ağ sunucusu olarak **ChirpStack** öneriyorum: kendimiz barındırdığımız için topluluk ağının günlük downlink sınırına takılmayız, vana düğümünü Sınıf C yapabiliriz ve `docker-compose` ile repoda tekrarlanabilir kurulum verebiliriz. E32 modülleri boşa gitmez; C seçeneğiyle ilk prototipte çift yönlü haberleşmeyi doğrulamak için kullanılabilir.

### Karar (14 Ekim 2026)

Ekip **A) Ham LoRa** seçeneğiyle ilerlemeye karar verdi: uç düğümler ESP32 + E32-433T30D, gateway ESP32 + E32 + Wi-Fi, ağ sunucusu yok. Adresleme, onay (ACK) ve komut protokolü kendimiz tarafından tasarlanacak (adım 2).

> **Güncelleme (15 Ekim 2026):** Gateway geçici olarak Raspberry Pi + E32 (Node.js + TypeScript) olarak değiştirildi ([ADR-0005](adr/0005-raspberry-pi-nodejs-gateway.md)).
>
> **Güncelleme (18 Ekim 2026):** Danışman başlığı "LoRaWAN Tabanlı Mobil Su Yönetim Sisteminin Geliştirilmesi" olarak onayladı; teknik olarak ham LoRa (A seçeneği) devam ediyor. Gateway yeniden **ESP32 + E32 + Wi-Fi** oldu ([ADR-0007](adr/0007-gateway-yeniden-esp32.md)). Radyo modülü E32-433T30D yerine **E32-433T20D** (100 mW, en düşük kademe ≈ 10 mW). Sahadaki mevcut kontrol cihazlarıyla Modbus RTU üzerinden konuşulur ([ADR-0008](adr/0008-saha-cihazi-modbus.md)). Güncel kararlar: [09](09-teknoloji-kararlari.md).

### Danışmana sorulacaklar

1. Proje gerçek LoRaWAN ağı (ağ sunucusu + standart gateway) mı gerektiriyor, yoksa LoRa ile çift yönlü haberleşme yeterli mi?
2. LoRaWAN ise yeni donanım (868 MHz uç düğüm radyosu + 8 kanallı gateway) alınabilir mi?
3. Gerçek saha cihazı entegrasyonu hangi protokolle olacak (ör. Modbus RTU / RS485)? Bu, uç düğüm tasarımını etkiliyor.

### Doğrulanması gereken notlar

- Türkiye'de LoRaWAN için yaygın kullanılan bant **EU863-870**; E32-433 modülleri 433 MHz'de çalışıyor. Türkiye'nin bölgesel parametreleri ve izinli güç/görev döngüsü değerleri adım 2'de resmi kaynaktan teyit edilmeli (bu not çıkarımdır).
- TTN'in günlük downlink sınırı resmi politika sayfasından değil forum yanıtlarından alındı.
