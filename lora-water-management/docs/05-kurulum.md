# 05 · Kurulum Rehberi

Bu rehber sıfırdan çalışan bir sistem kurmak için gereken adımları sırasıyla anlatır. Her adımın sonunda "kontrol" satırı var; o çalışmadan sonraki adıma geçmeyin.

## 0. Gerekli araçlar

| Araç | Ne için | İndirme |
|---|---|---|
| Git | Repoyu indirmek | https://git-scm.com |
| VS Code + **PlatformIO IDE** eklentisi | ESP32 firmware'ini derleyip yüklemek | https://platformio.org/install/ide?install=vscode |
| Node.js (LTS) | Mobil uygulama | https://nodejs.org |
| Python 3 | `tools/frame_tool.py` (isteğe bağlı) | https://python.org |
| Telefonda **Expo Go** | Uygulamayı telefonda açmak | App Store / Google Play |
| Supabase hesabı | Bulut veritabanı | https://supabase.com |

```bash
git clone https://github.com/hilllalllk/lora-water-management.git
cd lora-water-management
```

## 1. Supabase projesi

1. Supabase'de **New project** oluşturun. Bölge olarak size yakın olanı seçin (ör. Frankfurt).
2. **SQL Editor**'ü açın, [`backend/supabase/migrations/20261014000000_init.sql`](../backend/supabase/migrations/20261014000000_init.sql) dosyasının tamamını yapıştırıp **Run** deyin.
3. Aynı yerde [`backend/supabase/seed.sql`](../backend/supabase/seed.sql) dosyasını çalıştırın (iki örnek cihaz ekler).
4. **Authentication → Users → Add user** ile iki kullanıcı oluşturun:
   - Kendi kullanıcınız (mobil uygulamaya bununla gireceksiniz)
   - Gateway kullanıcısı, ör. `gateway@ornek.com` (güçlü bir şifre verin)
5. Gateway kullanıcısına "gateway" rolünü verin. SQL Editor'de:

   ```sql
   update auth.users
      set raw_app_meta_data = raw_app_meta_data || '{"role": "gateway"}'
    where email = 'gateway@ornek.com';
   ```

6. **Project Settings → API** sayfasından şunları not edin:
   - Project URL (`https://xxxx.supabase.co`)
   - `anon` (yeni panellerde **publishable**) anahtar

> **`service_role` (secret) anahtarını hiçbir cihaza veya uygulamaya koymayın.** Bu anahtar tüm güvenlik kurallarını atlar.

**Kontrol:** Table Editor'de `devices` tablosunda `node-1001` ve `node-1002` görünüyor.

## 2. Donanımı bağlama

[04 · Donanım](04-donanim.md) dokümanındaki şemaya göre her ESP32'yi bir E32 modülüne bağlayın. Antenleri takın. E32'yi harici 5 V ile besleyin.

## 3. Firmware

VS Code'da `firmware/` klasörünü açın (PlatformIO projeyi tanır).

### 3.1 Gizli bilgiler

```bash
cd firmware
cp include/secrets.example.h include/secrets.h
```

`include/secrets.h` içine Wi-Fi bilgilerini, Supabase URL'sini, anon anahtarı ve gateway kullanıcısının e-posta/şifresini yazın. Bu dosya `.gitignore` içindedir, repoya gitmez.

### 3.2 Testler (kart gerekmez)

```bash
pio test -e native
```

**Kontrol:** `16 Tests 0 Failures`.

### 3.3 Uç düğüm

```bash
pio run -e node -t upload      # adres 0x1001
pio device monitor
```

İkinci düğüm için `-e node2` (adres 0x1002). Yeni bir düğüm eklemek için `platformio.ini`'ye yeni bir ortam ekleyin ve aynı adresi `devices` tablosuna yazın.

**Kontrol:** Seri monitörde `[node] adres=0x1001` ve periyodik `[node] telemetri: ...` satırları. Gateway yokken `ACK alinamadi` uyarısı normaldir.

### 3.4 Gateway

```bash
pio run -e gateway -t upload
pio device monitor
```

**Kontrol:** `[gw] Wi-Fi bagli`, `[gw] 2 cihaz yuklendi`. Düğüm açıkken Supabase'de `telemetry` tablosuna satırlar düşmeli ve `devices.level_mm` güncellenmeli.

## 4. Mobil uygulama

```bash
cd mobile
npm install
cp .env.example .env.local     # URL ve anon/publishable anahtarı yazın
npx expo start
```

Terminaldeki QR kodu telefondaki Expo Go ile okutun ve 1. adımda oluşturduğunuz kendi kullanıcınızla giriş yapın.

**Kontrol:** Cihaz listesinde `Depo-1 (prototip)` çevrimiçi görünüyor, potansiyometreyi çevirince seviye birkaç saniye içinde değişiyor. "Vana aç" butonuna basınca düğümdeki LED yanıyor ve komut "Tamamlandı" oluyor.

## 5. Sorun giderme

| Belirti | Olası neden | Çözüm |
|---|---|---|
| `E32 yapilandirmasi dogrulanamadi` | M0/M1/AUX veya TX/RX ters bağlı, besleme yetersiz | Bağlantıları kontrol edin, TXD↔RX ve RXD↔TX çapraz olmalı |
| Kart sürekli yeniden başlıyor | E32 ESP32'nin 3V3 pininden besleniyor | Harici 5 V + kondansatör kullanın |
| Düğümde hep `ACK alinamadi` | Gateway kapalı / farklı kanal veya hava hızı | İki tarafta `LORA_CHANNEL` ve hava hızı aynı olmalı |
| Gateway `bilinmeyen adres 0x1001` | Adres `devices` tablosunda yok | `devices.address` = 4097 (0x1001) olmalı |
| Gateway `POST .../telemetry -> 401/403` | Gateway kullanıcısında rol yok veya şifre yanlış | 1.5 adımını tekrar edin, gateway'i yeniden başlatın |
| Uygulama "Supabase ayarları eksik" hatası | `.env.local` yok | `cp .env.example .env.local`, sonra `npx expo start -c` |
| Uygulamada veri var ama canlı güncellenmiyor | Realtime yayını eksik | Migration'daki `alter publication supabase_realtime ...` satırını çalıştırın |
| Çerçeveyi elle çözmek istiyorum | — | `python3 tools/frame_tool.py decode "01 01 ..."` |

## 6. Testleri çalıştırma (geliştiriciler için)

| Bölüm | Komut | Gerekenler |
|---|---|---|
| Protokol (C++) | `cd firmware && pio test -e native` | PlatformIO |
| Veritabanı + RLS | `bash backend/supabase/tests/run.sh` | PostgreSQL (`PGHOST`, `PGUSER` ayarlı) |
| Mobil tip kontrolü | `cd mobile && npm run typecheck` | Node.js |
| Python aracı | `python3 -m unittest discover tools` | Python 3 |

GitHub'a her gönderimde bu testler [CI](../.github/workflows/ci.yml) ile otomatik çalışır.
