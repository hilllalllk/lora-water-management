import Constants from 'expo-constants';
import * as Device from 'expo-device';
import { isRunningInExpoGo } from 'expo';
import { Platform } from 'react-native';

import { supabase } from './supabase';

// Expo push notifications for alerts (backend: Edge Function send-alert-push).
// expo-notifications is imported lazily so nothing loads where remote push is unsupported
// (Expo Go on Android since SDK 53, simulators, web).

let registeredToken: string | null = null;

function projectId(): string | undefined {
  return Constants.expoConfig?.extra?.eas?.projectId ?? Constants.easConfig?.projectId ?? undefined;
}

export function pushSupported(): { ok: boolean; reason?: string } {
  if (Platform.OS === 'web') return { ok: false, reason: 'web' };
  if (!Device.isDevice) return { ok: false, reason: 'simulator/emulator' };
  if (Platform.OS === 'android' && isRunningInExpoGo()) {
    return { ok: false, reason: 'Expo Go on Android has no remote push (use a development build or APK)' };
  }
  if (!projectId()) return { ok: false, reason: 'no EAS projectId in app.json (run `eas init`)' };
  return { ok: true };
}

// Shows notifications while the app is open and creates the Android channel used by the server.
async function setupNotifications() {
  const Notifications = await import('expo-notifications');
  Notifications.setNotificationHandler({
    handleNotification: async () => ({
      shouldShowBanner: true,
      shouldShowList: true,
      shouldPlaySound: true,
      shouldSetBadge: false,
    }),
  });
  if (Platform.OS === 'android') {
    // Must exist before the permission prompt on Android 13+
    await Notifications.setNotificationChannelAsync('alarms', {
      name: 'Alarmlar',
      importance: Notifications.AndroidImportance.HIGH,
      vibrationPattern: [0, 250, 250, 250],
      lightColor: '#B42318',
    });
  }
  return Notifications;
}

// Asks for permission, gets the Expo push token and stores it for the signed-in user.
// Returns the token, or null when push is unavailable or denied (never throws for that).
export async function registerForPush(): Promise<string | null> {
  const support = pushSupported();
  if (!support.ok) {
    console.log('[push] skipped:', support.reason);
    return null;
  }
  const Notifications = await setupNotifications();

  let { status } = await Notifications.getPermissionsAsync();
  if (status !== 'granted') {
    ({ status } = await Notifications.requestPermissionsAsync());
  }
  if (status !== 'granted') {
    console.log('[push] permission denied');
    return null;
  }

  const { data: token } = await Notifications.getExpoPushTokenAsync({ projectId: projectId() });
  const { error } = await supabase.rpc('register_push_token', { p_token: token, p_platform: Platform.OS });
  if (error) throw new Error(error.message);
  registeredToken = token;
  return token;
}

// Opens the device when the user taps an alert notification. Returns an unsubscribe function.
export function onNotificationTap(handler: (deviceId: string) => void): () => void {
  if (!pushSupported().ok) return () => {};
  let remove: (() => void) | null = null;
  let cancelled = false;
  import('expo-notifications').then((Notifications) => {
    if (cancelled) return;
    const sub = Notifications.addNotificationResponseReceivedListener((response) => {
      const deviceId = response.notification.request.content.data?.deviceId;
      if (typeof deviceId === 'string') handler(deviceId);
    });
    remove = () => sub.remove();
  });
  return () => {
    cancelled = true;
    remove?.();
  };
}

// Removes this phone's token (while still signed in), then signs out.
export async function signOut() {
  if (registeredToken) {
    await supabase.from('push_tokens').delete().eq('token', registeredToken);
    registeredToken = null;
  }
  await supabase.auth.signOut();
}
