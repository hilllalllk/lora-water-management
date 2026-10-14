// Veritabanı satır tipleri: backend/supabase/migrations/20261014000000_init.sql

export type ValveState = 0 | 1 | 2 | 3;

export interface Device {
  id: string;
  address: number;
  name: string;
  type: string;
  fw: string | null;
  period_s: number;
  level_mm: number | null;
  valve: ValveState | null;
  pressure_cbar: number | null;
  flow_dl_min: number | null;
  alarms: number;
  last_seen: string | null;
}

export type CommandOp = 'VANA_AC' | 'VANA_KAPAT' | 'CIHAZ_ETKIN' | 'SIMDI_OKU' | 'PERIYOT_AYARLA';
export type CommandStatus = 'pending' | 'sent' | 'delivered' | 'done' | 'failed';

export interface Command {
  id: number;
  device_id: string;
  op: CommandOp;
  args: Record<string, unknown> | null;
  status: CommandStatus;
  result: number | null;
  created_at: string;
  updated_at: string;
}

export interface Gateway {
  id: string;
  fw: string | null;
  wifi_rssi: number | null;
  last_seen: string | null;
}
