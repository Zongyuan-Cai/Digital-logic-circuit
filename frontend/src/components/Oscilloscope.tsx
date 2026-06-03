/* Bottom panel: digital waveform oscilloscope */

import { useMemo, useState } from 'react';
import { useStore } from '../store/useStore';
import type { WaveSignal } from '../types/circuit';

const DEFAULT_HEIGHT = 260;
const MIN_HEIGHT = 120;
const MAX_HEIGHT = 520;
const CHANNEL_H = 54;
const LEFT_PAD = 140;
const RIGHT_PAD = 20;
const EMPTY_SIGNALS: WaveSignal[] = [];
const WAVE_HIGH = 18;

function connectedNetwork(
  wires: { from: { device: string; pin: string }; to: { device: string; pin: string } }[],
  deviceId: string,
  pinId: string,
): { keys: Set<string>; pinNames: Set<string> } {
  const pinNames = new Set<string>([pinId]);
  const seen = new Set<string>([`${deviceId}.${pinId}`]);
  const queue = [{ device: deviceId, pin: pinId }];
  while (queue.length > 0) {
    const current = queue.shift();
    if (!current) break;
    for (const wire of wires) {
      const next =
        wire.from.device === current.device && wire.from.pin === current.pin
          ? wire.to
          : wire.to.device === current.device && wire.to.pin === current.pin
            ? wire.from
            : null;
      if (!next) continue;
      const key = `${next.device}.${next.pin}`;
      if (seen.has(key)) continue;
      seen.add(key);
      pinNames.add(next.pin);
      queue.push(next);
    }
  }
  return { keys: seen, pinNames };
}

export function Oscilloscope() {
  const {
    circuit, devices, waveformDeviceIds,
    simResult, playbackActive, playbackTime, playbackMaxTime,
  } = useStore();
  const [visible, setVisible] = useState(true);
  const [zoom, setZoom] = useState(1);
  const [panelHeight, setPanelHeight] = useState(DEFAULT_HEIGHT);

  function startResize(e: React.PointerEvent<HTMLDivElement>) {
    e.preventDefault();
    const startY = e.clientY;
    const startHeight = panelHeight;
    const maxHeight = Math.min(MAX_HEIGHT, Math.max(MIN_HEIGHT, window.innerHeight - 160));

    const onMove = (event: PointerEvent) => {
      const nextHeight = startHeight + startY - event.clientY;
      setPanelHeight(Math.min(maxHeight, Math.max(MIN_HEIGHT, nextHeight)));
    };
    const onUp = () => {
      window.removeEventListener('pointermove', onMove);
      window.removeEventListener('pointerup', onUp);
    };

    window.addEventListener('pointermove', onMove);
    window.addEventListener('pointerup', onUp);
  }

  const allSignals = simResult?.waveform?.signals ?? EMPTY_SIGNALS;
  const signals = useMemo(() => {
    if (!simResult) return EMPTY_SIGNALS;
    const chosen: WaveSignal[] = [];
    const seen = new Set<string>();

    function addSignal(sig: WaveSignal | undefined, label?: string) {
      if (!sig) return;
      const displayName = label ?? sig.name ?? sig.id;
      const key = displayName;
      if (seen.has(key)) return;
      seen.add(key);
      chosen.push({ ...sig, name: displayName });
    }

    function addNetwork(deviceId: string, pinId: string, label: string) {
      const network = connectedNetwork(circuit.wires, deviceId, pinId);
      const exact = allSignals.find(sig => network.keys.has(sig.id) || network.keys.has(sig.name));
      const named = allSignals.find(sig => network.pinNames.has(sig.name) && sig.values.some(v => v.v !== 'Z'));
      const fallback = allSignals.find(sig => network.pinNames.has(sig.name) || network.pinNames.has(sig.id));
      addSignal(exact ?? named ?? fallback, label);
    }

    for (const dev of circuit.devices) {
      if (dev.type === 'CLOCK') addNetwork(dev.id, 'OUT', `${dev.id}.OUT`);
    }

    for (const deviceId of waveformDeviceIds) {
      const dev = circuit.devices.find(d => d.id === deviceId);
      const def = dev ? devices.find(d => d.type === dev.type) : undefined;
      if (!dev || !def) continue;
      for (const pin of def.pins) {
        addNetwork(dev.id, pin.id, `${dev.id}.${pin.id}`);
      }
    }

    return chosen;
  }, [allSignals, circuit.devices, circuit.wires, devices, simResult, waveformDeviceIds]);

  const displayTime = playbackActive ? Math.max(0, playbackTime) : null;
  const maxTime = useMemo(() => {
    if (playbackActive) return Math.max(1, playbackMaxTime);
    if (signals.length === 0) return 100;
    return Math.max(100, ...signals.flatMap(s => s.values.map(v => v.t)));
  }, [playbackActive, playbackMaxTime, signals]);

  if (!simResult) {
    return (
      <div style={{ ...styles.container, height: panelHeight }}>
        <div style={styles.resizeHandle} onPointerDown={startResize} title="拖动调整示波器高度" />
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
        <div style={styles.resizeHandle} onPointerDown={startResize} title="拖动调整示波器高度" />
        <div style={styles.header}>
          <span style={styles.title}>📊 示波器</span>
          <button style={styles.toggleBtn} onClick={() => setVisible(true)}>展开</button>
        </div>
      </div>
    );
  }

  const drawWidth = 1100 * zoom;
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

  function visiblePoints(sig: WaveSignal): { t: number; v: string }[] {
    if (displayTime === null) return sig.values;
    const pts = sig.values.filter(p => p.t <= displayTime);
    const last = pts[pts.length - 1];
    if (last && last.t < displayTime) {
      return [...pts, { t: displayTime, v: last.v }];
    }
    return pts;
  }

  return (
    <div style={{ ...styles.container, height: panelHeight }}>
      <div style={styles.resizeHandle} onPointerDown={startResize} title="拖动调整示波器高度" />
      <div style={styles.header}>
        <span style={styles.title}>📊 示波器</span>
        <span style={styles.unit}>
          时间单位: {simResult.waveform?.time_unit || 'tick'} | {playbackActive ? `t=${playbackTime}/${playbackMaxTime}` : `ticks: ${simResult.ticks_elapsed}`}
        </span>
        <span style={styles.hint}>时钟源常驻；点选器件后在右侧加入示波器</span>
        <div style={{ flex: 1 }} />
        <button style={styles.zoomBtn} onClick={() => setZoom(z => Math.max(0.25, z - 0.25))}>−</button>
        <span style={{ fontSize: 13, color: '#a6adc8', minWidth: 44, textAlign: 'center' }}>{zoom}x</span>
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
              <text key={`t${i}`} x={x} y={totalH - 8} textAnchor="middle" fill="#6c7086" fontSize={12}>
                t={t}
              </text>
            );
          })}

          {displayTime !== null && (
            <line
              x1={timeToX(displayTime)}
              y1={0}
              x2={timeToX(displayTime)}
              y2={totalH - 5}
              stroke="#f9e2af"
              strokeWidth={1}
              strokeDasharray="4,3"
            />
          )}

          {signals.length === 0 && (
            <text x={LEFT_PAD + 16} y={46} fill="#6c7086" fontSize={14}>
              没有可显示的波形。选择器件后，在右侧属性面板点击“在示波器显示”。
            </text>
          )}

          {/* Signal channels */}
          {signals.map((sig, idx) => {
            const y = getSignalY(idx);
            const pts = visiblePoints(sig);
            if (pts.length === 0) return null;

            return (
              <g key={sig.id}>
                {/* Label */}
                <text x={LEFT_PAD - 8} y={y + 5} textAnchor="end" fill="#cdd6f4" fontSize={13}>
                  {sig.name || sig.id}
                </text>
                {/* Baseline */}
                <line x1={LEFT_PAD} y1={y} x2={LEFT_PAD + drawWidth} y2={y} stroke="#45475a" strokeWidth={0.5} />
                {/* Waveform */}
                <path
                  d={pts.map((p, i) => {
                    const x = timeToX(p.t);
                    const high = y - WAVE_HIGH;
                    const low = y;
                    const prevV = i > 0 ? pts[i - 1].v : p.v;
                    const prevX = i > 0 ? timeToX(pts[i - 1].t) : x;
                    const prevHigh = prevV === '1' ? high : low;
                    const curHigh = p.v === '1' ? high : low;
                    return `M ${prevX} ${prevHigh} L ${x} ${prevHigh} L ${x} ${curHigh}`;
                  }).join(' ') + (pts.length > 0
                    ? ` L ${timeToX(pts[pts.length - 1].t)} ${pts[pts.length - 1].v === '1' ? y - WAVE_HIGH : y}`
                    : '')
                  }
                  fill="none" stroke="#89b4fa" strokeWidth={2.5}
                />
                {/* Value markers */}
                {pts.map((p, i) => (
                  <circle key={i} cx={timeToX(p.t)} cy={p.v === '1' ? y - WAVE_HIGH : y}
                    r={4} fill={signalColor(p.v)} stroke="#1e1e2e" strokeWidth={1} />
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
    flexShrink: 0,
    overflow: 'hidden',
  },
  resizeHandle: {
    height: 7,
    flexShrink: 0,
    cursor: 'ns-resize',
    background: 'linear-gradient(to bottom, #313244, #1e1e2e)',
    borderBottom: '1px solid #181825',
  },
  header: {
    display: 'flex', alignItems: 'center', gap: 12,
    padding: '6px 12px',
    borderBottom: '1px solid #313244',
  },
  title: {
    fontSize: 15, fontWeight: 600, color: '#cdd6f4',
  },
  hint: {
    fontSize: 13, color: '#6c7086',
  },
  unit: {
    fontSize: 12, color: '#a6adc8',
  },
  toggleBtn: {
    padding: '3px 12px', fontSize: 12,
    border: '1px solid #45475a', borderRadius: 4,
    background: '#313244', color: '#cdd6f4',
    cursor: 'pointer',
  },
  zoomBtn: {
    padding: '3px 10px', fontSize: 14,
    border: '1px solid #45475a', borderRadius: 4,
    background: '#313244', color: '#cdd6f4',
    cursor: 'pointer',
  },
};
