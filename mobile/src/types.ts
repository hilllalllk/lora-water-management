// Database row types: backend/supabase/migrations/20261014000000_init.sql
// and 20261018000000_roles_alarms.sql

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
  // Alarm thresholds (null = no check); only admins can change them
  level_low_mm: number | null;
  level_high_mm: number | null;
  pressure_low_cbar: number | null;
  pressure_high_cbar: number | null;
  offline_after_s: number | null;
}

export type DeviceThresholds = Pick<
  Device,
  'level_low_mm' | 'level_high_mm' | 'pressure_low_cbar' | 'pressure_high_cbar' | 'offline_after_s'
>;

export interface TelemetryRow {
  received_at: string;
  level_mm: number | null;
  pressure_cbar: number | null;
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

export type AppRole = 'admin' | 'operator' | 'viewer';

export interface Profile {
  id: string;
  full_name: string | null;
  email: string | null;
  role: AppRole;
  created_at: string;
}

export type AlertKind =
  | 'level_low'
  | 'level_high'
  | 'pressure_low'
  | 'pressure_high'
  | 'offline'
  | 'device_alarm';

export interface Alert {
  id: number;
  device_id: string;
  kind: AlertKind;
  message: string;
  value: number | null;
  created_at: string;
  resolved_at: string | null;
}

export interface PushToken {
  token: string;
  user_id: string;
  platform: string | null;
  updated_at: string;
}
