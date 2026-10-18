import type { AlertKind, AppRole, CommandOp, CommandStatus, Device, ValveState } from '../types';

// Same meanings as docs/03-protokol.md
export const VALVE_LABEL: Record<ValveState, string> = {
  0: 'Kapalı',
  1: 'Açık',
  2: 'Hareket ediyor',
  3: 'Hata',
};

export const OP_LABEL: Record<CommandOp, string> = {
  VANA_AC: 'Vana aç',
  VANA_KAPAT: 'Vana kapat',
  CIHAZ_ETKIN: 'Cihaz etkin/devre dışı',
  SIMDI_OKU: 'Şimdi oku',
  PERIYOT_AYARLA: 'Periyot ayarla',
};

export const STATUS_LABEL: Record<CommandStatus, string> = {
  pending: 'Bekliyor',
  sent: 'Gönderildi',
  delivered: 'Cihaza ulaştı',
  done: 'Tamamlandı',
  failed: 'Başarısız',
};

// Result codes (firmware/lib/LoraProtocol: Result)
export const RESULT_LABEL: Record<number, string> = {
  0: 'Başarılı',
  1: 'Bilinmeyen komut',
  2: 'Cihaz hatası',
  3: 'Zaman aşımı (cihaza ulaşılamadı)',
  4: 'Zaten bu durumda',
};

export const ALARM_BITS: { bit: number; label: string }[] = [
  { bit: 1 << 0, label: 'Taşma' },
  { bit: 1 << 1, label: 'Kuru çalışma' },
  { bit: 1 << 2, label: 'Sensör hatası' },
];

export function alarmLabels(alarms: number): string[] {
  return ALARM_BITS.filter((a) => (alarms & a.bit) !== 0).map((a) => a.label);
}

// A node is considered offline if it sends nothing for 3 periods (docs/02 §5)
export function isOnline(d: Pick<Device, 'last_seen' | 'period_s'>, now = Date.now()): boolean {
  if (!d.last_seen) return false;
  return now - new Date(d.last_seen).getTime() < 3 * d.period_s * 1000;
}

export function timeAgo(iso: string | null, now = Date.now()): string {
  if (!iso) return 'hiç';
  const s = Math.max(0, Math.round((now - new Date(iso).getTime()) / 1000));
  if (s < 60) return `${s} sn önce`;
  if (s < 3600) return `${Math.round(s / 60)} dk önce`;
  if (s < 86400) return `${Math.round(s / 3600)} sa önce`;
  return `${Math.round(s / 86400)} gün önce`;
}

export function isFinal(status: CommandStatus): boolean {
  return status === 'done' || status === 'failed';
}

export const ROLE_LABEL: Record<AppRole, string> = {
  admin: 'Yönetici',
  operator: 'Operatör',
  viewer: 'İzleyici',
};

export const ALERT_KIND_LABEL: Record<AlertKind, string> = {
  level_low: 'Düşük seviye',
  level_high: 'Yüksek seviye',
  pressure_low: 'Düşük basınç',
  pressure_high: 'Yüksek basınç',
  offline: 'Veri yok',
  device_alarm: 'Cihaz alarmı',
};

export function formatDateTime(iso: string): string {
  const d = new Date(iso);
  const pad = (n: number) => n.toString().padStart(2, '0');
  return `${pad(d.getDate())}.${pad(d.getMonth() + 1)} ${pad(d.getHours())}:${pad(d.getMinutes())}`;
}
