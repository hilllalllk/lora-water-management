import { useState } from 'react';
import { ActivityIndicator, Alert, FlatList, Pressable, StyleSheet, Text, View } from 'react-native';

import { useProfiles } from '../hooks/useProfiles';
import { ROLE_LABEL } from '../lib/format';
import { colors, spacing } from '../theme';
import type { AppRole } from '../types';

const ROLES: AppRole[] = ['admin', 'operator', 'viewer'];

const ROLE_HINT: Record<AppRole, string> = {
  admin: 'Her şey, kullanıcı rolleri ve eşik ayarları dahil',
  operator: 'İzleme ve komut gönderme',
  viewer: 'Sadece izleme',
};

// Admin-only "Kullanıcılar": every user's role. New users are added in the Supabase dashboard
// and start as İzleyici.
export function UsersScreen({ currentUserId, onBack }: { currentUserId: string; onBack: () => void }) {
  const { profiles, loading, error, setRole } = useProfiles();
  const [busyId, setBusyId] = useState<string | null>(null);

  async function change(id: string, role: AppRole) {
    setBusyId(id);
    try {
      await setRole(id, role);
    } catch (e) {
      Alert.alert('Rol değiştirilemedi', e instanceof Error ? e.message : String(e));
    } finally {
      setBusyId(null);
    }
  }

  function confirm(id: string, role: AppRole) {
    if (id === currentUserId && role !== 'admin') {
      Alert.alert('Kendi rolünüz', 'Yönetici yetkinizi kaldırırsanız bu ekrana tekrar erişemezsiniz.', [
        { text: 'Vazgeç', style: 'cancel' },
        { text: 'Değiştir', style: 'destructive', onPress: () => change(id, role) },
      ]);
    } else {
      change(id, role);
    }
  }

  return (
    <View style={styles.container}>
      <Pressable onPress={onBack}>
        <Text style={styles.link}>‹ Cihazlar</Text>
      </Pressable>
      <Text style={styles.title}>Kullanıcılar</Text>
      {ROLES.map((r) => (
        <Text key={r} style={styles.meta}>
          {ROLE_LABEL[r]}: {ROLE_HINT[r]}
        </Text>
      ))}

      {loading && <ActivityIndicator style={{ marginTop: spacing.xl }} />}
      {error && <Text style={styles.error}>{error}</Text>}

      <FlatList
        data={profiles}
        keyExtractor={(p) => p.id}
        contentContainerStyle={{ gap: spacing.sm, paddingVertical: spacing.md, paddingBottom: spacing.xl }}
        renderItem={({ item }) => (
          <View style={styles.card}>
            <Text style={styles.name}>
              {item.full_name || item.email || item.id.slice(0, 8)}
              {item.id === currentUserId ? ' (siz)' : ''}
            </Text>
            {item.full_name && item.email ? <Text style={styles.meta}>{item.email}</Text> : null}
            <View style={styles.picker}>
              {ROLES.map((r) => {
                const active = item.role === r;
                return (
                  <Pressable
                    key={r}
                    disabled={active || busyId === item.id}
                    onPress={() => confirm(item.id, r)}
                    style={[styles.option, active && styles.optionActive]}
                  >
                    <Text style={[styles.optionText, active && styles.optionTextActive]}>{ROLE_LABEL[r]}</Text>
                  </Pressable>
                );
              })}
              {busyId === item.id && <ActivityIndicator style={{ marginLeft: spacing.sm }} />}
            </View>
          </View>
        )}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: colors.bg, paddingHorizontal: spacing.lg, paddingTop: 56 },
  link: { color: colors.primary, fontSize: 16, marginBottom: spacing.md },
  title: { fontSize: 24, fontWeight: '700', color: colors.text, marginBottom: spacing.xs },
  meta: { color: colors.muted, fontSize: 13, marginTop: 2 },
  error: { color: colors.danger, marginTop: spacing.md },
  card: { backgroundColor: colors.card, borderRadius: 12, padding: spacing.md, borderWidth: 1, borderColor: colors.border },
  name: { fontWeight: '600', color: colors.text },
  picker: { flexDirection: 'row', alignItems: 'center', gap: spacing.sm, marginTop: spacing.sm },
  option: {
    borderWidth: 1,
    borderColor: colors.primary,
    borderRadius: 8,
    paddingVertical: 6,
    paddingHorizontal: spacing.md,
  },
  optionActive: { backgroundColor: colors.primary },
  optionText: { color: colors.primary, fontWeight: '600', fontSize: 13 },
  optionTextActive: { color: '#fff' },
});
