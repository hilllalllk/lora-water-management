import { useState } from 'react';
import { ActivityIndicator, Pressable, StyleSheet, Text, TextInput, View } from 'react-native';

import { supabase } from '../lib/supabase';
import { colors, spacing } from '../theme';

// Kullanıcılar Supabase panelinden (Authentication > Users) eklenir; uygulamada kayıt ekranı yoktur.
export function LoginScreen() {
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);

  async function signIn() {
    setBusy(true);
    setError(null);
    const { error: err } = await supabase.auth.signInWithPassword({ email: email.trim(), password });
    if (err) setError('Giriş başarısız: ' + err.message);
    setBusy(false);
  }

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Su Yönetimi</Text>
      <Text style={styles.subtitle}>LoRa tabanlı uzaktan izleme ve kontrol</Text>

      <TextInput
        style={styles.input}
        placeholder="E-posta"
        autoCapitalize="none"
        keyboardType="email-address"
        value={email}
        onChangeText={setEmail}
      />
      <TextInput
        style={styles.input}
        placeholder="Şifre"
        secureTextEntry
        value={password}
        onChangeText={setPassword}
      />
      {error && <Text style={styles.error}>{error}</Text>}

      <Pressable style={styles.button} onPress={signIn} disabled={busy}>
        {busy ? <ActivityIndicator color="#fff" /> : <Text style={styles.buttonText}>Giriş yap</Text>}
      </Pressable>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, justifyContent: 'center', padding: spacing.xl, backgroundColor: colors.bg },
  title: { fontSize: 28, fontWeight: '700', color: colors.text },
  subtitle: { fontSize: 15, color: colors.muted, marginBottom: spacing.xl },
  input: {
    backgroundColor: colors.card,
    borderColor: colors.border,
    borderWidth: 1,
    borderRadius: 8,
    padding: spacing.md,
    marginBottom: spacing.md,
    fontSize: 16,
  },
  error: { color: colors.danger, marginBottom: spacing.md },
  button: {
    backgroundColor: colors.primary,
    borderRadius: 8,
    padding: spacing.md,
    alignItems: 'center',
  },
  buttonText: { color: '#fff', fontSize: 16, fontWeight: '600' },
});
