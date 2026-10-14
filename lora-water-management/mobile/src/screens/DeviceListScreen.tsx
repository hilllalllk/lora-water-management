import { ActivityIndicator, FlatList, Pressable, StyleSheet, Text, View } from 'react-native';

import { Badge } from '../components/Badge';
import { useDevices } from '../hooks/useDevices';
import { useNow } from '../hooks/useNow';
import { alarmLabels, isOnline, timeAgo, VALVE_LABEL } from '../lib/format';
import { supabase } from '../lib/supabase';
import { colors, spacing } from '../theme';
import type { Device } from '../types';

export function DeviceListScreen({ onSelect }: { onSelect: (d: Device) => void }) {
  const { devices, loading, error } = useDevices();
  const now = useNow();

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Saha cihazları</Text>
        <Pressable onPress={() => supabase.auth.signOut()}>
          <Text style={styles.link}>Çıkış</Text>
        </Pressable>
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
          return (
            <Pressable style={styles.card} onPress={() => onSelect(item)}>
              <View style={styles.row}>
                <Text style={styles.name}>{item.name}</Text>
                <Badge label={online ? 'Çevrimiçi' : 'Çevrimdışı'} tone={online ? 'ok' : 'muted'} />
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
  header: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: spacing.lg },
  title: { fontSize: 24, fontWeight: '700', color: colors.text },
  link: { color: colors.primary, fontSize: 16 },
  error: { color: colors.danger },
  empty: { color: colors.muted, marginTop: spacing.xl },
  card: { backgroundColor: colors.card, borderRadius: 12, padding: spacing.lg, borderWidth: 1, borderColor: colors.border },
  row: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: spacing.sm },
  name: { fontSize: 17, fontWeight: '600', color: colors.text },
  meta: { color: colors.muted, marginTop: 2 },
  alarms: { flexDirection: 'row', gap: spacing.sm, marginTop: spacing.sm },
});
