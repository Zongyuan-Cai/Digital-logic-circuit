import { useEffect, useMemo, useRef, useState } from 'react';
import { useStore } from '../store/useStore';
import { findNetworkSignal, SIGNAL_COLORS, valueAt } from '../lib/signals';
import { Icon } from './Icon';
import type { WaveSignal } from '../types/circuit';

const CHANNEL_HEIGHT = 46;
const colors = ['#6fe2be', '#85b5ff', '#eec17d', '#b7a0eb', '#ef9bba'];

export function Oscilloscope() {
  const { circuit, devices, waveformDeviceIds, simResult, playbackActive, playbackTime, playbackMaxTime, startPlayback, stopPlayback, seekPlayback } = useStore();
  const [collapsed, setCollapsed] = useState(false);
  const [zoom, setZoom] = useState(1);
  const [height, setHeight] = useState(218);
  const [width, setWidth] = useState(900);
  const [resizing, setResizing] = useState<{ y: number; height: number } | null>(null);
  const bodyRef = useRef<HTMLDivElement>(null);
  useEffect(() => {
    if (!bodyRef.current || typeof ResizeObserver === 'undefined') return;
    const observer = new ResizeObserver(entries => setWidth(entries[0].contentRect.width));
    observer.observe(bodyRef.current);
    return () => observer.disconnect();
  }, [collapsed]);
  const signals = useMemo(() => {
    if (!simResult) return [];
    const selected: WaveSignal[] = [];
    for (const device of circuit.devices) {
      if (device.type !== 'CLOCK' && !waveformDeviceIds.includes(device.id)) continue;
      const pins = device.type === 'CLOCK' ? [{ id: 'OUT' }] : devices.find(def => def.type === device.type)?.pins ?? [];
      for (const pin of pins) {
        const signal = findNetworkSignal(circuit, simResult.waveform.signals, device.id, pin.id);
        if (signal) selected.push({ ...signal, id: `${device.id}.${pin.id}`, name: `${device.id}.${pin.id}` });
      }
    }
    return selected;
  }, [circuit, devices, waveformDeviceIds, simResult]);
  const maxTime = Math.max(1, playbackMaxTime, simResult?.ticks_elapsed ?? 100);
  const drawWidth = Math.max(480, width - 190) * zoom;
  const totalHeight = Math.max(92, signals.length * CHANNEL_HEIGHT) + 28;
  const x = (time: number) => 12 + time / maxTime * (drawWidth - 24);
  const signalY = (index: number, value: string) => 28 + index * CHANNEL_HEIGHT + (value === '1' ? 6 : value === '0' ? 28 : 17);
  const cursorTime = playbackTime < 0 ? maxTime : playbackTime;
  function resizeTo(value: number) { setHeight(Math.max(140, Math.min(480, window.innerHeight - 280, value))); }

  return <section className={`scope-panel ${collapsed ? 'collapsed' : ''}`} style={{ height: collapsed ? 43 : height }} aria-label="数字示波器">
    <div className="scope-resize" role="separator" aria-label="调整示波器高度" aria-orientation="horizontal" aria-valuenow={height} aria-valuemin={140} aria-valuemax={480} tabIndex={0}
      onPointerDown={event => { event.currentTarget.setPointerCapture?.(event.pointerId); setResizing({ y: event.clientY, height }); }} onPointerMove={event => { if (resizing) resizeTo(resizing.height + resizing.y - event.clientY); }} onPointerUp={() => setResizing(null)} onPointerCancel={() => setResizing(null)} onKeyDown={event => { if (event.key === 'ArrowUp' || event.key === 'ArrowDown') { event.preventDefault(); resizeTo(height + (event.key === 'ArrowUp' ? 20 : -20)); } }} />
    <div className="scope-heading"><span className="scope-title"><Icon name="wave" size={17} /><h2>示波器</h2><span className="count-badge">{signals.length}</span></span><span className="scope-state"><span className={`status-dot ${simResult ? 'ready' : ''}`} />{simResult ? playbackActive ? '正在回放' : '采集完成' : '等待仿真'}</span><span className="scope-hint">时钟源常驻 · 在属性面板添加通道</span><div className="scope-actions"><button aria-label="缩小波形" disabled={zoom <= 1 || !simResult} onClick={() => setZoom(value => Math.max(1, value - 0.5))}><Icon name="minus" size={14} /></button><span>{zoom.toFixed(1)}×</span><button aria-label="放大波形" disabled={zoom >= 4 || !simResult} onClick={() => setZoom(value => Math.min(4, value + 0.5))}><Icon name="plus" size={14} /></button><span className="control-divider" /><button aria-label={collapsed ? '展开示波器' : '收起示波器'} onClick={() => setCollapsed(value => !value)}><Icon name="chevron" size={15} style={{ transform: `rotate(${collapsed ? -90 : 90}deg)` }} /></button></div></div>
    {!collapsed && <>
      <div className="scope-body" ref={bodyRef}>
        {!simResult ? <div className="scope-empty"><Icon name="wave" size={32} /><div><h3>捕捉逻辑的每一次变化</h3><p>运行仿真后，信号波形将显示在这里。<kbd>Ctrl</kbd> + <kbd>Enter</kbd> 开始仿真</p></div><span className="scope-empty-unit">DIGITAL SIGNAL / tick</span></div>
          : !signals.length ? <div className="scope-empty"><Icon name="wave" size={32} /><div><h3>添加一个观察通道</h3><p>选择器件，在属性面板点击“在示波器显示”。</p></div></div>
          : <div className="scope-scroll"><div className="channel-labels"><div className="channel-label-heading">信号名称<span>电平</span></div>{signals.map((signal, index) => <div className="channel-label" key={signal.id} style={{ height: CHANNEL_HEIGHT }}><span className="channel-color" style={{ background: colors[index % colors.length] }} /><code title={signal.name}>{signal.name}</code><span style={{ color: SIGNAL_COLORS[valueAt(signal, cursorTime) ?? ''] }}>{valueAt(signal, cursorTime) ?? '—'}</span></div>)}</div><div className="wave-scroll"><svg width={drawWidth} height={totalHeight} aria-label="数字信号波形" onClick={event => { const rect = event.currentTarget.getBoundingClientRect(); seekPlayback(Math.round((event.clientX - rect.left - 12) / (drawWidth - 24) * maxTime)); }}>
            {Array.from({ length: 11 }, (_, index) => <g key={index}><line x1={x(maxTime * index / 10)} y1="25" x2={x(maxTime * index / 10)} y2={totalHeight} stroke="#25303c" strokeDasharray="2 4" /><text x={x(maxTime * index / 10)} y="16" textAnchor={index === 0 ? 'start' : index === 10 ? 'end' : 'middle'} fill="#718298" fontSize="9" fontFamily="monospace">{Math.round(maxTime * index / 10)}</text></g>)}
            {signals.map((signal, index) => <g key={signal.id}><line x1="0" y1={28 + (index + 1) * CHANNEL_HEIGHT} x2={drawWidth} y2={28 + (index + 1) * CHANNEL_HEIGHT} stroke="#202a35" />{signal.values.map((point, pointIndex) => {
              const next = signal.values[pointIndex + 1];
              const end = next?.t ?? maxTime;
              const y = signalY(index, point.v);
              return <path key={pointIndex} d={`M ${x(point.t)} ${y} H ${x(end)}${next ? ` V ${signalY(index, next.v)}` : ''}`} fill="none" stroke={point.v === 'X' || point.v === 'Z' ? SIGNAL_COLORS[point.v] : colors[index % colors.length]} strokeOpacity={point.v === '0' ? 0.65 : 1} strokeWidth="1.8" strokeDasharray={point.v === 'X' || point.v === 'Z' ? '4 3' : undefined} />;
            })}</g>)}
            {playbackTime >= 0 && <g pointerEvents="none"><line x1={x(playbackTime)} y1="24" x2={x(playbackTime)} y2={totalHeight} stroke="#eec17d" strokeWidth="1" /><path d={`M ${x(playbackTime) - 4} 19 h8 l-4 6z`} fill="#eec17d" /></g>}
          </svg></div></div>}
      </div>
      <div className="scope-footer"><span className="scope-time"><span className="subtle-label">TIME</span><code>{playbackTime < 0 ? '最终状态' : `${playbackTime} / ${playbackMaxTime}`}</code><span>tick</span></span><div className="playback-controls"><button aria-label={playbackActive ? '暂停回放' : '播放波形'} disabled={!simResult || playbackMaxTime === 0} onClick={playbackActive ? stopPlayback : startPlayback}><Icon name={playbackActive ? 'pause' : 'play'} size={13} /></button><input type="range" aria-label="回放时间" min="0" max={Math.max(1, playbackMaxTime)} value={playbackTime < 0 ? playbackMaxTime : playbackTime} disabled={!simResult} onChange={event => seekPlayback(Number(event.target.value))} /><button className="final-state-button" disabled={!simResult} onClick={() => seekPlayback(-1)}>最终状态</button></div><span className="scope-event-count">{simResult ? `${simResult.events_processed} 个事件已处理` : '四值逻辑 0 / 1 / X / Z'}</span></div>
    </>}
  </section>;
}
