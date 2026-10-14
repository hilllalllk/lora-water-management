import { useCallback, useEffect, useState } from 'react';

import { supabase } from '../lib/supabase';
import type { Command, CommandOp } from '../types';

const HISTORY = 10;

// Bir cihazın son komutlarını okur, değişiklikleri canlı dinler ve komut gönderir.
export function useCommands(deviceId: string) {
  const [commands, setCommands] = useState<Command[]>([]);

  useEffect(() => {
    let active = true;

    supabase
      .from('commands')
      .select('*')
      .eq('device_id', deviceId)
      .order('id', { ascending: false })
      .limit(HISTORY)
      .then(({ data }) => {
        if (active && data) setCommands(data as Command[]);
      });

    const channel = supabase
      .channel(`commands-${deviceId}`)
      .on(
        'postgres_changes',
        { event: '*', schema: 'public', table: 'commands', filter: `device_id=eq.${deviceId}` },
        (payload) => {
          if (payload.eventType === 'DELETE') return;
          const row = payload.new as Command;
          setCommands((current) => {
            const rest = current.filter((c) => c.id !== row.id);
            return [row, ...rest].sort((a, b) => b.id - a.id).slice(0, HISTORY);
          });
        },
      )
      .subscribe();

    return () => {
      active = false;
      supabase.removeChannel(channel);
    };
  }, [deviceId]);

  // Komut 'pending' olarak eklenir; gateway alıp sahaya iletir (docs/02 §4.6).
  const send = useCallback(
    async (op: CommandOp, args?: Record<string, unknown>) => {
      const { data, error } = await supabase
        .from('commands')
        .insert({ device_id: deviceId, op, args: args ?? null })
        .select()
        .single();
      if (error) throw new Error(error.message);
      setCommands((current) => [data as Command, ...current.filter((c) => c.id !== data.id)]);
      return data as Command;
    },
    [deviceId],
  );

  return { commands, send };
}
