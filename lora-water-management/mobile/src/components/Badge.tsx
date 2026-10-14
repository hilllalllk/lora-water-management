import { StyleSheet, Text, View } from 'react-native';

import { colors } from '../theme';

type Tone = 'ok' | 'warn' | 'danger' | 'muted';

const toneColor: Record<Tone, string> = {
  ok: colors.ok,
  warn: colors.warn,
  danger: colors.danger,
  muted: colors.muted,
};

export function Badge({ label, tone }: { label: string; tone: Tone }) {
  return (
    <View style={[styles.badge, { borderColor: toneColor[tone] }]}>
      <Text style={[styles.text, { color: toneColor[tone] }]}>{label}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  badge: { borderWidth: 1, borderRadius: 999, paddingHorizontal: 8, paddingVertical: 2 },
  text: { fontSize: 12, fontWeight: '600' },
});
