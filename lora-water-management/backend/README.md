# Backend (Supabase)

```
backend/supabase/
├── migrations/20261014000000_init.sql   # tablolar, tetikleyiciler, RLS, Realtime
├── seed.sql                             # örnek cihazlar
└── tests/
    ├── supabase_stub.sql                # düz PostgreSQL'de Supabase'in auth şemasını taklit eder (SADECE TEST)
    ├── rls_test.sql                     # yetki ve tetikleyici testleri
    └── run.sh                           # testleri çalıştırır
```

## Tablolar

| Tablo | İçerik | Kim yazar |
|---|---|---|
| `devices` | Düğümler ve son durumları | Yönetici (ekleme), gateway (heartbeat), tetikleyici (son ölçüm) |
| `telemetry` | Ölçüm geçmişi | Sadece gateway |
| `commands` | Komut kuyruğu ve durumları | Kullanıcı (`pending` ekler), gateway (durum günceller) |
| `gateways` | Gateway sağlığı | Sadece gateway |

Kurulum: [docs/05-kurulum.md §1](../docs/05-kurulum.md#1-supabase-projesi). Tasarım: [docs/02 §6](../docs/02-sistem-mimarisi.md#6-supabase-veri-modeli).

## Testler

```bash
# Yerel PostgreSQL ile (PGHOST, PGUSER, PGPASSWORD ayarlı olmalı)
bash backend/supabase/tests/run.sh
```

Şemayı değiştirirseniz yeni bir migration dosyası ekleyin (`YYYYMMDDHHMMSS_aciklama.sql`) ve `rls_test.sql` dosyasına test yazın.
