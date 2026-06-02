/* Bottom panel: digital waveform oscilloscope */

import { useMemo, useState } from 'react';
import { useStore } from '../store/useStore';
import type { WaveSignal } from '../types/circuit';

const HEIGHT = 180;
const CHANNEL_H = 36;
const LEFT_PAD = 100;
const RIGHT_PAD = 20;
const EMPTY_SIGNALS: WaveSignal[] = [];

export function Oscilloscope() {
  const { simResult } = useStore();
  const [visible, setVisible] = useState(true);
  const [zoom, setZoom] = useState(1);

  const signals = simResult?.waveform?.signals ?? EMPTY_SIGNALS;
  const maxTime = useMemo(() => {
    if (signals.length === 0) return 100;
    return Math.max(100, ...signals.flatMap(s => s.values.map(v => v.t)));
  }, [signals]);

  if (!simResult) {
    return (
      <div style={styles.container}>
        <div style={styles.header}>
          <span style={styles.title}>📊 示波器</span>
          <span style={styles.hint}>运行仿真后显示波形</span>
        </div>
      </div>
    );
  }

  if (!visible) {
    return (
      <div style={{ ...styles.container, height: 36 }}>
        <div style={styles.header}>
          <span style={styles.title}>📊 示波器</span>
          <button style={styles.toggleBtn} onClick={() => setVisible(true)}>展开</button>
        </div>
      </div>
    );
  }

  const drawWidth = 800 * zoom;
  const totalH = Math.max(CHANNEL_H * 2, signals.length * CHANNEL_H) + 30;

  function timeToX(t: number): number {
    return LEFT_PAD + (t / maxTime) * drawWidth;
  }

  function getSignalY(idx: number): number {
    return 20 + idx * CHANNEL_H + CHANNEL_H / 2;
  }

  function signalColor(v: string): string {
    switch (v) {
      case '1': return '#a6e3a1';
      case '0': return '#313244';
      case 'X': return '#f38ba8';
      case 'Z': return '#89b4fa';
      default: return '#585b70';
    }
  }

  return (
    <div style={{ ...styles.container, height: Math.min(HEIGHT, totalH + 10) }}>
      <div style={styles.header}>
        <span style={styles.title}>📊 示波器</span>
        <span style={styles.unit}>时间单位: {simResult.waveform?.time_unit || 'tick'} | ticks: {simResult.ticks_elapsed}</span>
        <div style={{ flex: 1 }} />
        <button style={styles.zoomBtn} onClick={() => setZoom(z => Math.max(0.25, z - 0.25))}>−</button>
        <span style={{ fontSize: 11, color: '#a6adc8', minWidth: 40, textAlign: 'center' }}>{zoom}x</span>
        <button style={styles.zoomBtn} onClick={() => setZoom(z => Math.min(4, z + 0.25))}>+</button>
        <button style={styles.toggleBtn} onClick={() => setVisible(false)}>收起</button>
      </div>

      <div style={{ overflowX: 'auto', overflowY: 'auto', flex: 1 }}>
        <svg width={LEFT_PAD + drawWidth + RIGHT_PAD} height={totalH}>
          {/* Grid lines */}
          {Array.from({ length: Math.floor(drawWidth / 50) + 1 }, (_, i) => (
            <line key={`g${i}`} x1={LEFT_PAD + i * 50} y1={0} x2={LEFT_PAD + i * 50} y2={totalH}
              stroke="#181825" strokeWidth={0.5} />
          ))}

          {/* Time axis */}
          <line x1={LEFT_PAD} y1={totalH - 5} x2={LEFT_PAD + drawWidth} y2={totalH - 5} stroke="#45475a" strokeWidth={1} />
          {Array.from({ length: 6 }, (_, i) => {
            const t = Math.round((maxTime / 5) * i);
            const x = timeToX(t);
            return (
              <text key={`t${i}`} x={x} y={totalH - 8} textAnchor="middle" fill="#6c7086" fontSize={9}>
                t={t}
              </text>
            );
          })}

          {/* Signal channels */}
          {signals.map((sig, idx) => {
            const y = getSignalY(idx);
            const pts = sig.values;
            if (pts.length === 0) return null;

            return (
              <g key={sig.id}>
                {/* Label */}
                <text x={LEFT_PAD - 8} y={y + 4} textAnchor="end" fill="#cdd6f4" fontSize={10}>
                  {sig.name || sig.id}
                </text>
                {/* Baseline */}
                <line x1={LEFT_PAD} y1={y} x2={LEFT_PAD + drawWidth} y2={y} stroke="#45475a" strokeWidth={0.5} />
                {/* Waveform */}
                <path
                  d={pts.map((p, i) => {
                    const x = timeToX(p.t);
                    const high = y - 12;
                    const low = y;
                    const prevV = i > 0 ? pts[i - 1].v : p.v;
                    const prevX = i > 0 ? timeToX(pts[i - 1].t) : x;
                    const prevHigh = prevV === '1' ? high : low;
                    const curHigh = p.v === '1' ? high : low;
                    return `M ${prevX} ${prevHigh} L ${x} ${prevHigh} L ${x} ${curHigh}`;
                  }).join(' ') + (pts.length > 0
                    ? ` L ${timeToX(pts[pts.length - 1].t)} ${pts[pts.length - 1].v === '1' ? y - 12 : y}`
                    : '')
                  }
                  fill="none" stroke="#89b4fa" strokeWidth={1.5}
                />
                {/* Value markers */}
                {pts.map((p, i) => (
                  <circle key={i} cx={timeToX(p.t)} cy={p.v === '1' ? y - 12 : y}
                    r={3} fill={signalColor(p.v)} stroke="#1e1e2e" strokeWidth={1} />
                ))}
              </g>
            );
          })}
        </svg>
      </div>
    </div>
  );
}

const styles: Record<string, React.CSSProperties> = {
  container: {
    borderTop: '1px solid #313244',
    background: '#1e1e2e',
    display: 'flex', flexDirection: 'column',
    overflow: 'hidden',
  },
  header: {
    display: 'flex', alignItems: 'center', gap: 12,
    padding: '4px 12px',
    borderBottom: '1px solid #313244',
  },
  title: {
    fontSize: 13, fontWeight: 600, color: '#cdd6f4',
  },
  hint: {
    fontSize: 11, color: '#6c7086',
  },
  unit: {
    fontSize: 10, color: '#a6adc8',
  },
  toggleBtn: {
    padding: '2px 10px', fontSize: 10,
    border: '1px solid #45475a', borderRadius: 4,
    background: '#313244', color: '#cdd6f4',
    cursor: 'pointer',
  },
  zoomBtn: {
    padding: '2px 8px', fontSize: 12,
    border: '1px solid #45475a', borderRadius: 4,
    background: '#313244', color: '#cdd6f4',
    cursor: 'pointer',
  },
};
