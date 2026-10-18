# Katkı Rehberi

## Çalışma düzeni

1. Her iş için bir **issue** açın (şablonlar: Görev, Hata). İlk kurulumda `bash scripts/github-issues.sh` aşama issue'larını ve proje panosunu oluşturur.
2. `main`'den dal açın: `ozellik/kisa-ad`, `duzeltme/kisa-ad`, `dokuman/kisa-ad`.
3. Küçük ve anlamlı commit'ler atın (aşağıya bakın).
4. Pull request açın; CI yeşil olmadan birleştirmeyin.

## Commit mesajları

Türkçe, kısa ve emir kipinde. Başına bölümü yazın:

```
firmware: gateway'e komut zaman aşımı ekle
backend: telemetry tablosuna supply_mv sütunu ekle
mobil: cihaz detayına geçmiş grafiği ekle
docs: A1 masa testi sonuçlarını yaz
```

## Değişiklik yaptıktan sonra

| Değiştirdiğiniz yer | Çalıştırın | Güncelleyin |
|---|---|---|
| `firmware/lib/LoraProtocol` | `pio test -e native` | `docs/03-protokol.md`, `tools/src/protocol/protocol.ts` (iki uygulama aynı test vektörlerini geçmeli) |
| `firmware/lib/ModbusRtu`, `include/modbus_map.h` | `pio test -e native` | `docs/04-donanim.md` |
| `tools/` | `npm test && npm run typecheck` | `docs/03` |
| `firmware/src` | `pio run -e node -e gateway` | — |
| `backend/supabase` | `bash backend/supabase/tests/run.sh` | `docs/02 §6`, `mobile/src/types.ts` |
| `mobile/` | `npm run typecheck` | — |
| Önemli bir karar | — | `docs/adr/` altına yeni kayıt |

Her çalışma oturumundan sonra [geliştirme günlüğüne](docs/07-gelistirme-gunlugu.md) kısa bir not ekleyin.

## Dil kuralı

Kod yorumları ve log mesajları **İngilizce**, `docs/` altındaki dokümanlar **Türkçe**, mobil arayüz metinleri **Türkçe**, README iki dilli (README.md + README.en.md). Bkz. [ADR-0006](docs/adr/0006-bilesen-dilleri.md).

## Asla

- `firmware/include/secrets.h`, `mobile/.env.local` veya Supabase `service_role` anahtarını commit etmeyin ([docs/08](docs/08-guvenlik.md)).
