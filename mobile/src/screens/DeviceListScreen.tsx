import { ActivityIndicator, FlatList, Pressable, StyleSheet, Text, View } from 'react-native';

import { Badge } from '../components/Badge';
import { useNow } from '../hooks/useNow';
import { alarmLabels, isOnline, ROLE_LABEL, timeAgo, VALVE_LABEL } from '../lib/format';
import { signOut } from '../lib/push';
import { colors, spacing } from '../theme';
import type { AppRole, Device } from '../types';

interface Props {
  devices: Device[];
  loading: boolean;
  error: string | null;
  openByDevice: Record<string, number>;
  openAlertCount: number;
  role: AppRole;
  onSelect: (d: Device) => void;
  onAlerts: () => void;
  onUsers: () => void;
}

export function DeviceListScreen(props: Props) {
  const { devices, loading, error, openByDevice, openAlertCount, role, onSelect } = props;
  const now = useNow();

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Saha cihazları</Text>
        <Pressable onPress={() => signOut()}>
          <Text style={styles.link}>Çıkış</Text>
        </Pressable>
      </View>
      <View style={styles.toolbar}>
        <Pressable style={[styles.chip, openAlertCount > 0 && styles.chipAlert]} onPress={props.onAlerts}>
          <Text style={[styles.chipText, openAlertCount > 0 && { color: '#fff' }]}>
            Alarmlar{openAlertCount > 0 ? ` (${openAlertCount})` : ''}
          </Text>
        </Pressable>
        {role === 'admin' && (
          <Pressable style={styles.chip} onPress={props.onUsers}>
            <Text style={styles.chipText}>Kullanıcılar</Text>
          </Pressable>
        )}
        <Text style={styles.role}>{ROLE_LABEL[role]}</Text>
      </View>

      {loading && <ActivityIndicator style={{ marginTop: spacing.xl }} />}
      {error && <Text style={styles.error}>{error}</Text>}
      {!loading && devices.length === 0 && !error && (
        <Text style={styles.empty}>Henüz cihaz yok. Supabase'de devices tablosuna ekleyin.</Text>
      )}

      <FlatList
        data={devices}
        keyExtractor={(d) => d.id}
        contentContainerStyle={{ gap: spacing.md, paddingBottom: spacing.xl }}
        renderItem={({ item }) => {
          const online = isOnline(item, now);
          const alarms = alarmLabels(item.alarms);
          const openAlerts = openByDevice[item.id] ?? 0;
          return (
            <Pressable style={styles.card} onPress={() => onSelect(item)}>
              <View style={styles.row}>
                <Text style={styles.name}>{item.name}</Text>
                <View style={styles.badges}>
                  {openAlerts > 0 && <Badge label={`${openAlerts} alarm`} tone="danger" />}
                  <Badge label={online ? 'Çevrimiçi' : 'Çevrimdışı'} tone={online ? 'ok' : 'muted'} />
                </View>
              </View>
              <Text style={styles.meta}>
                Seviye: {item.level_mm ?? '-'} mm · Vana:{' '}
                {item.valve != null ? VALVE_LABEL[item.valve] : '-'}
              </Text>
              <Text style={styles.meta}>Son veri: {timeAgo(item.last_seen, now)}</Text>
              {alarms.length > 0 && (
                <View style={styles.alarms}>
                  {alarms.map((a) => (
                    <Badge key={a} label={a} tone="danger" />
                  ))}
                </View>
              )}
            </Pressable>
          );
        }}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: colors.bg, paddingHorizontal: spacing.lg, paddingTop: 56 },
  header: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: spacing.md },
  toolbar: { flexDirection: 'row', alignItems: 'center', gap: spacing.sm, marginBottom: spacing.lg },
  chip: { borderWidth: 1, borderColor: colors.primary, borderRadius: 999, paddingVertical: 6, paddingHorizontal: spacing.md },
  chipAlert: { backgroundColor: colors.danger, borderColor: colors.danger },
  chipText: { color: colors.primary, fontWeight: '600' },
  role: { marginLeft: 'auto', color: colors.muted, fontSize: 13 },
  badges: { flexDirection: 'row', gap: spacing.xs },
  title: { fontSize: 24, fontWeight: '700', color: colors.text },
  link: { color: colors.primary, fontSize: 16 },
  error: { color: colors.danger },
  empty: { color: colors.muted, marginTop: spacing.xl },
  card: { backgroundColor: colors.card, borderRadius: 12, padding: spacing.lg, borderWidth: 1, borderColor: colors.border },
  row: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: spacing.sm },
  name: { fontSize: 17, fontWeight: '600', color: colors.text, flexShrink: 1 },
  meta: { color: colors.muted, marginTop: 2 },
  alarms: { flexDirection: 'row', gap: spacing.sm, marginTop: spacing.sm },
});
