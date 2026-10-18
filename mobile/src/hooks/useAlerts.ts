import { useEffect, useMemo, useState } from 'react';

import { supabase } from '../lib/supabase';
import type { Alert } from '../types';

const RECENT = 100;

// Open alerts first, then newest first
function sortAlerts(list: Alert[]): Alert[] {
  return [...list].sort((a, b) => {
    const openA = a.resolved_at == null ? 0 : 1;
    const openB = b.resolved_at == null ? 0 : 1;
    if (openA !== openB) return openA - openB;
    return b.id - a.id;
  });
}

// Reads all open alerts plus the most recent ones, and keeps them live via Realtime.
export function useAlerts() {
  const [alerts, setAlerts] = useState<Alert[]>([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    let active = true;

    Promise.all([
      supabase.from('alerts').select('*').is('resolved_at', null).order('id', { ascending: false }),
      supabase.from('alerts').select('*').order('id', { ascending: false }).limit(RECENT),
    ]).then(([open, recent]) => {
      if (!active) return;
      const byId = new Map<number, Alert>();
      for (const a of [...(recent.data ?? []), ...(open.data ?? [])] as Alert[]) byId.set(a.id, a);
      setAlerts(sortAlerts([...byId.values()]));
      setLoading(false);
    });

    const channel = supabase
      .channel('alerts-changes')
      .on('postgres_changes', { event: '*', schema: 'public', table: 'alerts' }, (payload) => {
        setAlerts((current) => {
          if (payload.eventType === 'DELETE') {
            const old = payload.old as Partial<Alert>;
            return current.filter((a) => a.id !== old.id);
          }
          const row = payload.new as Alert;
          return sortAlerts([row, ...current.filter((a) => a.id !== row.id)]);
        });
      })
      .subscribe();

    return () => {
      active = false;
      supabase.removeChannel(channel);
    };
  }, []);

  // device_id -> number of open alerts
  const openByDevice = useMemo(() => {
    const m: Record<string, number> = {};
    for (const a of alerts) if (a.resolved_at == null) m[a.device_id] = (m[a.device_id] ?? 0) + 1;
    return m;
  }, [alerts]);

  const openCount = useMemo(() => alerts.filter((a) => a.resolved_at == null).length, [alerts]);

  return { alerts, loading, openByDevice, openCount };
}
