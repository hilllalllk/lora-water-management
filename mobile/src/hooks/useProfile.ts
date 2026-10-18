import { useCallback, useEffect, useState } from 'react';
import { AppState } from 'react-native';

import { supabase } from '../lib/supabase';
import type { AppRole, Profile } from '../types';

// Reads the signed-in user's profile (role). Re-read when the app comes to the foreground,
// so a role changed by an admin takes effect without signing out.
export function useProfile(userId: string) {
  const [profile, setProfile] = useState<Profile | null>(null);
  const [loading, setLoading] = useState(true);

  const reload = useCallback(async () => {
    const { data } = await supabase.from('profiles').select('*').eq('id', userId).maybeSingle();
    setProfile((data as Profile | null) ?? null);
    setLoading(false);
  }, [userId]);

  useEffect(() => {
    reload();
    const sub = AppState.addEventListener('change', (state) => {
      if (state === 'active') reload();
    });
    return () => sub.remove();
  }, [reload]);

  // No profile row (e.g. created before the migration) behaves like a viewer
  const role: AppRole = profile?.role ?? 'viewer';
  return {
    profile,
    role,
    loading,
    reload,
    isAdmin: role === 'admin',
    canControl: role === 'admin' || role === 'operator',
  };
}
