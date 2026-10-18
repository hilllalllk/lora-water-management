# Mobile App (React Native + Expo)

```
mobile/
├── App.tsx                    # session + simple state-based navigation
├── app.json                   # Expo config (expo-notifications plugin)
├── eas.json                   # EAS Build profiles (preview = Android APK)
├── .env.example               # -> .env.local (Supabase URL and anon/publishable key)
└── src/
    ├── lib/supabase.ts        # Supabase client
    ├── lib/format.ts          # Turkish labels, online-status calculation
    ├── lib/push.ts            # push permission, Expo push token, sign-out
    ├── hooks/                 # useDevices, useCommands, useAlerts (Realtime), useProfile,
    │                          # useProfiles, useTelemetryHistory, useNow
    ├── screens/               # Login, DeviceList, DeviceDetail, Alerts, Users
    ├── components/            # Badge, LineChart (react-native-svg), HistorySection, ThresholdEditor
    └── types.ts               # database row types
```

The app's user-facing text is in Turkish.

## Features

| Feature | Who sees it |
|---|---|
| Device list with live state, open-alert badge per device | everyone |
| Device detail: metrics, 24 saat / 7 gün level and pressure chart | everyone |
| "Alarmlar" list (open first, live via Realtime), opened from the list header | everyone |
| Command buttons (Vana aç / Vana kapat / Şimdi oku) | operator, admin |
| "Eşik ayarları" on the device detail (level, pressure, offline thresholds) | admin |
| "Kullanıcılar": role picker (Yönetici / Operatör / İzleyici) | admin |

Roles come from `public.profiles` (see [backend/README.md](../backend/README.md#roles)). New users start as İzleyici (viewer); the first admin is set once in the SQL Editor. Database policies enforce the same rules, so hiding buttons is only for convenience.

The chart reads raw telemetry for the range (paged, 1000 rows per request) and averages it into 120 buckets on the phone; empty buckets show as gaps. Dashed red lines are the alarm thresholds.

## Development with Expo Go

```bash
npm install
cp .env.example .env.local    # fill in your Supabase URL and anon/publishable key
npx expo start                # scan the QR code with Expo Go (same Wi-Fi as the computer)
npm run typecheck             # type check
```

Install Expo Go from the Play Store / App Store. Everything except remote push notifications works in Expo Go.

### Windows

- Development works the same on Windows (PowerShell or cmd): install Node.js LTS, then run the commands above. Use `copy .env.example .env.local` instead of `cp`.
- If the phone cannot reach the computer (campus/corporate Wi-Fi, client isolation, firewall), use a tunnel: `npx expo start --tunnel`. Allow Node.js through Windows Firewall when asked.
- iOS: there is no iOS simulator on Windows. Use Expo Go on a real iPhone (scan the QR code with the Camera app). Android: a real phone with Expo Go, or an Android Studio emulator.
- EAS Build runs in Expo's cloud, so building the APK also works from Windows.

## Building an Android APK (EAS Build)

```bash
npm install -g eas-cli
eas login                      # free Expo account
eas init                       # creates the EAS project, writes extra.eas.projectId into app.json
# .env.local is not uploaded to EAS: store the public Supabase settings as EAS environment variables
eas env:create --environment preview --name EXPO_PUBLIC_SUPABASE_URL --value https://xxxx.supabase.co --visibility plaintext
eas env:create --environment preview --name EXPO_PUBLIC_SUPABASE_KEY --value <anon-or-publishable-key> --visibility plaintext
npm run build:apk              # = eas build -p android --profile preview
```

When the build finishes, EAS prints a link/QR code to download the `.apk`; install it on the phone (allow "install unknown apps"). Profiles in `eas.json`:

- `preview`: internal distribution, Android **APK** (`android.buildType: "apk"`) — for demos and testing.
- `production`: Android App Bundle (`.aab`) for Google Play (`npm run build:android`). Create the same env variables for the `production` environment.
- `development`: development build (needs `expo-dev-client` to be installed first: `npx expo install expo-dev-client`).

Never put the `service_role` key in the app or in EAS env variables of the app.

## Push notifications

How it works: when the database opens an alert, a Database Webhook calls the Edge Function `send-alert-push` (see [backend/README.md](../backend/README.md#push-notifications)), which sends it through the Expo push service to every token in `push_tokens`. After sign-in the app asks for notification permission, gets an Expo push token and saves it with `register_push_token()`; signing out deletes the phone's token. Tapping a notification opens the device.

Setup:

1. `eas init` so `app.json` has `extra.eas.projectId` (the app reads it from `Constants.expoConfig.extra.eas.projectId`; without it, registration is skipped).
2. Android needs Firebase Cloud Messaging (FCM V1) credentials, even through Expo's push service:
   - Create a Firebase project, add an Android app with package `com.lorawater.mobile`, download `google-services.json` into `mobile/` and add `"googleServicesFile": "./google-services.json"` under `expo.android` in `app.json`.
   - Upload an FCM V1 service account key to EAS: `eas credentials` → Android → Google Service Account → FCM V1.
   - Rebuild the APK.
3. iOS needs an Apple Developer account for push credentials in a standalone build (`eas credentials` handles it).
4. Deploy the Edge Function and create the webhook (backend README).

Limitations (as of SDK 53+; this project is on SDK 57):

- **Expo Go on Android does not support remote push notifications** — `expo-notifications` throws there, so the app skips registration. Test push with the APK (or a development build).
- Expo Go on iOS can still receive push in development, but use a real build for anything you rely on.
- Push never works on emulators/simulators; the app skips registration there.
- Check the device's logs for `[push] skipped: ...` to see why a token was not registered.

## Command flow

The "Vana aç" (open valve) button inserts a `pending` row into the `commands` table (operators and admins only). The gateway (an ESP32 on Wi-Fi) polls pending commands over the Supabase REST API every 1-2 s and forwards them to the node over LoRa; the status updates live in the app as `sent → delivered → done`. New command buttons are disabled until the previous command finishes. Details: [docs/03 §8](../docs/03-protokol.md#8-komut-durum-makinesi-veritabanı).

Users are added from the Supabase dashboard; the app has no sign-up screen.
