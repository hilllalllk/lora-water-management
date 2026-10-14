import { useState } from 'react';
import { ActivityIndicator, Alert, Pressable, ScrollView, StyleSheet, Text, View } from 'react-native';

import { Badge } from '../components/Badge';
import { useCommands } from '../hooks/useCommands';
import { useDevices } from '../hooks/useDevices';
import { useNow } from '../hooks/useNow';
import {
  alarmLabels,
  isFinal,
  isOnline,
  OP_LABEL,
  RESULT_LABEL,
  STATUS_LABEL,
  timeAgo,
  VALVE_LABEL,
} from '../lib/format';
import { colors, spacing } from '../theme';
import type { CommandOp, CommandStatus } from '../types';

const statusTone: Record<CommandStatus, 'ok' | 'warn' | 'danger' | 'muted'> = {
  pending: 'warn',
  sent: 'warn',
  delivered: 'warn',
  done: 'ok',
  failed: 'danger',
};

export function DeviceDetailScreen({ deviceId, onBack }: { deviceId: string; onBack: () => void }) {
  const { devices } = useDevices();
  const { commands, send } = useCommands(deviceId);
  const [sending, setSending] = useState(false);
  const now = useNow();

  const device = devices.find((d) => d.id === deviceId);
  // Önceki komut bitmeden yeni komut gönderilmez: gerçek durum ancak cihaz cevap verince bilinir.
  const waiting = commands.some((c) => !isFinal(c.status));

  async function run(op: CommandOp) {
    setSending(true);
    try {
      await send(op);
    } catch (e) {
      Alert.alert('Komut gönderilemedi', e instanceof Error ? e.message : String(e));
    } finally {
      setSending(false);
    }
  }

  if (!device) {
    return (
      <View style={styles.container}>
        <ActivityIndicator />
      </View>
    );
  }

  const online = isOnline(device, now);
  const alarms = alarmLabels(device.alarms);
  const disabled = sending || waiting;

  return (
    <ScrollView style={styles.container} contentContainerStyle={{ paddingBottom: spacing.xl }}>
      <Pressable onPress={onBack}>
        <Text style={styles.link}>‹ Cihazlar</Text>
      </Pressable>

      <View style={styles.row}>
        <Text style={styles.title}>{device.name}</Text>
        <Badge label={online ? 'Çevrimiçi' : 'Çevrimdışı'} tone={online ? 'ok' : 'muted'} />
      </View>
      <Text style={styles.meta}>
        Adres 0x{device.address.toString(16).toUpperCase().padStart(4, '0')} · Son veri{' '}
        {timeAgo(device.last_seen, now)}
      </Text>

      <View style={styles.grid}>
        <Metric label="Depo seviyesi" value={device.level_mm != null ? `${device.level_mm} mm` : '-'} />
        <Metric label="Vana" value={device.valve != null ? VALVE_LABEL[device.valve] : '-'} />
        <Metric
          label="Basınç"
          value={device.pressure_cbar != null ? `${(device.pressure_cbar / 100).toFixed(2)} bar` : '-'}
        />
        <Metric
          label="Debi"
          value={device.flow_dl_min != null ? `${(device.flow_dl_min / 10).toFixed(1)} L/dk` : '-'}
        />
      </View>

      {alarms.length > 0 && (
        <View style={styles.alarms}>
          {alarms.map((a) => (
            <Badge key={a} label={`Alarm: ${a}`} tone="danger" />
          ))}
        </View>
      )}

      <Text style={styles.section}>Kontrol</Text>
      <View style={styles.buttons}>
        <ActionButton label="Vana aç" onPress={() => run('VANA_AC')} disabled={disabled} />
        <ActionButton label="Vana kapat" onPress={() => run('VANA_KAPAT')} disabled={disabled} />
        <ActionButton label="Şimdi oku" onPress={() => run('SIMDI_OKU')} disabled={disabled} secondary />
      </View>
      {waiting && <Text style={styles.waiting}>Önceki komutun cevabı bekleniyor…</Text>}

      <Text style={styles.section}>Son komutlar</Text>
      {commands.length === 0 && <Text style={styles.meta}>Henüz komut yok.</Text>}
      {commands.map((c) => (
        <View key={c.id} style={styles.command}>
          <View style={styles.row}>
            <Text style={styles.commandName}>{OP_LABEL[c.op]}</Text>
            <Badge label={STATUS_LABEL[c.status]} tone={statusTone[c.status]} />
          </View>
          <Text style={styles.meta}>
            {timeAgo(c.created_at, now)}
            {c.result != null ? ` · ${RESULT_LABEL[c.result] ?? `Kod ${c.result}`}` : ''}
          </Text>
        </View>
      ))}
    </ScrollView>
  );
}

function Metric({ label, value }: { label: string; value: string }) {
  return (
    <View style={styles.metric}>
      <Text style={styles.metricLabel}>{label}</Text>
      <Text style={styles.metricValue}>{value}</Text>
    </View>
  );
}

function ActionButton(props: { label: string; onPress: () => void; disabled: boolean; secondary?: boolean }) {
  return (
    <Pressable
      onPress={props.onPress}
      disabled={props.disabled}
      style={[styles.button, props.secondary && styles.buttonSecondary, props.disabled && styles.buttonDisabled]}
    >
      <Text style={[styles.buttonText, props.secondary && { color: colors.primary }]}>{props.label}</Text>
    </Pressable>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: colors.bg, paddingHorizontal: spacing.lg, paddingTop: 56 },
  link: { color: colors.primary, fontSize: 16, marginBottom: spacing.md },
  row: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center' },
  title: { fontSize: 24, fontWeight: '700', color: colors.text, flexShrink: 1 },
  meta: { color: colors.muted, marginTop: 2 },
  grid: { flexDirection: 'row', flexWrap: 'wrap', gap: spacing.md, marginTop: spacing.lg },
  metric: {
    width: '47%',
    backgroundColor: colors.card,
    borderRadius: 12,
    padding: spacing.md,
    borderWidth: 1,
    borderColor: colors.border,
  },
  metricLabel: { color: colors.muted, fontSize: 13 },
  metricValue: { color: colors.text, fontSize: 20, fontWeight: '600', marginTop: spacing.xs },
  alarms: { flexDirection: 'row', flexWrap: 'wrap', gap: spacing.sm, marginTop: spacing.md },
  section: { fontSize: 17, fontWeight: '600', color: colors.text, marginTop: spacing.xl, marginBottom: spacing.sm },
  buttons: { flexDirection: 'row', flexWrap: 'wrap', gap: spacing.sm },
  button: { backgroundColor: colors.primary, borderRadius: 8, paddingVertical: spacing.md, paddingHorizontal: spacing.lg },
  buttonSecondary: { backgroundColor: colors.card, borderWidth: 1, borderColor: colors.primary },
  buttonDisabled: { opacity: 0.4 },
  buttonText: { color: '#fff', fontWeight: '600' },
  waiting: { color: colors.warn, marginTop: spacing.sm },
  command: {
    backgroundColor: colors.card,
    borderRadius: 10,
    padding: spacing.md,
    marginBottom: spacing.sm,
    borderWidth: 1,
    borderColor: colors.border,
  },
  commandName: { fontWeight: '600', color: colors.text },
});
