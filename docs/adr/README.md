# Mimari Karar Kayıtları (ADR)

Projede alınan önemli teknik kararlar, gerekçeleriyle birlikte burada tutulur. Bir karar değişirse eski kayıt silinmez; durumu "Yerini aldı: ADR-XXXX" yapılır ve yeni kayıt eklenir. Güncel kararların özeti: [09 · Teknoloji ve Donanım Kararları](../09-teknoloji-kararlari.md).

| No | Karar | Durum |
|---|---|---|
| [0001](0001-duz-lora.md) | Radyo katmanı olarak düz LoRa (E32) | Kabul edildi |
| [0002](0002-supabase.md) | Bulut olarak Supabase | Kabul edildi |
| [0003](0003-gateway-rest-sorgulama.md) | Gateway komutları REST ile sorgular | Kabul edildi (ADR-0007 ile yeniden) |
| [0004](0004-kendi-protokolumuz.md) | E32 üzerinde kendi ikili protokolümüz | Kabul edildi |
| [0005](0005-raspberry-pi-nodejs-gateway.md) | Gateway Raspberry Pi üzerinde Node.js + TypeScript ile çalışır | Yerini aldı: ADR-0007 |
| [0006](0006-bilesen-dilleri.md) | Bileşen başına programlama dili ve doküman dili | Kabul edildi (18.10.2026'da güncellendi) |
| [0007](0007-gateway-yeniden-esp32.md) | Gateway yeniden ESP32 + E32 + Wi-Fi | Kabul edildi |
| [0008](0008-saha-cihazi-modbus.md) | Saha cihazı adaptörleri ve Modbus RTU | Kabul edildi |

Şablon: [0000-sablon.md](0000-sablon.md)
