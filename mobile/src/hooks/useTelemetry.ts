import { useEffect, useState } from 'react';

import { supabase } from '../lib/supabase';
import type { TelemetryRow } from '../types';

export type HistoryRange = '24h' | '7d';

export const RANGE_MS: Record<HistoryRange, number> = {
  '24h': 24 * 3600 * 1000,
  '7d': 7 * 24 * 3600 * 1000,
};

const PAGE = 1000; // Supabase's default max rows per request
const MAX_ROWS = 20000; // safety cap; the newest rows are kept
const BUCKETS = 120; // points drawn per chart

export interface Point {
  t: number; // ms since epoch
  v: number;
}

export interface History {
  from: number;
  to: number;
  level: (Point | null)[]; // null = no data in that bucket (gap in the line)
  pressure: (Point | null)[];
  hasPressure: boolean;
  count: number; // raw rows read
}

// Averages rows into fixed time buckets so the chart stays light with thousands of rows.
function downsample(
  rows: TelemetryRow[],
  from: number,
  to: number,
  pick: (r: TelemetryRow) => number | null,
): (Point | null)[] {
  const size = (to - from) / BUCKETS;
  const sum = new Array<number>(BUCKETS).fill(0);
  const n = new Array<number>(BUCKETS).fill(0);
  for (const r of rows) {
    const v = pick(r);
    if (v == null) continue;
    const i = Math.min(BUCKETS - 1, Math.max(0, Math.floor((Date.parse(r.received_at) - from) / size)));
    sum[i] += v;
    n[i] += 1;
  }
  return sum.map((s, i) => (n[i] ? { t: from + (i + 0.5) * size, v: s / n[i] } : null));
}

// Reads a device's telemetry for the selected range (paged) and downsamples it client-side.
export function useTelemetryHistory(deviceId: string, range: HistoryRange, refreshKey?: unknown) {
  const [history, setHistory] = useState<History | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    let active = true;
    const to = Date.now();
    const from = to - RANGE_MS[range];
    setLoading(true);

    (async () => {
      const rows: TelemetryRow[] = [];
      for (let offset = 0; offset < MAX_ROWS; offset += PAGE) {
        const { data, error: err } = await supabase
          .from('telemetry')
          .select('received_at, level_mm, pressure_cbar')
          .eq('device_id', deviceId)
          .gte('received_at', new Date(from).toISOString())
          .order('received_at', { ascending: false })
          .range(offset, offset + PAGE - 1);
        if (err) throw new Error(err.message);
        rows.push(...((data ?? []) as TelemetryRow[]));
        if (!data || data.length < PAGE) break;
      }
      return rows;
    })()
      .then((rows) => {
        if (!active) return;
        setHistory({
          from,
          to,
          level: downsample(rows, from, to, (r) => r.level_mm),
          pressure: downsample(rows, from, to, (r) => (r.pressure_cbar != null ? r.pressure_cbar / 100 : null)),
          hasPressure: rows.some((r) => r.pressure_cbar != null),
          count: rows.length,
        });
        setError(null);
      })
      .catch((e: unknown) => {
        if (active) setError(e instanceof Error ? e.message : String(e));
      })
      .finally(() => {
        if (active) setLoading(false);
      });

    return () => {
      active = false;
    };
  }, [deviceId, range, refreshKey]);

  return { history, loading, error };
}
