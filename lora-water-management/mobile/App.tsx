import type { Session } from '@supabase/supabase-js';
import { StatusBar } from 'expo-status-bar';
import { useEffect, useState } from 'react';
import { ActivityIndicator, View } from 'react-native';

import { supabase } from './src/lib/supabase';
import { DeviceDetailScreen } from './src/screens/DeviceDetailScreen';
import { DeviceListScreen } from './src/screens/DeviceListScreen';
import { LoginScreen } from './src/screens/LoginScreen';

// Basit ekran akışı: Giriş -> Cihaz listesi -> Cihaz detayı
export default function App() {
  const [session, setSession] = useState<Session | null>(null);
  const [ready, setReady] = useState(false);
  const [selectedId, setSelectedId] = useState<string | null>(null);

  useEffect(() => {
    supabase.auth.getSession().then(({ data }) => {
      setSession(data.session);
      setReady(true);
    });
    const { data } = supabase.auth.onAuthStateChange((_event, s) => {
      setSession(s);
      if (!s) setSelectedId(null);
    });
    return () => data.subscription.unsubscribe();
  }, []);

  let screen;
  if (!ready) {
    screen = (
      <View style={{ flex: 1, justifyContent: 'center' }}>
        <ActivityIndicator />
      </View>
    );
  } else if (!session) {
    screen = <LoginScreen />;
  } else if (selectedId) {
    screen = <DeviceDetailScreen deviceId={selectedId} onBack={() => setSelectedId(null)} />;
  } else {
    screen = <DeviceListScreen onSelect={(d) => setSelectedId(d.id)} />;
  }

  return (
    <>
      {screen}
      <StatusBar style="dark" />
    </>
  );
}
