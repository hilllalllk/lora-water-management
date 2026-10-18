#!/usr/bin/env bash
# Creates the project's labels, phase issues (A1-A8) and a GitHub Projects board.
#
# Requirements: GitHub CLI (https://cli.github.com), logged in with
#   gh auth login
#   gh auth refresh -s project      # needed only for the board
# Run once from the repository root:
#   bash scripts/github-issues.sh
# On Windows, run it from Git Bash (installed with Git for Windows).
set -euo pipefail

REPO="${REPO:-$(gh repo view --json nameWithOwner -q .nameWithOwner)}"
echo "Repository: $REPO"

label() { gh label create "$1" --repo "$REPO" --color "$2" --description "$3" --force >/dev/null; }
label "aşama"        "1d76db" "Geliştirme aşaması (A1-A8)"
label "firmware"     "5319e7" "ESP32 uç düğüm / gateway"
label "backend"      "0e8a16" "Supabase şeması, RLS, Edge Function"
label "mobil"        "fbca04" "React Native uygulaması"
label "dokümantasyon" "c5def5" "docs/ ve README"
label "test"         "d93f0b" "Masa, saha veya kabul testi"

issue() { # title, labels, body
  gh issue create --repo "$REPO" --title "$1" --label "$2" --body "$3" | tail -1
}

issue "A1: İki ESP32 + E32 ile sabit adresli gönderim" "aşama,firmware,test" "$(cat <<'MD'
**Amaç:** İki E32-433T20D modülü arasında sabit adresli iletimi masada doğrulamak.

- [ ] Uç düğüm ve gateway kartlarını bağla (docs/04-donanim.md)
- [ ] E32 ayarlarını oku ve doğrula (kanal 0x17, 2,4 kbps, en düşük güç)
- [ ] 100 paket gönder, kaçının ulaştığını kaydet
- [ ] Sonuçları docs/06-test-plani.md ve geliştirme günlüğüne yaz
MD
)"

issue "A2: Protokol v1 (CRC, ACK, tekrar) kartta" "aşama,firmware,test" "$(cat <<'MD'
- [ ] Telemetri çerçevesi gateway'de doğru çözülüyor (seri monitör + `cd tools && npm run frame -- decode`)
- [ ] ACK kaybında 3 deneme ve tekrar filtresi çalışıyor
- [ ] Bozuk CRC'li çerçeve reddediliyor
MD
)"

issue "A3: Saha cihazı adaptörleri (simüle + Modbus RTU)" "aşama,firmware,test" "$(cat <<'MD'
- [ ] Simüle cihaz: potansiyometre seviyeyi, LED vanayı taklit ediyor
- [ ] Modbus RTU: MAX3485 bağlantısı, bilgisayardaki Modbus slave simülatörü ile okuma/yazma
- [ ] Gerçek cihazın register listesi danışmandan alındı (firmware/include/modbus_map.h)
MD
)"

issue "A4: Gateway → Supabase" "aşama,firmware,backend" "$(cat <<'MD'
- [ ] Supabase projesi (ücretsiz plan, Frankfurt) açıldı, migration'lar çalıştırıldı
- [ ] Gateway kullanıcısı oluşturuldu ve `gateway` rolü verildi
- [ ] Telemetri `telemetry` tablosuna düşüyor, `devices` güncelleniyor
- [ ] TLS sertifika doğrulaması çalışıyor
MD
)"

issue "A5: Supabase → gateway → düğüm komutları" "aşama,firmware,backend,test" "$(cat <<'MD'
- [ ] Uygulamadan gönderilen vana komutu düğüme ulaşıyor (pending → sent → delivered → done)
- [ ] Ulaşmayan komut `failed` oluyor
- [ ] Komut gecikmesi ölçüldü ve kaydedildi
MD
)"

issue "A6: Mobil uygulama v1 (roller, grafik, alarm, eşikler)" "aşama,mobil,backend" "$(cat <<'MD'
- [ ] Giriş, cihaz listesi, cihaz detayı, komutlar
- [ ] Roller: yönetici / operatör / izleyici
- [ ] Geçmiş grafiği (24 saat / 7 gün)
- [ ] Alarm listesi ve push bildirimi (Edge Function + Database Webhook)
- [ ] Eşik ayarları (sadece yönetici)
- [ ] Android APK (EAS) üretildi, iPhone'da Expo Go ile denendi
MD
)"

issue "A7: Saha / menzil testi" "aşama,test" "$(cat <<'MD'
- [ ] Farklı mesafelerde paket başarı oranı ve RSSI ölçüldü
- [ ] Sonuçlar tablo ve grafik olarak docs/06-test-plani.md'ye yazıldı
MD
)"

issue "A8: Gerçek kontrol cihazı entegrasyonu" "aşama,firmware,test" "$(cat <<'MD'
- [ ] Danışmanla gerçek cihaz ve register haritası belirlendi
- [ ] Modbus adaptörü gerçek cihazla test edildi
MD
)"

issue "Dokümantasyon ve sunum" "dokümantasyon" "$(cat <<'MD'
- [ ] Ekip bilgileri ve danışman README'ye yazıldı
- [ ] Bitirme raporu docs/ içeriğinden derlendi
- [ ] Demo videosu ve sunum hazırlandı
MD
)"

if gh project create --owner "${REPO%%/*}" --title "LoRa Su Yönetim Sistemi" --format json -q .number > /tmp/lwm-project 2>/dev/null; then
  NUM=$(cat /tmp/lwm-project)
  gh project link "$NUM" --owner "${REPO%%/*}" --repo "$REPO" >/dev/null || true
  for n in $(gh issue list --repo "$REPO" --limit 50 --json url -q '.[].url'); do
    gh project item-add "$NUM" --owner "${REPO%%/*}" --url "$n" >/dev/null
  done
  echo "Board created: https://github.com/users/${REPO%%/*}/projects/$NUM (switch the view to Board)"
else
  echo "Board not created (run: gh auth refresh -s project). Issues are ready."
fi
