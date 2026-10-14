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
- Proje başlığındaki "LoRaWAN" ifadesi danışmanla netleştirilmeli.
- 433 MHz bandında yasal çıkış gücü kontrol edilmeli ([04 §4](../04-donanim.md)).
