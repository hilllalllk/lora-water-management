import { useCallback, useEffect, useState } from 'react';

import { supabase } from '../lib/supabase';
import type { AppRole, Profile } from '../types';

// All users' profiles (admin "Kullanıcılar" screen).
export function useProfiles() {
  const [profiles, setProfiles] = useState<Profile[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  const reload = useCallback(async () => {
    const { data, error: err } = await supabase.from('profiles').select('*').order('created_at');
    if (err) setError(err.message);
    else {
      setProfiles((data ?? []) as Profile[]);
      setError(null);
    }
    setLoading(false);
  }, []);

  useEffect(() => {
    reload();
  }, [reload]);

  const setRole = useCallback(async (id: string, role: AppRole) => {
    const { data, error: err } = await supabase
      .from('profiles')
      .update({ role })
      .eq('id', id)
      .select()
      .single();
    if (err) {
      if (err.message.includes('admin must remain')) throw new Error('En az bir yönetici kalmalıdır.');
      throw new Error(err.message);
    }
    setProfiles((current) => current.map((p) => (p.id === id ? (data as Profile) : p)));
  }, []);

  return { profiles, loading, error, reload, setRole };
}
