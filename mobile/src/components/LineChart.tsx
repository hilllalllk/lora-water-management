import { useState } from 'react';
import { StyleSheet, Text, View } from 'react-native';
import Svg, { Line, Path, Text as SvgText } from 'react-native-svg';

import { colors } from '../theme';

export interface ChartPoint {
  t: number; // ms since epoch
  v: number;
}

interface Props {
  title: string;
  unit: string;
  points: (ChartPoint | null)[]; // null breaks the line (no data)
  from: number;
  to: number;
  color?: string;
  height?: number;
  decimals?: number;
  // Optional dashed reference lines, e.g. alarm thresholds
  guides?: (number | null | undefined)[];
}

const PAD = { left: 44, right: 8, top: 10, bottom: 22 };

function timeLabel(ms: number, spanMs: number): string {
  const d = new Date(ms);
  const pad = (n: number) => n.toString().padStart(2, '0');
  if (spanMs <= 36 * 3600 * 1000) return `${pad(d.getHours())}:${pad(d.getMinutes())}`;
  return `${pad(d.getDate())}.${pad(d.getMonth() + 1)}`;
}

// Small hand-written line chart: one series, y grid with 3 labels, x labels at start/middle/end.
export function LineChart({
  title,
  unit,
  points,
  from,
  to,
  color = colors.primary,
  height = 160,
  decimals = 0,
  guides = [],
}: Props) {
  const [width, setWidth] = useState(0);
  const values = points.filter((p): p is ChartPoint => p != null);
  const lines = guides.filter((g): g is number => g != null);

  let body = null;
  if (values.length === 0) {
    body = <Text style={styles.empty}>Bu aralıkta veri yok.</Text>;
  } else if (width > 0) {
    let min = Math.min(...values.map((p) => p.v), ...lines);
    let max = Math.max(...values.map((p) => p.v), ...lines);
    if (max - min < 1e-9) {
      min -= 1;
      max += 1;
    }
    const padY = (max - min) * 0.08;
    min -= padY;
    max += padY;

    const w = width - PAD.left - PAD.right;
    const h = height - PAD.top - PAD.bottom;
    const x = (t: number) => PAD.left + ((t - from) / (to - from)) * w;
    const y = (v: number) => PAD.top + (1 - (v - min) / (max - min)) * h;

    // Path with a new segment ("M") after every gap
    let d = '';
    let pen = false;
    for (const p of points) {
      if (!p) {
        pen = false;
        continue;
      }
      d += `${pen ? 'L' : 'M'}${x(p.t).toFixed(1)},${y(p.v).toFixed(1)} `;
      pen = true;
    }
    // Single isolated points get a tiny segment so they are visible
    const dots = points
      .map((p, i) => (p && !points[i - 1] && !points[i + 1] ? p : null))
      .filter((p): p is ChartPoint => p != null);

    const ticks = [min + padY, (min + max) / 2, max - padY];
    const span = to - from;

    body = (
      <Svg width={width} height={height}>
        {ticks.map((v, i) => (
          <Line key={`g${i}`} x1={PAD.left} x2={width - PAD.right} y1={y(v)} y2={y(v)} stroke={colors.border} strokeWidth={1} />
        ))}
        {ticks.map((v, i) => (
          <SvgText key={`l${i}`} x={PAD.left - 6} y={y(v) + 4} fontSize={10} fill={colors.muted} textAnchor="end">
            {v.toFixed(decimals)}
          </SvgText>
        ))}
        {lines.map((g, i) => (
          <Line
            key={`t${i}`}
            x1={PAD.left}
            x2={width - PAD.right}
            y1={y(g)}
            y2={y(g)}
            stroke={colors.danger}
            strokeWidth={1}
            strokeDasharray="4 4"
          />
        ))}
        <Path d={d} stroke={color} strokeWidth={2} fill="none" strokeLinejoin="round" strokeLinecap="round" />
        {dots.map((p, i) => (
          <Path
            key={`d${i}`}
            d={`M${x(p.t) - 1.5},${y(p.v)} L${x(p.t) + 1.5},${y(p.v)}`}
            stroke={color}
            strokeWidth={3}
            strokeLinecap="round"
          />
        ))}
        {[from, from + span / 2, to].map((t, i) => (
          <SvgText
            key={`x${i}`}
            x={x(t)}
            y={height - 6}
            fontSize={10}
            fill={colors.muted}
            textAnchor={i === 0 ? 'start' : i === 2 ? 'end' : 'middle'}
          >
            {timeLabel(t, span)}
          </SvgText>
        ))}
      </Svg>
    );
  }

  return (
    <View style={styles.card}>
      <Text style={styles.title}>
        {title} <Text style={styles.unit}>({unit})</Text>
      </Text>
      <View style={{ minHeight: values.length ? height : undefined }} onLayout={(e) => setWidth(e.nativeEvent.layout.width)}>
        {body}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: colors.card,
    borderRadius: 12,
    borderWidth: 1,
    borderColor: colors.border,
    padding: 12,
    marginBottom: 12,
  },
  title: { color: colors.text, fontWeight: '600', marginBottom: 6 },
  unit: { color: colors.muted, fontWeight: '400' },
  empty: { color: colors.muted, paddingVertical: 16 },
});
