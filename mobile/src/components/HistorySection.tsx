import { useState } from 'react';
import { ActivityIndicator, Pressable, StyleSheet, Text, View } from 'react-native';

import { type HistoryRange, useTelemetryHistory } from '../hooks/useTelemetry';
import { colors, spacing } from '../theme';
import type { Device } from '../types';
import { LineChart } from './LineChart';

const RANGES: { key: HistoryRange; label: string }[] = [
  { key: '24h', label: '24 saat' },
  { key: '7d', label: '7 gün' },
];

// Level (and pressure, if the device reports it) history with a 24 h / 7 day toggle.
export function HistorySection({ device }: { device: Device }) {
  const [range, setRange] = useState<HistoryRange>('24h');
  const { history, loading, error } = useTelemetryHistory(device.id, range);

  return (
    <View>
      <View style={styles.header}>
        <Text style={styles.section}>Geçmiş</Text>
        <View style={styles.toggle}>
          {RANGES.map((r) => (
            <Pressable
              key={r.key}
              onPress={() => setRange(r.key)}
              style={[styles.toggleItem, range === r.key && styles.toggleActive]}
            >
              <Text style={[styles.toggleText, range === r.key && styles.toggleTextActive]}>{r.label}</Text>
            </Pressable>
          ))}
        </View>
      </View>

      {loading && !history && <ActivityIndicator style={{ marginVertical: spacing.lg }} />}
      {error && <Text style={styles.error}>Geçmiş okunamadı: {error}</Text>}
      {history && (
        <View style={{ opacity: loading ? 0.5 : 1 }}>
          <LineChart
            title="Depo seviyesi"
            unit="mm"
            points={history.level}
            from={history.from}
            to={history.to}
            guides={[device.level_low_mm, device.level_high_mm]}
          />
          {history.hasPressure && (
            <LineChart
              title="Basınç"
              unit="bar"
              points={history.pressure}
              from={history.from}
              to={history.to}
              color={colors.ok}
              decimals={2}
              guides={[
                device.pressure_low_cbar != null ? device.pressure_low_cbar / 100 : null,
                device.pressure_high_cbar != null ? device.pressure_high_cbar / 100 : null,
              ]}
            />
          )}
          <Text style={styles.note}>{history.count} ölçüm · kesikli çizgiler alarm eşikleri</Text>
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
  toggle: { flexDirection: 'row', borderWidth: 1, borderColor: colors.primary, borderRadius: 8, overflow: 'hidden' },
  toggleItem: { paddingVertical: 6, paddingHorizontal: spacing.md, backgroundColor: colors.card },
  toggleActive: { backgroundColor: colors.primary },
  toggleText: { color: colors.primary, fontWeight: '600', fontSize: 13 },
  toggleTextActive: { color: '#fff' },
  error: { color: colors.danger, marginBottom: spacing.sm },
  note: { color: colors.muted, fontSize: 12 },
});
