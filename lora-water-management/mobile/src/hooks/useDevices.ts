import { useEffect, useState } from 'react';

import { supabase } from '../lib/supabase';
import type { Device } from '../types';

// Tüm cihazları okur ve devices tablosundaki değişiklikleri canlı (Realtime) dinler.
export function useDevices() {
  const [devices, setDevices] = useState<Device[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    let active = true;

    supabase
      .from('devices')
      .select('*')
      .order('name')
      .then(({ data, error: err }) => {
        if (!active) return;
        if (err) setError(err.message);
        else setDevices((data ?? []) as Device[]);
        setLoading(false);
      });

    const channel = supabase
      .channel('devices-changes')
      .on('postgres_changes', { event: '*', schema: 'public', table: 'devices' }, (payload) => {
        setDevices((current) => {
          if (payload.eventType === 'DELETE') {
            const old = payload.old as Partial<Device>;
            return current.filter((d) => d.id !== old.id);
          }
          const row = payload.new as Device;
          const exists = current.some((d) => d.id === row.id);
          const next = exists ? current.map((d) => (d.id === row.id ? row : d)) : [...current, row];
          return next.sort((a, b) => a.name.localeCompare(b.name, 'tr'));
        });
      })
      .subscribe();

    return () => {
      active = false;
      supabase.removeChannel(channel);
    };
  }, []);

  return { devices, loading, error };
}
