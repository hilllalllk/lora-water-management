import { useEffect, useState } from 'react';

// "x sn önce" ve çevrimiçi durumunun güncel kalması için belirli aralıkla yenilenen saat.
export function useNow(intervalMs = 5000) {
  const [now, setNow] = useState(Date.now());
  useEffect(() => {
    const t = setInterval(() => setNow(Date.now()), intervalMs);
    return () => clearInterval(t);
  }, [intervalMs]);
  return now;
}
