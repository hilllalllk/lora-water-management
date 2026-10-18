import { useEffect, useState } from 'react';

// Clock that ticks at a fixed interval so "x sn önce" labels and online status stay current.
export function useNow(intervalMs = 5000) {
  const [now, setNow] = useState(Date.now());
  useEffect(() => {
    const t = setInterval(() => setNow(Date.now()), intervalMs);
    return () => clearInterval(t);
  }, [intervalMs]);
  return now;
}
