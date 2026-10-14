# 08 · Güvenlik

Bu bölüm sistemin nerede korunduğunu ve prototipte **bilerek** açık bırakılan noktaları listeler. Bitirme raporunda "kısıtlar" bölümüne aynen aktarılabilir.

## 1. Katman katman

| Katman | Koruma (v1) | Açık nokta | Plan |
|---|---|---|---|
| Radyo (LoRa) | CRC ile bozuk paket reddi, SEQ ile tekrar ayıklama | **Şifreleme ve kimlik doğrulama yok.** Aynı frekanstaki biri paketleri okuyabilir ve sahte COMMAND gönderebilir | v2: AES-128-CCM ile şifreleme + 4 baytlık MIC, düğüm başına anahtar (çerçeve ≤ 58 bayt sınırında kalacak şekilde) |
| Gateway ↔ Supabase | HTTPS, gateway'e özel kullanıcı, kısa ömürlü JWT | **Sertifika doğrulaması kapalı** (`setInsecure`), ortadaki adam saldırısına açık | Teslimden önce Supabase kök sertifikası `setCACert` ile eklenecek |
| Veritabanı | Row Level Security: telemetriyi ve komut durumunu sadece gateway yazar, kullanıcı sadece `pending` komut ekler, anon hiçbir şey göremez | Giriş yapmış her kullanıcı tüm cihazları görür ve komut verebilir | v2: kullanıcı–cihaz yetki tablosu |
| Mobil uygulama | Supabase Auth, oturum cihazda saklanır | Kayıt ekranı yok (kullanıcıları yönetici ekler) | Bilinçli tercih |
| Gizli bilgiler | `secrets.h` ve `.env.local` repoya girmez, `service_role` anahtarı hiçbir istemcide yok | Gateway'in şifresi firmware'e gömülü | Kabul edilebilir (fiziksel erişim gerektirir) |

## 2. Kurallar

1. `service_role` / secret anahtar **hiçbir zaman** firmware'e, uygulamaya veya repoya girmez.
2. `secrets.h`, `.env.local` dosyaları commit edilmez (`.gitignore`). Yanlışlıkla commit edilirse anahtar Supabase panelinden **hemen yenilenir**; sadece dosyayı silmek yetmez, Git geçmişinde kalır.
3. Yeni tablo eklenirse RLS açılır ve `backend/supabase/tests/rls_test.sql` dosyasına test eklenir.

## 3. Komut güvenliği

- Vana komutları **idempotent**tir: aynı `cmdId` iki kez gelirse vana bir kez değişir.
- Uygulama, önceki komut bitmeden yeni komut göndermez; gerçek durum sadece düğümün `CMD_RESULT` cevabıyla gösterilir.
- Gateway, cihaza ulaşamazsa komutu `failed` (TIMEOUT) yapar; uygulama kullanıcıya bunu gösterir. Komutlar sonradan "sessizce" çalışmaz.
