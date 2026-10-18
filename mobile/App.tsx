import type { Session } from '@supabase/supabase-js';
import { StatusBar } from 'expo-status-bar';
import { useEffect, useState } from 'react';
import { ActivityIndicator, View } from 'react-native';

import { useAlerts } from './src/hooks/useAlerts';
import { useDevices } from './src/hooks/useDevices';
import { useProfile } from './src/hooks/useProfile';
import { onNotificationTap, registerForPush } from './src/lib/push';
import { supabase } from './src/lib/supabase';
import { AlertsScreen } from './src/screens/AlertsScreen';
import { DeviceDetailScreen } from './src/screens/DeviceDetailScreen';
import { DeviceListScreen } from './src/screens/DeviceListScreen';
import { LoginScreen } from './src/screens/LoginScreen';
import { UsersScreen } from './src/screens/UsersScreen';

function Loading() {
  return (
    <View style={{ flex: 1, justifyContent: 'center' }}>
      <ActivityIndicator />
    </View>
  );
}

export default function App() {
  const [session, setSession] = useState<Session | null>(null);
  const [ready, setReady] = useState(false);

  useEffect(() => {
    supabase.auth.getSession().then(({ data }) => {
      setSession(data.session);
      setReady(true);
    });
    const { data } = supabase.auth.onAuthStateChange((_event, s) => setSession(s));
    return () => data.subscription.unsubscribe();
  }, []);

  let screen;
  if (!ready) screen = <Loading />;
  else if (!session) screen = <LoginScreen />;
  // key: remount (fresh state and subscriptions) when another user signs in
  else screen = <SignedInApp key={session.user.id} userId={session.user.id} />;

  return (
    <>
      {screen}
      <StatusBar style="dark" />
    </>
  );
}

// Simple state-based navigation (no router library)
type Route =
  | { name: 'list' }
  | { name: 'detail'; deviceId: string; from: 'list' | 'alerts' }
  | { name: 'alerts' }
  | { name: 'users' };

function SignedInApp({ userId }: { userId: string }) {
  const [route, setRoute] = useState<Route>({ name: 'list' });
  const { role, loading: profileLoading } = useProfile(userId);
  const { devices, loading, error } = useDevices();
  const { alerts, loading: alertsLoading, openByDevice, openCount } = useAlerts();

  // Ask for notification permission after sign-in and store the push token
  useEffect(() => {
    registerForPush().catch((e) => console.warn('[push] registration failed:', e));
    return onNotificationTap((deviceId) => setRoute({ name: 'detail', deviceId, from: 'alerts' }));
  }, []);

  // Users screen is admin-only; leave it if the role changes
  useEffect(() => {
    if (route.name === 'users' && !profileLoading && role !== 'admin') setRoute({ name: 'list' });
  }, [route.name, role, profileLoading]);

  if (profileLoading) return <Loading />;

  switch (route.name) {
    case 'detail':
      return (
        <DeviceDetailScreen
          deviceId={route.deviceId}
          devices={devices}
          alerts={alerts}
          role={role}
          onBack={() => setRoute(route.from === 'alerts' ? { name: 'alerts' } : { name: 'list' })}
        />
      );
    case 'alerts':
      return (
        <AlertsScreen
          alerts={alerts}
          loading={alertsLoading}
          devices={devices}
          onBack={() => setRoute({ name: 'list' })}
          onSelectDevice={(deviceId) => setRoute({ name: 'detail', deviceId, from: 'alerts' })}
        />
      );
    case 'users':
      return <UsersScreen currentUserId={userId} onBack={() => setRoute({ name: 'list' })} />;
    default:
      return (
        <DeviceListScreen
          devices={devices}
          loading={loading}
          error={error}
          openByDevice={openByDevice}
          openAlertCount={openCount}
          role={role}
          onSelect={(d) => setRoute({ name: 'detail', deviceId: d.id, from: 'list' })}
          onAlerts={() => setRoute({ name: 'alerts' })}
          onUsers={() => setRoute({ name: 'users' })}
        />
      );
  }
}
