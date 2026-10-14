# Mobil Uygulama (React Native + Expo)

```
mobile/
├── App.tsx                    # ekran akışı: giriş -> liste -> detay
├── .env.example               # -> .env.local (Supabase URL ve anon/publishable anahtar)
└── src/
    ├── lib/supabase.ts        # Supabase istemcisi
    ├── lib/format.ts          # etiketler, çevrimiçi hesabı
    ├── hooks/                 # useDevices, useCommands (Realtime), useNow
    ├── screens/               # LoginScreen, DeviceListScreen, DeviceDetailScreen
    ├── components/Badge.tsx
    └── types.ts               # veritabanı satır tipleri
```

## Çalıştırma

```bash
npm install
cp .env.example .env.local    # bilgileri doldurun
npx expo start                # QR kodu Expo Go ile okutun
npm run typecheck             # tip kontrolü
```

## Komut akışı

"Vana aç" butonu `commands` tablosuna `pending` bir satır ekler. Gateway bunu alıp düğüme iletir; durum `sent → delivered → done` olarak canlı güncellenir. Önceki komut bitmeden yeni komut butonları pasiftir. Ayrıntı: [docs/03 §8](../docs/03-protokol.md#8-komut-durum-makinesi-veritabanı).

Kullanıcılar Supabase panelinden eklenir; uygulamada kayıt ekranı yoktur.
