/* SVG circuit canvas with drag-and-drop and wiring */

import { useRef, useCallback } from 'react';
import { useStore } from '../store/useStore';
import type { DeviceInstance, PinDef, WireEndpoint } from '../types/circuit';

const DEVICE_W = 140;
const PIN_SPACING = 20;
const PIN_RADIUS = 5;

export function CircuitCanvas() {
  const {
    circuit, devices, selectedDeviceId, selectedPin,
    selectDevice, addDevice,
    startWire, completeWire, cancelWire, removeWire,
  } = useStore();

  const svgRef = useRef<SVGSVGElement>(null);

  // Build a lookup: device type -> pin definitions
  const pinDefs: Record<string, PinDef[]> = {};
  for (const d of devices) {
    pinDefs[d.type] = d.pins;
  }

  function getPins(type: string): PinDef[] {
    return pinDefs[type] || [];
  }

  function getDeviceById(id: string) {
    return circuit.devices.find(d => d.id === id);
  }

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
    if (isInput) {
      return { x: dev.x - DEVICE_W / 2, y: startY + pinIdx * PIN_SPACING };
    }
    return { x: dev.x + DEVICE_W / 2, y: startY + pinIdx * PIN_SPACING };
  }

  const handleDrop = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    const type = e.dataTransfer.getData('device-type');
    if (!type || !svgRef.current) return;
    const rect = svgRef.current.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;
    addDevice(type, x, y);
  }, [addDevice]);

  const handleDragOver = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    e.dataTransfer.dropEffect = 'copy';
  }, []);

  const handlePinClick = useCallback((e: React.MouseEvent, endpoint: WireEndpoint) => {
    e.stopPropagation();
    if (selectedPin) {
      completeWire(endpoint);
    } else {
      startWire(endpoint);
    }
  }, [selectedPin, startWire, completeWire]);

  const handleCanvasClick = useCallback(() => {
    selectDevice(null);
    if (selectedPin) cancelWire();
  }, [selectDevice, selectedPin, cancelWire]);

  const getNodeValue = (deviceId: string, pinId: string): string | null => {
    const result = useStore.getState().simResult;
    if (!result) return null;
    const key = `${deviceId}.${pinId}`;
    return result.final_nodes[key] || null;
  };

  const signalColor = (v: string): string => {
    switch (v) {
      case '1': return '#a6e3a1';
      case '0': return '#585b70';
      case 'X': return '#f38ba8';
      case 'Z': return '#89b4fa';
      default: return '#6c7086';
    }
  };

  // Compute wire paths
  function wirePath(fromX: number, fromY: number, toX: number, toY: number): string {
    const midX = (fromX + toX) / 2;
    return `M ${fromX} ${fromY} C ${midX} ${fromY}, ${midX} ${toY}, ${toX} ${toY}`;
  }

  return (
    <svg
      ref={svgRef}
      style={{ flex: 1, background: '#11111b', cursor: 'crosshair' }}
      onDrop={handleDrop}
      onDragOver={handleDragOver}
      onClick={handleCanvasClick}
    >
      <defs>
        <marker id="arrow" viewBox="0 0 10 10" refX={9} refY={5} markerWidth={6} markerHeight={6} orient="auto-start-reverse">
          <path d="M 0 0 L 10 5 L 0 10 z" fill="#89b4fa" />
        </marker>
      </defs>

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
        const fromPins = getPins(getDeviceById(w.from.device)?.type || '');
        const toPins = getPins(getDeviceById(w.to.device)?.type || '');
        const fromPin = fromPins.find(p => p.id === w.from.pin);
        const toPin = toPins.find(p => p.id === w.to.pin);
        if (!fromPin || !toPin) return null;
        const from = pinPosition(fromDev, fromPin);
        const to = pinPosition(toDev, toPin);
        return (
          <g key={w.id}>
            <path
              d={wirePath(from.x, from.y, to.x, to.y)}
              fill="none"
              stroke={selectedPin ? '#585b70' : '#89b4fa'}
              strokeWidth={2}
              markerEnd="url(#arrow)"
              style={{ cursor: 'pointer' }}
              onClick={(e) => { e.stopPropagation(); removeWire(w.id); }}
            />
            {/* Invisible wider path for easier clicking */}
            <path
              d={wirePath(from.x, from.y, to.x, to.y)}
              fill="none"
              stroke="transparent"
              strokeWidth={12}
              style={{ cursor: 'pointer' }}
              onClick={(e) => { e.stopPropagation(); removeWire(w.id); }}
            />
          </g>
        );
      })}

      {/* Temporary wire being drawn */}
      {selectedPin && (() => {
        const dev = getDeviceById(selectedPin.device);
        if (!dev) return null;
        const pins = getPins(dev.type);
        const pin = pins.find(p => p.id === selectedPin.pin);
        if (!pin) return null;
        const pos = pinPosition(dev, pin);
        return (
          <line x1={pos.x} y1={pos.y} x2={pos.x + 100} y2={pos.y}
            stroke="#f9e2af" strokeWidth={2} strokeDasharray="5,5" />
        );
      })()}

      {/* Devices */}
      {circuit.devices.map(dev => {
        const def = devices.find(d => d.type === dev.type);
        const isSelected = dev.id === selectedDeviceId;
        const pins = getPins(dev.type);
        const h = deviceHeight(dev.type);

        return (
          <g key={dev.id}
            transform={`translate(${dev.x - DEVICE_W / 2}, ${dev.y - h / 2})`}
            onClick={(e) => { e.stopPropagation(); selectDevice(dev.id); }}
            style={{ cursor: 'pointer' }}
          >
            {/* Body */}
            <rect x={0} y={0} width={DEVICE_W} height={h} rx={6}
              fill={isSelected ? '#313244' : '#1e1e2e'}
              stroke={isSelected ? '#89b4fa' : '#45475a'}
              strokeWidth={isSelected ? 2 : 1}
            />
            {/* Title */}
            <text x={DEVICE_W / 2} y={18} textAnchor="middle"
              fill="#cdd6f4" fontSize={11} fontWeight={600}>
              {def?.name || dev.type}
            </text>

            {/* Pins */}
            {pins.map((pin) => {
              const isInput = pin.direction === 'input';
              const inputs = pins.filter(p => p.direction === 'input');
              const outputs = pins.filter(p => p.direction === 'output' || p.direction === 'bidirectional');
              const pinIdx = isInput ? inputs.indexOf(pin) : outputs.indexOf(pin);
              const px = isInput ? 0 : DEVICE_W;
              const py = 30 + pinIdx * PIN_SPACING;
              const val = getNodeValue(dev.id, pin.id);

              return (
                <g key={pin.id} style={{ cursor: 'pointer' }}
                  onClick={(e) => handlePinClick(e, { device: dev.id, pin: pin.id })}>
                  {/* Pin circle */}
                  <circle cx={px} cy={py} r={PIN_RADIUS}
                    fill={val ? signalColor(val) : (selectedPin?.device === dev.id && selectedPin?.pin === pin.id ? '#f9e2af' : '#45475a')}
                    stroke={isInput ? '#a6e3a1' : '#89b4fa'}
                    strokeWidth={1.5}
                  />
                  {/* Label */}
                  <text
                    x={isInput ? px + 10 : px - 10}
                    y={py + 4}
                    textAnchor={isInput ? 'start' : 'end'}
                    fill={pin.active === 'low' ? '#f38ba8' : '#a6adc8'}
                    fontSize={9}
                  >
                    {pin.name}
                    {pin.active === 'low' && '◌'}
                  </text>
                </g>
              );
            })}
          </g>
        );
      })}

      {/* Hint text */}
      {circuit.devices.length === 0 && (
        <text x="50%" y="50%" textAnchor="middle" fill="#585b70" fontSize={18}>
          从左侧拖拽器件到此处开始搭建电路
        </text>
      )}
    </svg>
  );
}
