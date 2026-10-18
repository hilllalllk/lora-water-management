import { useEffect, useState } from 'react';
import { ActivityIndicator, Alert, Pressable, StyleSheet, Text, TextInput, View } from 'react-native';

import { supabase } from '../lib/supabase';
import { colors, spacing } from '../theme';
import type { Device, DeviceThresholds } from '../types';

// Form values are strings in user units: mm, bar, minutes. Empty = no check.
interface Form {
  levelLow: string;
  levelHigh: string;
  pressureLow: string;
  pressureHigh: string;
  offlineMin: string;
}

function toForm(d: Device): Form {
  const s = (v: number | null, f = (x: number) => String(x)) => (v == null ? '' : f(v));
  return {
    levelLow: s(d.level_low_mm),
    levelHigh: s(d.level_high_mm),
    pressureLow: s(d.pressure_low_cbar, (x) => (x / 100).toString().replace('.', ',')),
    pressureHigh: s(d.pressure_high_cbar, (x) => (x / 100).toString().replace('.', ',')),
    offlineMin: s(d.offline_after_s, (x) => String(Math.round(x / 60))),
  };
}

// Parses "1,5" or "1.5"; returns undefined for invalid input, null for empty
function parse(text: string): number | null | undefined {
  const t = text.trim().replace(',', '.');
  if (t === '') return null;
  const n = Number(t);
  return Number.isFinite(n) && n >= 0 ? n : undefined;
}

function toThresholds(f: Form): DeviceThresholds | string {
  const ll = parse(f.levelLow);
  const lh = parse(f.levelHigh);
  const pl = parse(f.pressureLow);
  const ph = parse(f.pressureHigh);
  const off = parse(f.offlineMin);
  if ([ll, lh, pl, ph, off].some((v) => v === undefined)) return 'Lütfen geçerli (pozitif) sayılar girin.';
  if (ll != null && lh != null && ll >= lh) return 'Alt seviye eşiği üst eşikten küçük olmalı.';
  if (pl != null && ph != null && pl >= ph) return 'Alt basınç eşiği üst eşikten küçük olmalı.';
  if (off != null && off < 1) return 'Veri yok süresi en az 1 dakika olmalı.';
  return {
    level_low_mm: ll != null ? Math.round(ll) : null,
    level_high_mm: lh != null ? Math.round(lh) : null,
    pressure_low_cbar: pl != null ? Math.round(pl * 100) : null,
    pressure_high_cbar: ph != null ? Math.round(ph * 100) : null,
    offline_after_s: off != null ? Math.round(off * 60) : null,
  };
}

// Admin-only "Eşik ayarları": alarm thresholds of one device.
export function ThresholdEditor({ device }: { device: Device }) {
  const [open, setOpen] = useState(false);
  const [form, setForm] = useState<Form>(() => toForm(device));
  const [saving, setSaving] = useState(false);

  // Re-sync when the device changes elsewhere, unless the form is being edited
  useEffect(() => {
    if (!open) setForm(toForm(device));
  }, [device, open]);

  async function save() {
    const values = toThresholds(form);
    if (typeof values === 'string') {
      Alert.alert('Geçersiz değer', values);
      return;
    }
    setSaving(true);
    const { error } = await supabase.from('devices').update(values).eq('id', device.id).select('id').single();
    setSaving(false);
    if (error) {
      Alert.alert('Kaydedilemedi', error.message);
      return;
    }
    setOpen(false);
    Alert.alert('Kaydedildi', 'Eşik ayarları güncellendi.');
  }

  const field = (key: keyof Form, label: string, hint: string) => (
    <View style={styles.field}>
      <Text style={styles.label}>{label}</Text>
      <TextInput
        style={styles.input}
        value={form[key]}
        onChangeText={(t) => setForm((f) => ({ ...f, [key]: t }))}
        keyboardType="decimal-pad"
        placeholder={hint}
      />
    </View>
  );

  return (
    <View>
      <Pressable onPress={() => setOpen((o) => !o)} style={styles.header}>
        <Text style={styles.section}>Eşik ayarları</Text>
        <Text style={styles.link}>{open ? 'Kapat' : 'Düzenle'}</Text>
      </Pressable>
      {!open && (
        <Text style={styles.summary}>
          Seviye {device.level_low_mm ?? '–'} / {device.level_high_mm ?? '–'} mm · Basınç{' '}
          {device.pressure_low_cbar != null ? (device.pressure_low_cbar / 100).toFixed(2) : '–'} /{' '}
          {device.pressure_high_cbar != null ? (device.pressure_high_cbar / 100).toFixed(2) : '–'} bar · Veri yok{' '}
          {device.offline_after_s != null ? `${Math.round(device.offline_after_s / 60)} dk` : '–'}
        </Text>
      )}
      {open && (
        <View style={styles.card}>
          <View style={styles.row}>
            {field('levelLow', 'Alt seviye (mm)', 'boş = yok')}
            {field('levelHigh', 'Üst seviye (mm)', 'boş = yok')}
          </View>
          <View style={styles.row}>
            {field('pressureLow', 'Alt basınç (bar)', 'örn. 0,5')}
            {field('pressureHigh', 'Üst basınç (bar)', 'örn. 4')}
          </View>
          <View style={styles.row}>{field('offlineMin', 'Veri yok alarmı (dk)', 'örn. 10')}</View>
          <Text style={styles.summary}>Boş bırakılan eşik kontrol edilmez.</Text>
          <Pressable style={[styles.button, saving && { opacity: 0.5 }]} onPress={save} disabled={saving}>
            {saving ? <ActivityIndicator color="#fff" /> : <Text style={styles.buttonText}>Kaydet</Text>}
          </Pressable>
        </View>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginTop: spacing.xl,
    marginBottom: spacing.sm,
  },
  section: { fontSize: 17, fontWeight: '600', color: colors.text },
  link: { color: colors.primary, fontSize: 15 },
  summary: { color: colors.muted, marginBottom: spacing.sm },
  card: { backgroundColor: colors.card, borderRadius: 12, borderWidth: 1, borderColor: colors.border, padding: spacing.md },
  row: { flexDirection: 'row', gap: spacing.md },
  field: { flex: 1, marginBottom: spacing.md },
  label: { color: colors.muted, fontSize: 13, marginBottom: 4 },
  input: {
    borderWidth: 1,
    borderColor: colors.border,
    borderRadius: 8,
    padding: spacing.sm,
    fontSize: 16,
    color: colors.text,
    backgroundColor: colors.bg,
  },
  button: { backgroundColor: colors.primary, borderRadius: 8, padding: spacing.md, alignItems: 'center' },
  buttonText: { color: '#fff', fontWeight: '600' },
});
