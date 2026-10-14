# Katkı Rehberi

## Çalışma düzeni

1. Her iş için bir **issue** açın (şablonlar: Görev, Hata).
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
| `firmware/lib/LoraProtocol` | `pio test -e native` | `docs/03-protokol.md`, `tools/frame_tool.py` |
| `firmware/src` | `pio run` | — |
| `backend/supabase` | `bash backend/supabase/tests/run.sh` | `docs/02 §6`, `mobile/src/types.ts` |
| `mobile/` | `npm run typecheck` | — |
| Önemli bir karar | — | `docs/adr/` altına yeni kayıt |

Her çalışma oturumundan sonra [geliştirme günlüğüne](docs/07-gelistirme-gunlugu.md) kısa bir not ekleyin.

## Asla

- `secrets.h`, `.env.local` veya Supabase `service_role` anahtarını commit etmeyin ([docs/08](docs/08-guvenlik.md)).
