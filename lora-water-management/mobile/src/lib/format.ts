import type { CommandOp, CommandStatus, Device, ValveState } from '../types';

// docs/03-protokol.md ile aynı anlamlar
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

// Sonuç kodları (firmware/lib/LoraProtocol: Result)
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

// Düğüm 3 periyot boyunca veri göndermezse çevrimdışı sayılır (docs/02 §5)
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
