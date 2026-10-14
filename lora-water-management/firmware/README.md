# Firmware (ESP32 + E32)

PlatformIO projesi. Uç düğüm ve gateway aynı kod tabanını paylaşır.

```
firmware/
├── platformio.ini            # ortamlar: node, node2, gateway, native (testler)
├── include/
│   ├── config.h              # pinler, kanal, zamanlamalar
│   └── secrets.example.h     # -> secrets.h olarak kopyalayın (repoya girmez)
├── lib/
│   ├── LoraProtocol/         # protokol v1: çerçeve, CRC, TLV, ACK/yeniden gönderim (donanımdan bağımsız)
│   ├── E32Radio/             # E32 sürücüsü: mod, yapılandırma, sabit adresli gönderim
│   └── FieldDevice/          # saha cihazı arayüzü + SimulatedDevice + ModbusRtuDevice (taslak)
├── src/
│   ├── node/main.cpp         # uç düğüm
│   └── gateway/              # gateway: LoRa <-> Supabase köprüsü
└── test/test_protocol/       # birim testleri (pio test -e native)
```

## Komutlar

```bash
pio test -e native               # protokol testleri (kart gerekmez)
pio run -e node -t upload        # düğüm 0x1001
pio run -e node2 -t upload       # düğüm 0x1002
pio run -e gateway -t upload     # gateway (önce include/secrets.h)
pio device monitor               # seri monitör, 115200 baud
```

## Yeni düğüm ekleme

1. `platformio.ini` içine yeni ortam: `build_flags = ${esp32.build_flags} -DNODE_ADDRESS=0x1003`
2. Supabase `devices` tablosuna satır: `address = 4099` (0x1003)
3. Gateway cihaz listesini 5 dakikada bir yeniler (veya gateway'i yeniden başlatın).

## Yeni saha cihazı ekleme

`lib/FieldDevice/src/FieldDevice.h` arayüzünü uygulayan yeni bir sınıf yazın ve `src/node/main.cpp` içinde `SimulatedDevice` yerine onu kullanın. Protokol, gateway, veritabanı ve uygulama değişmez.

Ayrıntılar: [docs/02](../docs/02-sistem-mimarisi.md), [docs/03](../docs/03-protokol.md), [docs/04](../docs/04-donanim.md), [docs/05](../docs/05-kurulum.md).
