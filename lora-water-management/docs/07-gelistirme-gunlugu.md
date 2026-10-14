# 07 · Geliştirme Günlüğü

Her çalışma oturumundan sonra kısa bir kayıt ekleyin. Bitirme raporunda "proje süreci" bölümü ve danışman görüşmeleri için kullanılır. En yeni kayıt en üstte.

## Şablon

```markdown
## GG.AA.YYYY – Kısa başlık
**Kim:** ...
**Ne yapıldı:** ...
**Sorunlar / öğrenilenler:** ...
**Sonraki adım:** ...
```

---

## 14.10.2026 – Proje başlangıcı, mimari ve repo iskeleti

**Ne yapıldı:**
- Benzer LoRa/LoRaWAN projeleri araştırıldı ([01](01-mimari-arastirmasi.md)).
- Düz LoRa (E32) ile ilerleme kararı alındı ([ADR-0001](adr/0001-duz-lora.md)).
- Bulut olarak Firebase yerine Supabase seçildi ([ADR-0002](adr/0002-supabase.md)).
- Sistem mimarisi ve LoRa protokolü tasarlandı ([02](02-sistem-mimarisi.md), [03](03-protokol.md)).
- Repo iskeleti kuruldu: protokol kütüphanesi ve testleri, düğüm/gateway firmware'i, Supabase şeması ve RLS testleri, mobil uygulama, CI.

**Açık konular:**
- Başlıktaki "LoRaWAN" ifadesi ve E32 çıkış gücü danışmanla konuşulacak ([danışman soruları](danisman-sorulari.md)).
- Firmware henüz gerçek kartta denenmedi (A1).

**Sonraki adım:** A1 masa testi.
