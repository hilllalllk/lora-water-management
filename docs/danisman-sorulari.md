# Danışmana Sorulacaklar

Cevaplanınca tarih ve cevabı yanına yazın, gerekiyorsa ilgili ADR'yi güncelleyin.

| # | Soru | Neden önemli | Cevap |
|---|---|---|---|
| 1 | Proje başlığındaki "LoRaWAN" ifadesi "LoRa" olarak güncellenebilir mi? | Sistem LoRaWAN ağ sunucusu kullanmıyor ([ADR-0001](adr/0001-duz-lora.md)) | **Cevaplandı (18.10.2026):** Başlık "LoRaWAN Tabanlı Mobil Su Yönetim Sisteminin Geliştirilmesi" olarak onaylandı. Teknik olarak düz LoRa kullanıldığı raporda gerekçesiyle açıklanır. |
| 2 | 433 MHz'te 10 mW sınırı için okulun/laboratuvarın bir yönergesi var mı? Saha testine (A7) izin var mı? | E32-433T20D en düşük kademede ≈ 10 mW; Türkiye'deki sınır BTK'dan teyit edilmedi ([04 §4](04-donanim.md#4-yasal-uyarı-çıkış-gücü-ve-frekans)) | Açık |
| 3 | Gerçek saha cihazı hangi arayüzle bağlanacak? | Uç düğüm adaptörü buna göre seçilir | **Kısmen cevaplandı (18.10.2026):** Cihazlar sahada var, iletişim kısmı esnek. Modbus RTU (RS-485) adaptörü hazırlandı ([ADR-0008](adr/0008-saha-cihazi-modbus.md)) |
| 4 | Gerçek kontrol cihazının Modbus register listesi (adres, birim, slave ID, baud) ve model/kılavuzu nedir? | `modbus_map.h` şu an yer tutucu; A8 bu listeye bağlı | Açık |
| 5 | Radyo şifrelemesi bitirme kapsamında beklenir mi? | v2 planında ([08](08-guvenlik.md)) | Açık |
