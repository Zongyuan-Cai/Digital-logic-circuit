import { useRef, useState, useEffect } from 'react';
import { useStore } from '../store/useStore';
import type { DeviceInstance, PinDef, WireEndpoint } from '../types/circuit';
import { setCanvasSvgRef } from './canvasExportRef';

const DEVICE_W = 140;
const PIN_SPACING = 20;
const PIN_RADIUS = 5;

export function CircuitCanvas() {
  const {
    circuit, devices, selectedDeviceId, selectedWireId, selectedPin,
    selectDevice, selectWire, moveDevice, deleteSelected,
    startWire, completeWire, cancelWire,
    playbackTime,
  } = useStore();

  const svgRef = useRef<SVGSVGElement>(null);
  const [dragging, setDragging] = useState<{ id: string; ox: number; oy: number } | null>(null);
  const [mousePos, setMousePos] = useState<{ x: number; y: number } | null>(null);
  const [hoveredWire, setHoveredWire] = useState<string | null>(null);
  const [tooltip, setTooltip] = useState<{ dev: DeviceInstance; def: import('../types/circuit').DeviceDef | undefined; x: number; y: number } | null>(null);

  // Register SVG ref for export
  useEffect(() => {
    setCanvasSvgRef(svgRef.current);
    return () => setCanvasSvgRef(null);
  }, []);

  // Global keyboard handler
  useEffect(() => {
    function onKey(e: KeyboardEvent) {
      if (e.key === 'Delete' || e.key === 'Backspace') {
        // Don't delete if user is typing in an input
        if ((e.target as HTMLElement)?.tagName === 'INPUT') return;
        deleteSelected();
      }
      if (e.key === 'Escape') {
        cancelWire();
        selectDevice(null);
        selectWire(null);
      }
    }
    window.addEventListener('keydown', onKey);
    return () => window.removeEventListener('keydown', onKey);
  }, [deleteSelected, cancelWire, selectDevice, selectWire]);

  const pinDefs: Record<string, PinDef[]> = {};
  for (const d of devices) pinDefs[d.type] = d.pins;

  function getPins(type: string): PinDef[] { return pinDefs[type] || []; }
  function getDeviceById(id: string) { return circuit.devices.find(d => d.id === id); }

  function deviceHeight(type: string): number {
    const pins = getPins(type);
    const inputs = pins.filter(p => p.direction === 'input').length;
    const outputs = pins.filter(p => p.direction === 'output' || p.direction === 'bidirectional').length;
    return Math.max(inputs, outputs) * PIN_SPACING + 40;
  }

  function pinPosition(dev: DeviceInstance, pin: PinDef): { x: number; y: number } {
    const pins = getPins(dev.type);
    const inputs = pins.filter(p => p.direction === 'input');
    const outputs = pins.filter(p => p.direction === 'output' || p.direction === 'bidirectional');
    const isInput = pin.direction === 'input';
    const group = isInput ? inputs : outputs;
    const pinIdx = group.findIndex(p => p.id === pin.id);
    const h = deviceHeight(dev.type);
    const startY = dev.y - h / 2 + 30;
    if (isInput) return { x: dev.x - DEVICE_W / 2, y: startY + pinIdx * PIN_SPACING };
    return { x: dev.x + DEVICE_W / 2, y: startY + pinIdx * PIN_SPACING };
  }

  function getSVGPoint(e: React.MouseEvent): { x: number; y: number } {
    if (!svgRef.current) return { x: 0, y: 0 };
    const rect = svgRef.current.getBoundingClientRect();
    return { x: e.clientX - rect.left, y: e.clientY - rect.top };
  }

  function handleDrop(e: React.DragEvent) {
    e.preventDefault();
    const type = e.dataTransfer.getData('device-type');
    if (!type || !svgRef.current) return;
    const rect = svgRef.current.getBoundingClientRect();
    useStore.getState().addDevice(type, e.clientX - rect.left, e.clientY - rect.top);
  }

  function handleDragOver(e: React.DragEvent) {
    e.preventDefault();
    e.dataTransfer.dropEffect = 'copy';
  }

  // --- Device dragging ---
  function handleDeviceMouseDown(e: React.MouseEvent, devId: string) {
    e.stopPropagation();
    const state = useStore.getState();
    const dev = state.circuit.devices.find(d => d.id === devId);
    if (!dev) return;
    const pt = getSVGPoint(e);
    setDragging({ id: devId, ox: pt.x - dev.x, oy: pt.y - dev.y });
  }

  function handleMouseMove(e: React.MouseEvent) {
    const pt = getSVGPoint(e);
    setMousePos(pt);
    setDragging(d => {
      if (!d) return null;
      moveDevice(d.id, pt.x - d.ox, pt.y - d.oy);
      return d;
    });
  }

  function handleMouseUp() {
    setDragging(null);
  }

  // --- Pin click for wiring ---
  function handlePinClick(e: React.MouseEvent, endpoint: WireEndpoint) {
    e.stopPropagation();
    const state = useStore.getState();
    if (state.selectedPin) {
      const sp = state.selectedPin;
      if (sp.device === endpoint.device && sp.pin === endpoint.pin) {
        useStore.getState().cancelWire();
        return;
      }
      completeWire(endpoint);
    } else {
      startWire(endpoint);
    }
  }

  function handleCanvasClick() {
    const state = useStore.getState();
    if (state.selectedPin) state.cancelWire();
    else {
      state.selectDevice(null);
      state.selectWire(null);
    }
  }

  const getNodeValue = (deviceId: string, pinId: string): string | null => {
    const state = useStore.getState();
    if (!state.simResult) return null;
    if (playbackTime >= 0) {
      return state.getNodeValueAt(deviceId, pinId, playbackTime);
    }
    return state.simResult.final_nodes[`${deviceId}.${pinId}`] || null;
  };

  const signalColor = (v: string): string => {
    switch (v) { case '1': return '#a6e3a1'; case '0': return '#585b70'; case 'X': return '#f38ba8'; case 'Z': return '#89b4fa'; default: return '#6c7086'; }
  };

  const ledFill = (v: string | null): string => {
    if (!v) return '#313244';
    switch (v) { case '1': return '#a6e3a1'; case '0': return '#313244'; case 'X': return '#f38ba8'; case 'Z': return '#89b4fa'; default: return '#313244'; }
  };
  const ledLabel = (v: string | null): string => {
    if (!v) return '?';
    switch (v) { case '1': return '●'; case '0': return '○'; case 'X': return 'X'; case 'Z': return 'Z'; default: return '?'; }
  };
  const segmentOn = (v: string | null): boolean => v === '1';
  const segs7: { id: string; x1: number; y1: number; x2: number; y2: number }[] = [
    { id: 'A', x1: 6, y1: 4, x2: 32, y2: 4 },
    { id: 'B', x1: 32, y1: 6, x2: 32, y2: 22 },
    { id: 'C', x1: 32, y1: 24, x2: 32, y2: 40 },
    { id: 'D', x1: 6, y1: 40, x2: 32, y2: 40 },
    { id: 'E', x1: 4, y1: 24, x2: 4, y2: 40 },
    { id: 'F', x1: 4, y1: 6, x2: 4, y2: 22 },
    { id: 'G', x1: 6, y1: 22, x2: 32, y2: 22 },
  ];

  function wireOffset(wire: { id: string; from: WireEndpoint; to: WireEndpoint }): number {
    const key = [wire.from.device, wire.to.device].sort().join('::');
    const group = circuit.wires
      .filter(w => [w.from.device, w.to.device].sort().join('::') === key)
      .map(w => w.id)
      .sort();
    const idx = Math.max(0, group.indexOf(wire.id));
    return (idx - (group.length - 1) / 2) * 18;
  }

  function wirePath(fx: number, fy: number, tx: number, ty: number, offset = 0): string {
    const midX = (fx + tx) / 2 + offset;
    return `M ${fx} ${fy} L ${midX} ${fy} L ${midX} ${ty} L ${tx} ${ty}`;
  }

  function selfWirePath(dev: DeviceInstance, fx: number, fy: number, tx: number, ty: number, offset = 0): string {
    const h = deviceHeight(dev.type);
    const topY = dev.y - h / 2;
    const bottomY = dev.y + h / 2;
    const routeY = Math.max(16, Math.min(fy, ty) - 36);
    const loopY = routeY < topY - 8 ? routeY + offset : bottomY + 36 + offset;
    const fromDir = fx >= dev.x ? 1 : -1;
    const toDir = tx >= dev.x ? 1 : -1;
    const fromX = fx + fromDir * (34 + Math.abs(offset) / 2);
    const toX = tx + toDir * (34 + Math.abs(offset) / 2);
    return `M ${fx} ${fy} L ${fromX} ${fy} L ${fromX} ${loopY} L ${toX} ${loopY} L ${toX} ${ty} L ${tx} ${ty}`;
  }

  return (
    <svg
      ref={svgRef}
      style={{
        flex: '1 1 auto',
        minWidth: 0,
        minHeight: 0,
        background: '#11111b',
        cursor: dragging ? 'grabbing' : 'crosshair',
      }}
      onDrop={handleDrop}
      onDragOver={handleDragOver}
      onClick={handleCanvasClick}
      onMouseMove={handleMouseMove}
      onMouseUp={handleMouseUp}
      onMouseLeave={handleMouseUp}
    >
      <defs />

      {/* Grid */}
      {Array.from({ length: 40 }, (_, i) => (
        <line key={`gx${i}`} x1={i * 40} y1={0} x2={i * 40} y2={2000} stroke="#181825" strokeWidth={0.5} />
      ))}
      {Array.from({ length: 40 }, (_, i) => (
        <line key={`gy${i}`} x1={0} y1={i * 40} x2={3000} y2={i * 40} stroke="#181825" strokeWidth={0.5} />
      ))}

      {/* Wires */}
      {circuit.wires.map(w => {
        const fromDev = getDeviceById(w.from.device);
        const toDev = getDeviceById(w.to.device);
        if (!fromDev || !toDev) return null;
        const fromPin = getPins(fromDev.type).find(p => p.id === w.from.pin);
        const toPin = getPins(toDev.type).find(p => p.id === w.to.pin);
        if (!fromPin || !toPin) return null;
        const from = pinPosition(fromDev, fromPin);
        const to = pinPosition(toDev, toPin);
        const isSelected = w.id === selectedWireId;
        const isHovered = w.id === hoveredWire;
        const strokeW = isSelected ? 3 : isHovered ? 3 : 2;
        const strokeC = isSelected ? '#f9e2af' : isHovered ? '#cba6f7' : '#89b4fa';
        const offset = wireOffset(w);
        const pathD = w.from.device === w.to.device
          ? selfWirePath(fromDev, from.x, from.y, to.x, to.y, offset)
          : wirePath(from.x, from.y, to.x, to.y, offset);

        return (
          <g key={w.id}>
            <path
              d={pathD}
              fill="none" stroke={strokeC} strokeWidth={strokeW}
              strokeLinecap="square" strokeLinejoin="miter"
              style={{ cursor: 'pointer', transition: 'stroke-width 0.15s' }}
              onClick={(e) => { e.stopPropagation(); selectWire(w.id === selectedWireId ? null : w.id); }}
              onMouseEnter={() => setHoveredWire(w.id)}
              onMouseLeave={() => setHoveredWire(null)}
            />
            <path
              d={pathD}
              fill="none" stroke="transparent" strokeWidth={14}
              strokeLinecap="square" strokeLinejoin="miter"
              style={{ cursor: 'pointer' }}
              onClick={(e) => { e.stopPropagation(); selectWire(w.id === selectedWireId ? null : w.id); }}
              onMouseEnter={() => setHoveredWire(w.id)}
              onMouseLeave={() => setHoveredWire(null)}
            />
          </g>
        );
      })}

      {/* Temp wire following mouse */}
      {selectedPin && mousePos && (() => {
        const dev = getDeviceById(selectedPin.device);
        if (!dev) return null;
        const pin = getPins(dev.type).find(p => p.id === selectedPin.pin);
        if (!pin) return null;
        const pos = pinPosition(dev, pin);
        return (
          <path
            d={wirePath(pos.x, pos.y, mousePos.x, mousePos.y)}
            fill="none" stroke="#f9e2af" strokeWidth={2} strokeDasharray="6,4"
            strokeLinecap="square" strokeLinejoin="miter"
          />
        );
      })()}

      {/* Wiring hint */}
      {selectedPin && (
        <text x="50%" y={20} textAnchor="middle" fill="#f9e2af" fontSize={12}>
          点击目标引脚完成连线 (Esc 取消)
        </text>
      )}

      {/* Devices */}
      {circuit.devices.map(dev => {
        const def = devices.find(d => d.type === dev.type);
        const isSelected = dev.id === selectedDeviceId;
        const pins = getPins(dev.type);
        const h = deviceHeight(dev.type);
        const simResult = useStore.getState().simResult;

        // Check for floating inputs
        const floatingInputs: string[] = [];
        if (simResult) {
          for (const pin of pins) {
            if (pin.direction === 'input') {
              const wired = circuit.wires.some(w =>
                (w.to.device === dev.id && w.to.pin === pin.id) ||
                (w.from.device === dev.id && w.from.pin === pin.id)
              );
              if (!wired) floatingInputs.push(pin.id);
            }
          }
        }

        return (
          <g key={dev.id}
            data-testid={`device-${dev.id}`}
            transform={`translate(${dev.x - DEVICE_W / 2}, ${dev.y - h / 2})`}
            style={{ cursor: dragging?.id === dev.id ? 'grabbing' : 'grab' }}
            onClick={(e) => {
              e.stopPropagation();
              const currentSelectedDeviceId = useStore.getState().selectedDeviceId;
              if (dev.id === currentSelectedDeviceId) {
                selectDevice(null);
              } else {
                selectDevice(dev.id);
              }
            }}
            onMouseEnter={() => {
              const def = devices.find(d => d.type === dev.type);
              setTooltip({ dev, def, x: dev.x, y: dev.y - h / 2 - 8 });
            }}
            onMouseLeave={() => setTooltip(null)}
          >
            {/* Body */}
            <rect x={0} y={0} width={DEVICE_W} height={h} rx={6}
              fill={isSelected ? '#313244' : '#1e1e2e'}
              stroke={isSelected ? '#89b4fa' : '#45475a'}
              strokeWidth={isSelected ? 2 : 1}
              onMouseDown={(e) => handleDeviceMouseDown(e, dev.id)}
            />
            {/* Title */}
            <text x={DEVICE_W / 2} y={18} textAnchor="middle"
              fill="#cdd6f4" fontSize={11} fontWeight={600}
              onMouseDown={(e) => handleDeviceMouseDown(e, dev.id)}
              style={{ pointerEvents: 'none' }}
            >
              {(() => { const raw = def?.name || dev.type; const i = raw.search(/[一-鿿]/); return i > 0 ? raw.slice(i) : raw; })()}
            </text>

            {/* LED visual indicator */}
            {dev.type === 'LED' && (
              <g>
                <circle cx={DEVICE_W / 2} cy={h / 2 + 10} r={14}
                  fill={ledFill(getNodeValue(dev.id, 'IN'))}
                  stroke="#45475a" strokeWidth={1.5}
                />
                <text x={DEVICE_W / 2} y={h / 2 + 14} textAnchor="middle"
                  fill="#1e1e2e" fontSize={8} fontWeight={700}>
                  {ledLabel(getNodeValue(dev.id, 'IN'))}
                </text>
              </g>
            )}

            {/* 7-segment visual */}
            {dev.type === 'SEVEN_SEGMENT' && (
              <g transform={`translate(${DEVICE_W / 2 - 20}, ${h / 2 - 22})`}>
                <rect x={0} y={0} width={40} height={48} rx={2} fill="#11111b" stroke="#313244" strokeWidth={0.5} />
                {segs7.map(s => {
                  const on = segmentOn(getNodeValue(dev.id, s.id));
                  return (
                    <line key={s.id} x1={s.x1} y1={s.y1} x2={s.x2} y2={s.y2}
                      stroke={on ? '#a6e3a1' : '#1e1e2e'} strokeWidth={3} strokeLinecap="round" />
                  );
                })}
              </g>
            )}

            {/* VCC/GND output indicator */}
            {(dev.type === 'VCC' || dev.type === 'GND') && (
              <text x={DEVICE_W / 2} y={h / 2 + 8} textAnchor="middle"
                fill={dev.type === 'VCC' ? '#a6e3a1' : '#585b70'} fontSize={18} fontWeight={700}>
                {dev.type === 'VCC' ? '1' : '0'}
              </text>
            )}

            {/* SWITCH indicator */}
            {dev.type === 'SWITCH' && (
              <g>
                <rect
                  x={DEVICE_W / 2 - 24} y={h / 2 - 2}
                  width={48} height={20} rx={10}
                  fill={dev.params?.value === 1 ? '#a6e3a120' : '#585b7020'}
                  stroke={dev.params?.value === 1 ? '#a6e3a1' : '#585b70'}
                  strokeWidth={1.5}
                  style={{ cursor: 'pointer' }}
                  onClick={(e) => {
                    e.stopPropagation();
                    const cur = dev.params?.value === 1 ? 1 : 0;
                    useStore.getState().updateDeviceParam(dev.id, 'value', cur ? 0 : 1);
                  }}
                />
                <circle
                  cx={dev.params?.value === 1 ? DEVICE_W / 2 + 14 : DEVICE_W / 2 - 14}
                  cy={h / 2 + 8}
                  r={7}
                  fill={dev.params?.value === 1 ? '#a6e3a1' : '#585b70'}
                  stroke={dev.params?.value === 1 ? '#a6e3a1' : '#585b70'}
                  strokeWidth={1}
                  style={{ cursor: 'pointer', transition: 'cx 0.15s' }}
                  onClick={(e) => {
                    e.stopPropagation();
                    const cur = dev.params?.value === 1 ? 1 : 0;
                    useStore.getState().updateDeviceParam(dev.id, 'value', cur ? 0 : 1);
                  }}
                />
                <text x={DEVICE_W / 2 - 28} y={h / 2 + 4} textAnchor="end"
                  fill={dev.params?.value === 1 ? '#a6e3a1' : '#6c7086'} fontSize={11} fontWeight={600}>
                  {dev.params?.value === 1 ? 'ON' : 'OFF'}
                </text>
              </g>
            )}

            {/* Pins */}
            {pins.map((pin) => {
              const isInput = pin.direction === 'input';
              const inputs = pins.filter(p => p.direction === 'input');
              const outputs = pins.filter(p => p.direction === 'output' || p.direction === 'bidirectional');
              const pinIdx = isInput ? inputs.indexOf(pin) : outputs.indexOf(pin);
              const px = isInput ? 0 : DEVICE_W;
              const py = 30 + pinIdx * PIN_SPACING;
              const val = getNodeValue(dev.id, pin.id);
              const isFloating = floatingInputs.includes(pin.id);
              const isPinSelected = selectedPin?.device === dev.id && selectedPin?.pin === pin.id;

              return (
                <g key={pin.id} style={{ cursor: 'pointer' }}
                  onClick={(e) => handlePinClick(e, { device: dev.id, pin: pin.id })}>
                  <circle cx={px} cy={py} r={PIN_RADIUS}
                    fill={val ? signalColor(val) : (isPinSelected ? '#f9e2af' : '#45475a')}
                    stroke={isFloating ? '#f9e2af' : (isInput ? '#a6e3a1' : '#89b4fa')}
                    strokeWidth={isFloating ? 2.5 : 1.5}
                  />
                  <text
                    x={isInput ? px + 10 : px - 10}
                    y={py + 4}
                    textAnchor={isInput ? 'start' : 'end'}
                    fill={pin.active === 'low' ? '#f38ba8' : '#a6adc8'}
                    fontSize={9}
                  >
                    {pin.name}{pin.active === 'low' ? '◌' : ''}
                  </text>
                </g>
              );
            })}
          </g>
        );
      })}

      {/* Empty canvas hint */}
      {circuit.devices.length === 0 && (
        <text x="50%" y="50%" textAnchor="middle" fill="#585b70" fontSize={18}>
          从左侧拖拽器件到此处开始搭建电路
        </text>
      )}

      {/* Hover tooltip */}
      {tooltip && (() => {
        const d = tooltip.def;
        if (!d) return null;
        const lines: string[] = [d.name, `型号: ${d.type}`];
        if (d.description) lines.push(d.description);
        const pinCount = d.pins.length;
        const inputs = d.pins.filter(p => p.direction === 'input').length;
        const outputs = d.pins.filter(p => p.direction === 'output' || p.direction === 'bidirectional').length;
        lines.push(`引脚: ${pinCount} (入${inputs} / 出${outputs})`);
        const tw = Math.max(...lines.map(l => l.length)) * 7 + 20;
        const th = lines.length * 16 + 16;
        const tx = Math.min(tooltip.x, 2800);
        const ty = Math.max(tooltip.y - th, 8);
        return (
          <g>
            <rect x={tx} y={ty} width={tw} height={th} rx={6}
              fill="#1e1e2e" stroke="#89b4fa" strokeWidth={1.5}
              opacity={0.95}
            />
            {lines.map((l, i) => (
              <text key={i} x={tx + 10} y={ty + 16 + i * 16}
                fill={i === 0 ? '#cdd6f4' : '#a6adc8'}
                fontSize={i === 0 ? 12 : 10}
                fontWeight={i === 0 ? 600 : 400}
              >
                {l}
              </text>
            ))}
          </g>
        );
      })()}
    </svg>
  );
}
