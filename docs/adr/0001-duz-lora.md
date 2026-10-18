# ADR-0001: Radyo katmanı olarak düz LoRa (E32)

- **Tarih:** 14.10.2026
- **Durum:** Kabul edildi

## Bağlam
Ön tasarımda Ebyte E32-433T30D modülleri planlanmıştı. Araştırma ([01](../01-mimari-arastirmasi.md)) bu modüllerin LoRaWAN değil, UART tabanlı düz LoRa modülü olduğunu gösterdi. ESP32 ile yapılan tek kanallı "LoRaWAN gateway"leri de topluluk tarafından gerçek LoRaWAN gateway'i sayılmıyor.

## Seçenekler
1. **Düz LoRa:** E32 düğümler + ESP32/E32 gateway, ağ sunucusu yok.
2. **Tam LoRaWAN:** 868 MHz LoRaWAN düğüm radyosu, 8 kanallı gateway, ChirpStack.
3. **Aşamalı:** Önce E32 prototip, sonra LoRaWAN'a geçiş.

## Karar
**Düz LoRa** (ekip kararı).

## Sonuçlar
- Donanım elde, ek maliyet yok; komut sayısında ağ kaynaklı sınır yok.
- LoRaWAN'ın verdiği kimlik, tekrar ayıklama, onay ve şifreleme işlevleri bizim tarafımızdan yazılmalı ([ADR-0004](0004-kendi-protokolumuz.md)).
- Proje başlığındaki "LoRaWAN" ifadesi danışmanla netleştirilmeli. **Güncelleme (18.10.2026):** Danışman başlığı "LoRaWAN Tabanlı Mobil Su Yönetim Sisteminin Geliştirilmesi" olarak onayladı; düz LoRa kararı aynen geçerli, gerekçesi raporda açıklanır.
- 433 MHz bandında yasal çıkış gücü kontrol edilmeli ([04 §4](../04-donanim.md#4-yasal-uyarı-çıkış-gücü-ve-frekans)). **Güncelleme (18.10.2026):** Modül E32-433T30D (~1 W) yerine E32-433T20D (100 mW, en düşük kademe ≈ 10 mW) seçildi; gateway yeniden ESP32 + E32 ([ADR-0007](0007-gateway-yeniden-esp32.md)).
