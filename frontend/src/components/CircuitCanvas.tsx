import { useCallback, useEffect, useRef, useState } from 'react';
import { useStore } from '../store/useStore';
import type { CircuitDef, DeviceInstance, PinDef, WireEndpoint } from '../types/circuit';
import { deviceHeight, DEVICE_WIDTH, pinPosition, wirePath } from '../lib/geometry';
import { FIXED_DEVICE_IDS } from '../lib/devices';
import { nodeValue, SIGNAL_COLORS } from '../lib/signals';
import { halfAdder } from '../lib/examples';
import { setCanvasCenter, setCanvasSvgRef } from './canvasExportRef';
import { DeviceGlyph } from './DeviceGlyph';
import { Icon } from './Icon';

type Viewport = { x: number; y: number; zoom: number };
type Gesture = { kind: 'device'; id: string; ox: number; oy: number; before: CircuitDef; startX: number; startY: number }
  | { kind: 'pan'; startX: number; startY: number; x: number; y: number };
const clampZoom = (zoom: number) => Math.max(0.25, Math.min(2, zoom));

export function CircuitCanvas() {
  const circuit = useStore(s => s.circuit);
  const devices = useStore(s => s.devices);
  const selectedDeviceId = useStore(s => s.selectedDeviceId);
  const selectedWireId = useStore(s => s.selectedWireId);
  const selectedPin = useStore(s => s.selectedPin);
  const simResult = useStore(s => s.simResult);
  const playbackTime = useStore(s => s.playbackTime);
  const svgRef = useRef<SVGSVGElement>(null);
  const moved = useRef(false);
  const [viewport, setViewport] = useState<Viewport>({ x: 0, y: 0, zoom: 1 });
  const [gesture, setGesture] = useState<Gesture | null>(null);
  const [mode, setMode] = useState<'select' | 'hand'>('select');
  const [space, setSpace] = useState(false);
  const [mouse, setMouse] = useState<{ x: number; y: number } | null>(null);
  const panning = mode === 'hand' || space;
  const definitions = new Map(devices.map(device => [device.type, device]));

  const fit = useCallback(() => {
    const rect = svgRef.current?.getBoundingClientRect();
    if (!rect?.width || !rect.height) return;
    const state = useStore.getState();
    if (!state.circuit.devices.length) { setViewport({ x: 0, y: 0, zoom: 1 }); return; }
    const bounds = state.circuit.devices.map(device => {
      const height = deviceHeight(state.devices.find(def => def.type === device.type));
      return { left: device.x - DEVICE_WIDTH / 2 - 30, right: device.x + DEVICE_WIDTH / 2 + 30, top: device.y - height / 2 - 50, bottom: device.y + height / 2 + 30 };
    });
    const left = Math.min(...bounds.map(box => box.left)), right = Math.max(...bounds.map(box => box.right));
    const top = Math.min(...bounds.map(box => box.top)), bottom = Math.max(...bounds.map(box => box.bottom));
    const zoom = clampZoom(Math.min((rect.width - 80) / (right - left), (rect.height - 120) / (bottom - top), 1));
    setViewport({ zoom, x: (rect.width - (right + left) * zoom) / 2, y: (rect.height - (bottom + top) * zoom) / 2 + 10 });
  }, []);

  useEffect(() => {
    setCanvasSvgRef(svgRef.current);
    return () => setCanvasSvgRef(null);
  }, []);
  useEffect(() => {
    setCanvasCenter(() => {
      const rect = svgRef.current?.getBoundingClientRect();
      return { x: ((rect?.width ?? 800) / 2 - viewport.x) / viewport.zoom, y: ((rect?.height ?? 400) / 2 - viewport.y) / viewport.zoom };
    });
  }, [viewport]);
  useEffect(() => {
    const svg = svgRef.current;
    function wheel(event: WheelEvent) {
      event.preventDefault();
      const rect = svg?.getBoundingClientRect();
      if (!rect) return;
      const x = event.clientX - rect.left, y = event.clientY - rect.top;
      setViewport(view => {
        const zoom = clampZoom(view.zoom * Math.exp(-event.deltaY * 0.0015));
        return { zoom, x: x - (x - view.x) * zoom / view.zoom, y: y - (y - view.y) * zoom / view.zoom };
      });
    }
    svg?.addEventListener('wheel', wheel, { passive: false });
    window.addEventListener('circuit:fit', fit);
    return () => { svg?.removeEventListener('wheel', wheel); window.removeEventListener('circuit:fit', fit); };
  }, [fit]);
  useEffect(() => {
    function keyDown(event: KeyboardEvent) {
      const target = event.target as HTMLElement;
      if (target?.closest('input, textarea, select, [contenteditable="true"], dialog')) return;
      const state = useStore.getState();
      if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === 'z') { event.preventDefault(); if (event.shiftKey) state.redo(); else state.undo(); }
      else if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === 'y') { event.preventDefault(); state.redo(); }
      else if (event.key === 'Delete' || event.key === 'Backspace') { event.preventDefault(); state.deleteSelected(); }
      else if (event.key === 'Escape') { state.cancelWire(); state.selectDevice(null); state.selectWire(null); }
      else if (event.code === 'Space') { event.preventDefault(); setSpace(true); }
      else if (event.key.toLowerCase() === 'f') fit();
    }
    const keyUp = (event: KeyboardEvent) => { if (event.code === 'Space') setSpace(false); };
    const blur = () => setSpace(false);
    window.addEventListener('keydown', keyDown); window.addEventListener('keyup', keyUp); window.addEventListener('blur', blur);
    return () => { window.removeEventListener('keydown', keyDown); window.removeEventListener('keyup', keyUp); window.removeEventListener('blur', blur); };
  }, [fit]);

  function point(clientX: number, clientY: number) {
    const rect = svgRef.current?.getBoundingClientRect();
    return { x: (clientX - (rect?.left ?? 0) - viewport.x) / viewport.zoom, y: (clientY - (rect?.top ?? 0) - viewport.y) / viewport.zoom };
  }
  function capture(event: React.PointerEvent) { svgRef.current?.setPointerCapture?.(event.pointerId); moved.current = false; }
  function startDevice(event: React.PointerEvent, device: DeviceInstance) {
    if (panning || event.button !== 0) return;
    event.stopPropagation();
    capture(event);
    const cursor = point(event.clientX, event.clientY);
    setGesture({ kind: 'device', id: device.id, ox: cursor.x - device.x, oy: cursor.y - device.y, before: circuit, startX: event.clientX, startY: event.clientY });
  }
  function pointerMove(event: React.PointerEvent) {
    const cursor = point(event.clientX, event.clientY);
    if (selectedPin) setMouse(cursor);
    if (!gesture) return;
    if (Math.abs(event.clientX - gesture.startX) + Math.abs(event.clientY - gesture.startY) < 4) return;
    moved.current = true;
    if (gesture.kind === 'pan') setViewport(view => ({ ...view, x: gesture.x + event.clientX - gesture.startX, y: gesture.y + event.clientY - gesture.startY }));
    else useStore.getState().moveDevice(gesture.id, cursor.x - gesture.ox, cursor.y - gesture.oy, false);
  }
  function pointerUp(event: React.PointerEvent) {
    if (gesture?.kind === 'device' && useStore.getState().circuit !== gesture.before) useStore.getState().checkpoint(gesture.before);
    if (gesture?.kind === 'device' && event.type === 'pointerup') {
      const state = useStore.getState();
      state.selectDevice(moved.current ? gesture.id : state.selectedDeviceId === gesture.id ? null : gesture.id);
    }
    // Pointer capture retargets the browser click to the SVG root.
    // Selection belongs to the gesture; suppress the duplicate click.
    if (gesture) moved.current = true;
    if (svgRef.current?.hasPointerCapture?.(event.pointerId)) svgRef.current.releasePointerCapture(event.pointerId);
    setGesture(null);
  }
  function pinClick(event: React.MouseEvent, endpoint: WireEndpoint) {
    event.stopPropagation();
    const state = useStore.getState();
    if (state.selectedPin) state.completeWire(endpoint); else state.startWire(endpoint);
  }
  const value = (deviceId: string, pinId: string) => nodeValue(circuit, simResult, deviceId, pinId, playbackTime);
  const zoomBy = (factor: number) => {
    const rect = svgRef.current?.getBoundingClientRect();
    const x = (rect?.width ?? 800) / 2, y = (rect?.height ?? 400) / 2;
    setViewport(view => { const zoom = clampZoom(view.zoom * factor); return { zoom, x: x - (x - view.x) * zoom / view.zoom, y: y - (y - view.y) * zoom / view.zoom }; });
  };
  function example() { useStore.getState().replaceCircuit(halfAdder(), '半加器实验'); fit(); }
  const starterOnly = circuit.devices.every(device => FIXED_DEVICE_IDS.has(device.id)) && !circuit.wires.length;

  return <section className="canvas-panel" aria-label="电路工作区">
    <div className="canvas-heading"><span className="workspace-tab"><Icon name="circuit" size={15} />电路工作区<span className="tab-dot" /></span><span className="canvas-heading-hint">20 px 网格 · 自动吸附</span></div>
    <div className="canvas-surface">
      <svg ref={svgRef} className="circuit-svg" aria-label="电路画布" style={{ cursor: gesture ? 'grabbing' : panning ? 'grab' : 'default' }}
        onDrop={event => { event.preventDefault(); const type = event.dataTransfer.getData('device-type'); if (type) { const cursor = point(event.clientX, event.clientY); useStore.getState().addDevice(type, cursor.x, cursor.y); } }}
        onDragOver={event => { event.preventDefault(); event.dataTransfer.dropEffect = 'copy'; }}
        onPointerDown={event => { if (panning || event.button === 1) { event.preventDefault(); capture(event); setGesture({ kind: 'pan', startX: event.clientX, startY: event.clientY, x: viewport.x, y: viewport.y }); } }}
        onPointerMove={pointerMove} onPointerUp={pointerUp} onPointerCancel={pointerUp}
        onClick={() => { if (moved.current) { moved.current = false; return; } const state = useStore.getState(); state.cancelWire(); state.selectDevice(null); state.selectWire(null); }}>
        <defs><pattern id="circuit-dots" width="20" height="20" patternUnits="userSpaceOnUse" patternTransform={`translate(${viewport.x} ${viewport.y}) scale(${viewport.zoom})`}><circle cx="0" cy="0" r="0.8" fill="#2b3643" /></pattern></defs>
        <rect width="100%" height="100%" fill="#10161d" /><rect width="100%" height="100%" fill="url(#circuit-dots)" />
        <g data-circuit-layer transform={`translate(${viewport.x} ${viewport.y}) scale(${viewport.zoom})`}>
          {circuit.wires.map(wire => {
            const fromDevice = circuit.devices.find(device => device.id === wire.from.device), toDevice = circuit.devices.find(device => device.id === wire.to.device);
            const fromDef = fromDevice && definitions.get(fromDevice.type), toDef = toDevice && definitions.get(toDevice.type);
            const fromPin = fromDef?.pins.find(pin => pin.id === wire.from.pin), toPin = toDef?.pins.find(pin => pin.id === wire.to.pin);
            if (!fromDevice || !toDevice || !fromDef || !toDef || !fromPin || !toPin) return null;
            const pair = [wire.from.device, wire.to.device].sort().join(':');
            const siblings = circuit.wires.filter(item => [item.from.device, item.to.device].sort().join(':') === pair);
            const offset = (siblings.indexOf(wire) - (siblings.length - 1) / 2) * 14;
            const path = wirePath(pinPosition(fromDevice, fromDef, fromPin), pinPosition(toDevice, toDef, toPin), offset,
              fromDevice === toDevice ? fromDevice.y - deviceHeight(fromDef) / 2 - 30 - Math.abs(offset) : undefined,
              fromPin.direction === 'input' ? -1 : 1, toPin.direction === 'input' ? -1 : 1);
            const signal = value(wire.from.device, wire.from.pin);
            return <g key={wire.id} className="circuit-wire" data-testid={`wire-${wire.id}`} onPointerDown={event => event.stopPropagation()} onClick={event => { event.stopPropagation(); useStore.getState().selectWire(wire.id === selectedWireId ? null : wire.id); }}>
              <path d={path} fill="none" stroke={wire.id === selectedWireId ? '#eec17d' : signal ? SIGNAL_COLORS[signal] : '#68bda8'} strokeWidth={wire.id === selectedWireId ? 3 : 2} strokeLinejoin="round" />
              <path className="wire-hit-area" d={path} fill="none" stroke="transparent" strokeWidth="14" />
            </g>;
          })}
          {selectedPin && mouse && (() => {
            const device = circuit.devices.find(item => item.id === selectedPin.device), def = device && definitions.get(device.type), pin = def?.pins.find(item => item.id === selectedPin.pin);
            return device && def && pin ? <path d={wirePath(pinPosition(device, def, pin), mouse, 0, undefined, pin.direction === 'input' ? -1 : 1)} fill="none" stroke="#eec17d" strokeWidth="2" strokeDasharray="5 5" pointerEvents="none" /> : null;
          })()}
          {circuit.devices.map(device => {
            const def = definitions.get(device.type), height = deviceHeight(def), selected = selectedDeviceId === device.id;
            const pins = def?.pins ?? [];
            function renderPin(pin: PinDef) {
              if (!def) return null;
              const position = pinPosition(device, def, pin), input = pin.direction === 'input';
              const px = position.x - device.x + DEVICE_WIDTH / 2, py = position.y - device.y + height / 2;
              const signal = value(device.id, pin.id), active = selectedPin?.device === device.id && selectedPin.pin === pin.id;
              return <g key={pin.id} className="circuit-pin" data-testid={`pin-${device.id}-${pin.id}`} onPointerDown={event => event.stopPropagation()} onClick={event => pinClick(event, { device: device.id, pin: pin.id })}>
                <title>{`${device.id}.${pin.id} · ${input ? '输入' : '输出'}${signal ? ` · ${signal}` : ''}`}</title>
                <circle cx={px} cy={py} r="12" fill="transparent" />
                <circle cx={px} cy={py} r={active ? 6 : 4.5} fill={active ? '#eec17d' : signal ? SIGNAL_COLORS[signal] : '#10161d'} stroke={active ? '#eec17d' : input ? '#73b7a9' : '#8eaee2'} strokeWidth="1.6" />
                <text x={input ? px + 12 : px - 12} y={py + 3} textAnchor={input ? 'start' : 'end'} fill={pin.active === 'low' ? '#eec17d' : '#8896a8'} fontSize="9" pointerEvents="none">{pin.name}{pin.active === 'low' ? ' ◌' : ''}</text>
              </g>;
            }
            return <g key={device.id} data-testid={`device-${device.id}`} className="canvas-device" transform={`translate(${device.x - DEVICE_WIDTH / 2} ${device.y - height / 2})`}
              onPointerDown={event => startDevice(event, device)} onClick={event => { event.stopPropagation(); if (moved.current) { moved.current = false; return; } if (!panning) useStore.getState().selectDevice(selected ? null : device.id); }}>
              <title>{`${def?.name ?? device.type} · ${device.id}${def?.description ? `\n${def.description}` : ''}`}</title>
              <text x="0" y="-10" fill="#657589" fontSize="9" fontFamily="monospace" pointerEvents="none">{device.id}</text>
              <rect width={DEVICE_WIDTH} height={height} rx="8" fill={selected ? '#1c302e' : '#1b242e'} stroke={selected ? '#6fe2be' : '#3b4b5d'} strokeWidth={selected ? 1.8 : 1} />
              <path d={`M0 29H${DEVICE_WIDTH}`} stroke="#2d3b49" />
              <text x={DEVICE_WIDTH / 2} y="19" textAnchor="middle" fill="#dbe5ed" fontSize="11" fontWeight="600" pointerEvents="none">{device.type.replaceAll('_', ' ')}</text>
              {['VCC', 'GND'].includes(device.type) ? <text x={DEVICE_WIDTH / 2} y={height / 2 + 13} textAnchor="middle" fill={device.type === 'VCC' ? '#6fe2be' : '#8d9bac'} fontSize="22" fontFamily="monospace" pointerEvents="none">{device.type === 'VCC' ? '1' : '0'}</text>
                : device.type === 'LED' ? <circle cx={DEVICE_WIDTH / 2} cy={height / 2 + 10} r="12" fill={value(device.id, 'IN') === '1' ? '#6fe2be' : '#293442'} stroke={SIGNAL_COLORS[value(device.id, 'IN') ?? ''] ?? '#526276'} strokeWidth="2" />
                : device.type === 'SWITCH' ? <g role="switch" aria-label={`切换开关 ${device.id}`} aria-checked={device.params.value === 1} tabIndex={0} className="canvas-switch" onPointerDown={event => event.stopPropagation()} onClick={event => { event.stopPropagation(); useStore.getState().updateDeviceParam(device.id, 'value', device.params.value === 1 ? 0 : 1); }} onKeyDown={event => { if (event.key === 'Enter' || event.key === ' ') { event.preventDefault(); event.stopPropagation(); useStore.getState().updateDeviceParam(device.id, 'value', device.params.value === 1 ? 0 : 1); } }}><rect x="50" y={height / 2} width="44" height="22" rx="11" fill={device.params.value === 1 ? '#295447' : '#344151'} /><circle cx={device.params.value === 1 ? 83 : 61} cy={height / 2 + 11} r="7" fill={device.params.value === 1 ? '#6fe2be' : '#96a3b4'} /></g>
                : device.type === 'SEVEN_SEGMENT' ? <g transform={`translate(56 ${height / 2 - 12})`}>{['M4 0h24', 'M30 3v20', 'M30 27v20', 'M4 50h24', 'M2 27v20', 'M2 3v20', 'M4 25h24'].map((path, index) => <path key={path} d={path} stroke={value(device.id, 'ABCDEFG'[index]) === '1' ? '#6fe2be' : '#344151'} strokeWidth="4" strokeLinecap="round" />)}</g>
                : <svg x="48" y={height / 2 - 6} width="48" height="30" pointerEvents="none" color={def?.category === 'chip' ? '#92b5e6' : '#76bdab'}><DeviceGlyph type={device.type} className="canvas-glyph" /></svg>}
              {pins.map(renderPin)}
            </g>;
          })}
        </g>
      </svg>
      <div className="canvas-tools"><button className={mode === 'select' ? 'active' : ''} aria-label="选择工具" aria-pressed={mode === 'select'} onClick={() => setMode('select')}><Icon name="cursor" /></button><button className={mode === 'hand' ? 'active' : ''} aria-label="平移工具" aria-pressed={mode === 'hand'} onClick={() => setMode('hand')}><Icon name="hand" /></button><span /><button aria-label="适应画布" title="适应画布 (F)" onClick={fit}><Icon name="fit" /></button></div>
      {selectedPin && <div className="wiring-hint"><Icon name="link" size={14} />选择目标引脚完成连线<kbd>Esc</kbd><button aria-label="取消连线" onClick={() => useStore.getState().cancelWire()}><Icon name="close" size={14} /></button></div>}
      {starterOnly && <div className="canvas-welcome"><span className="welcome-eyebrow">YOUR NEXT CIRCUIT STARTS HERE</span><h1>让逻辑，发生。</h1><p>将器件拖入画布，连接引脚，观察信号如何流动。</p><button className="button secondary" disabled={!devices.some(device => device.type === 'XOR')} onClick={example}><Icon name="circuit" size={16} />加载半加器示例<Icon name="chevron" size={14} /></button><small>左侧已有高电平、低电平和时钟源</small></div>}
      <div className="canvas-bottom"><span className="canvas-shortcuts"><kbd>Space</kbd> 拖动画布<span>·</span>滚轮缩放</span><div className="zoom-controls"><button aria-label="缩小画布" disabled={viewport.zoom <= 0.25} onClick={() => zoomBy(1 / 1.2)}><Icon name="minus" size={14} /></button><button className="zoom-value" aria-label="重置画布视图" onClick={() => setViewport({ x: 0, y: 0, zoom: 1 })}>{Math.round(viewport.zoom * 100)}%</button><button aria-label="放大画布" disabled={viewport.zoom >= 2} onClick={() => zoomBy(1.2)}><Icon name="plus" size={14} /></button></div></div>
    </div>
  </section>;
}
