import { ActivityIndicator, FlatList, Pressable, StyleSheet, Text, View } from 'react-native';

import { Badge } from '../components/Badge';
import { useNow } from '../hooks/useNow';
import { ALERT_KIND_LABEL, formatDateTime, timeAgo } from '../lib/format';
import { colors, spacing } from '../theme';
import type { Alert, Device } from '../types';

interface Props {
  alerts: Alert[];
  loading: boolean;
  devices: Device[];
  onBack: () => void;
  onSelectDevice: (deviceId: string) => void;
}

// "Alarmlar": open alerts first, then recent resolved ones. Updated live by useAlerts (Realtime).
export function AlertsScreen({ alerts, loading, devices, onBack, onSelectDevice }: Props) {
  const now = useNow(15000);
  const name = (id: string) => devices.find((d) => d.id === id)?.name ?? id;
  const openCount = alerts.filter((a) => a.resolved_at == null).length;

  return (
    <View style={styles.container}>
      <Pressable onPress={onBack}>
        <Text style={styles.link}>‹ Cihazlar</Text>
      </Pressable>
      <Text style={styles.title}>Alarmlar</Text>
      <Text style={styles.meta}>{openCount > 0 ? `${openCount} açık alarm` : 'Açık alarm yok'}</Text>

      {loading && <ActivityIndicator style={{ marginTop: spacing.xl }} />}
      {!loading && alerts.length === 0 && <Text style={styles.empty}>Henüz alarm kaydı yok.</Text>}

      <FlatList
        data={alerts}
        keyExtractor={(a) => String(a.id)}
        contentContainerStyle={{ gap: spacing.sm, paddingVertical: spacing.md, paddingBottom: spacing.xl }}
        renderItem={({ item }) => {
          const open = item.resolved_at == null;
          return (
            <Pressable
              style={[styles.card, open && styles.cardOpen]}
              onPress={() => onSelectDevice(item.device_id)}
            >
              <View style={styles.row}>
                <Text style={styles.device}>{name(item.device_id)}</Text>
                <Badge label={open ? 'Açık' : 'Çözüldü'} tone={open ? 'danger' : 'muted'} />
              </View>
              <Text style={styles.message}>{item.message}</Text>
              <Text style={styles.meta}>
                {ALERT_KIND_LABEL[item.kind]} · {formatDateTime(item.created_at)} ({timeAgo(item.created_at, now)})
                {item.resolved_at ? ` · çözüldü ${formatDateTime(item.resolved_at)}` : ''}
              </Text>
            </Pressable>
          );
        }}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: colors.bg, paddingHorizontal: spacing.lg, paddingTop: 56 },
  link: { color: colors.primary, fontSize: 16, marginBottom: spacing.md },
  title: { fontSize: 24, fontWeight: '700', color: colors.text },
  meta: { color: colors.muted, marginTop: 2, fontSize: 13 },
  empty: { color: colors.muted, marginTop: spacing.xl },
  card: { backgroundColor: colors.card, borderRadius: 12, padding: spacing.md, borderWidth: 1, borderColor: colors.border },
  cardOpen: { borderColor: colors.danger, borderLeftWidth: 4 },
  row: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: spacing.xs },
  device: { fontWeight: '600', color: colors.text, flexShrink: 1 },
  message: { color: colors.text, marginTop: 2 },
});
