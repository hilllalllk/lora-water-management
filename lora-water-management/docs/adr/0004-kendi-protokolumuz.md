# ADR-0004: E32 üzerinde kendi ikili protokolümüz

- **Tarih:** 14.10.2026
- **Durum:** Kabul edildi

## Bağlam
E32 sadece "adrese bayt gönder" hizmeti verir. Komutların güvenilir ulaşması, tekrar eden paketlerin ayıklanması ve cihazdan bağımsız veri formatı gerekiyor.

## Seçenekler
1. Metin tabanlı (ör. `VANA_AC\n`, JSON)
2. Hazır bir kütüphane protokolü
3. Küçük, ikili, sürümlü kendi protokolümüz

## Karar
**Kendi ikili protokolümüz** ([03 · Protokol](../03-protokol.md)): sabit 6 baytlık başlık, TLV telemetri, CRC16, SEQ, ACK + 3 deneme.

## Sonuçlar
- Paketler 58 bayta sığıyor, 2.4 kbps hava hızında kısa süre havada kalıyor.
- Donanımdan bağımsız C++ kütüphanesi olarak yazıldı; bilgisayarda birim testleriyle doğrulanıyor.
- Şifreleme yok; v2 planında ([08](../08-guvenlik.md)).
