import { create } from 'zustand';
import type {
  DeviceDef, DeviceInstance, WireDef, WireEndpoint, CircuitDef,
  SimResult, ValidationMessage
} from '../types/circuit';
import { api } from '../api/client';

// VCC, GND, and CLOCK are fixed starter sources on the canvas.
const CANVAS_FIXED_DEVICES: DeviceInstance[] = [
  { id: 'd_vcc',           type: 'VCC',           x: 120, y: 160, params: { delay: 1 } },
  { id: 'd_gnd',           type: 'GND',           x: 120, y: 240, params: { delay: 1 } },
  { id: 'd_clock',         type: 'CLOCK',         x: 120, y: 320, params: { delay: 1, period: 2, duty: 0.5, initial: '0' } },
];

let _playbackTimer: ReturnType<typeof setInterval> | null = null;
function clearPlaybackTimer(): void {
  if (_playbackTimer) {
    clearInterval(_playbackTimer);
    _playbackTimer = null;
  }
}

let _nextId = CANVAS_FIXED_DEVICES.length + 1;
function uid(): string { return `d${_nextId++}`; }
function wid(): string { return `w${_nextId++}`; }
function syncNextId(circuit: CircuitDef): void {
  let max = CANVAS_FIXED_DEVICES.length;
  for (const item of [...circuit.devices, ...circuit.wires]) {
    const match = /^[dw](\d+)$/.exec(item.id);
    if (match) max = Math.max(max, Number(match[1]));
  }
  _nextId = Math.max(_nextId, max + 1);
}

function simulationRecordPins(circuit: CircuitDef, devices: DeviceDef[]): string[] {
  const records: string[] = [];
  for (const dev of circuit.devices) {
    const def = devices.find(d => d.type === dev.type);
    if (!def) continue;
    for (const pin of def.pins) {
      records.push(`${dev.id}.${pin.id}`);
    }
  }
  return records;
}

export interface AppState {
  devices: DeviceDef[];
  devicesLoaded: boolean;

  circuit: CircuitDef;
  selectedDeviceId: string | null;
  selectedWireId: string | null;
  selectedPin: WireEndpoint | null;

  simResult: SimResult | null;
  simRunning: boolean;
  validationMessages: ValidationMessage[];

  // Playback
  playbackTime: number;       // current display time (-1 = show final nodes)
  playbackMaxTime: number;    // max time in waveform
  playbackActive: boolean;
  waveformDeviceIds: string[];

  // Actions
  loadDevices: () => Promise<void>;
  addDevice: (type: string, x: number, y: number) => void;
  moveDevice: (id: string, x: number, y: number) => void;
  removeDevice: (id: string) => void;
  selectDevice: (id: string | null) => void;
  selectWire: (id: string | null) => void;
  deleteSelected: () => void;
  updateDeviceParam: (deviceId: string, key: string, value: unknown) => void;
  startWire: (endpoint: WireEndpoint) => void;
  completeWire: (endpoint: WireEndpoint) => void;
  removeWire: (id: string) => void;
  cancelWire: () => void;
  runSimulation: () => Promise<void>;
  resetSimulation: () => void;
  clearCanvas: () => void;
  startPlayback: () => void;
  stopPlayback: () => void;
  toggleWaveformDevice: (deviceId: string) => void;
  getNodeValueAt: (deviceId: string, pinId: string, time: number) => string | null;

  saveProject: (name: string) => Promise<string>;
  loadProject: (id: string) => Promise<void>;
}

export const useStore = create<AppState>((set, get) => ({
  devices: [],
  devicesLoaded: false,
  circuit: { version: '1.0', devices: [...CANVAS_FIXED_DEVICES], wires: [] },
  selectedDeviceId: null,
  selectedWireId: null,
  selectedPin: null,
  simResult: null,
  simRunning: false,
  validationMessages: [],
  playbackTime: -1,
  playbackMaxTime: 0,
  playbackActive: false,
  waveformDeviceIds: [],

  loadDevices: async () => {
    const devices = await api.listDevices();
    set({ devices, devicesLoaded: true });
  },

  addDevice: (type, x, y) => {
    clearPlaybackTimer();
    const rx = Math.round(x / 20) * 20;
    const ry = Math.round(y / 20) * 20;
    const dev: DeviceInstance = { id: uid(), type, x: rx, y: ry, params: { delay: 1 } };
    set(s => ({
      circuit: { ...s.circuit, devices: [...s.circuit.devices, dev] },
      simResult: null,
      playbackActive: false,
      playbackTime: -1,
    }));
  },

  moveDevice: (id, x, y) => {
    const rx = Math.round(x / 20) * 20;
    const ry = Math.round(y / 20) * 20;
    set(s => ({
      circuit: {
        ...s.circuit,
        devices: s.circuit.devices.map(d =>
          d.id === id ? { ...d, x: rx, y: ry } : d
        ),
      },
    }));
  },

  removeDevice: (id) => {
    // Prevent removing the fixed canvas devices
    if (CANVAS_FIXED_DEVICES.some(fd => fd.id === id)) return;
    clearPlaybackTimer();
    set(s => ({
      circuit: {
        ...s.circuit,
        devices: s.circuit.devices.filter(d => d.id !== id),
        wires: s.circuit.wires.filter(
          w => w.from.device !== id && w.to.device !== id
        ),
      },
      selectedDeviceId: s.selectedDeviceId === id ? null : s.selectedDeviceId,
      waveformDeviceIds: s.waveformDeviceIds.filter(deviceId => deviceId !== id),
      simResult: null,
      playbackActive: false,
      playbackTime: -1,
    }));
  },

  selectDevice: (id) => {
    set({ selectedDeviceId: id, selectedWireId: null });
  },

  selectWire: (id) => {
    set({ selectedWireId: id, selectedDeviceId: null });
  },

  deleteSelected: () => {
    const { selectedDeviceId, selectedWireId, removeDevice, removeWire } = get();
    if (selectedDeviceId) removeDevice(selectedDeviceId);
    if (selectedWireId) removeWire(selectedWireId);
  },

  updateDeviceParam: (deviceId: string, key: string, value: unknown) => {
    clearPlaybackTimer();
    set(s => ({
      circuit: {
        ...s.circuit,
        devices: s.circuit.devices.map(d =>
          d.id === deviceId
            ? { ...d, params: { ...d.params, [key]: value } }
            : d
        ),
      },
      simResult: null,
      playbackActive: false,
      playbackTime: -1,
    }));
  },

  startWire: (endpoint) => {
    set({ selectedPin: endpoint });
  },

  completeWire: (endpoint) => {
    const { selectedPin, circuit } = get();
    if (!selectedPin) return;
    if (selectedPin.device === endpoint.device && selectedPin.pin === endpoint.pin) {
      set({ selectedPin: null });
      return;
    }
    const exists = circuit.wires.some(w =>
      (w.from.device === selectedPin.device && w.from.pin === selectedPin.pin &&
       w.to.device === endpoint.device && w.to.pin === endpoint.pin) ||
      (w.from.device === endpoint.device && w.from.pin === endpoint.pin &&
       w.to.device === selectedPin.device && w.to.pin === selectedPin.pin)
    );
    if (exists) {
      set({ selectedPin: null });
      return;
    }
    const wire: WireDef = { id: wid(), from: selectedPin, to: endpoint };
    clearPlaybackTimer();
    set(s => ({
      circuit: { ...s.circuit, wires: [...s.circuit.wires, wire] },
      selectedPin: null,
      simResult: null,
      playbackActive: false,
      playbackTime: -1,
    }));
  },

  removeWire: (id) => {
    clearPlaybackTimer();
    set(s => ({
      circuit: { ...s.circuit, wires: s.circuit.wires.filter(w => w.id !== id) },
      selectedWireId: s.selectedWireId === id ? null : s.selectedWireId,
      simResult: null,
      playbackActive: false,
      playbackTime: -1,
    }));
  },

  cancelWire: () => {
    set({ selectedPin: null });
  },

  runSimulation: async () => {
    const { circuit } = get();
    clearPlaybackTimer();
    set({ simRunning: true, simResult: null, validationMessages: [], playbackActive: false, playbackTime: -1 });
    try {
      const validation = await api.validate(circuit);
      if (!validation.valid) {
        set({ simRunning: false, validationMessages: validation.errors });
        return;
      }
      const recordPins = simulationRecordPins(circuit, get().devices);
      const result = await api.run(circuit, {
        max_ticks: 100,
        record_all: recordPins.length === 0,
        record: recordPins,
        default_delay: 1,
      });
      // Merge validation warnings into messages for display
      const simWarnings: ValidationMessage[] = [];
      for (const w of validation.warnings) {
        simWarnings.push({ severity: 'warning', message: w.message, detail: w.detail });
      }
      for (const w of result.warnings) {
        if (w === 'Max ticks reached') continue;
        simWarnings.push({ severity: 'warning', message: w, detail: '' });
      }
      set({ simResult: result, simRunning: false, validationMessages: simWarnings });
      // Auto-start playback
      get().startPlayback();
    } catch (e: unknown) {
      set({
        simRunning: false,
        validationMessages: [{
          severity: 'error',
          message: 'Simulation failed',
          detail: e instanceof Error ? e.message : String(e),
        }],
      });
    }
  },

  resetSimulation: () => {
    clearPlaybackTimer();
    set({ simResult: null, validationMessages: [], playbackActive: false, playbackTime: -1 });
  },

  clearCanvas: () => {
    clearPlaybackTimer();
    const fixedIds = new Set(CANVAS_FIXED_DEVICES.map(d => d.id));
    set(s => ({
      circuit: {
        ...s.circuit,
        devices: s.circuit.devices.filter(d => fixedIds.has(d.id)),
        wires: [],
      },
      selectedDeviceId: null,
      selectedWireId: null,
      selectedPin: null,
      waveformDeviceIds: [],
      simResult: null,
      validationMessages: [],
      playbackActive: false,
      playbackTime: -1,
    }));
  },

  startPlayback: () => {
    const { simResult } = get();
    if (!simResult?.waveform?.signals?.length) return;
    // Find max time
    let maxT = 0;
    for (const sig of simResult.waveform.signals) {
      for (const vp of sig.values) {
        if (vp.t > maxT) maxT = vp.t;
      }
    }
    if (maxT === 0) return;

    // Stop any existing playback
    clearPlaybackTimer();

    set({ playbackTime: 0, playbackMaxTime: maxT, playbackActive: true });

    _playbackTimer = setInterval(() => {
      const s = get();
      if (!s.playbackActive) {
        if (_playbackTimer) { clearInterval(_playbackTimer); _playbackTimer = null; }
        return;
      }
      const next = s.playbackTime + 1;
      if (next > s.playbackMaxTime) {
        // Loop: restart from 0
        set({ playbackTime: 0 });
      } else {
        set({ playbackTime: next });
      }
    }, 500); // 2 steps per second = 1Hz for period=2 clock
  },

  stopPlayback: () => {
    clearPlaybackTimer();
    set({ playbackActive: false, playbackTime: -1 });
  },

  toggleWaveformDevice: (deviceId: string) => {
    set(s => ({
      waveformDeviceIds: s.waveformDeviceIds.includes(deviceId)
        ? s.waveformDeviceIds.filter(id => id !== deviceId)
        : [...s.waveformDeviceIds, deviceId],
    }));
  },

  getNodeValueAt: (deviceId: string, pinId: string, time: number): string | null => {
    const { simResult, circuit } = get();
    if (!simResult) return null;
    const key = `${deviceId}.${pinId}`;
    // If showing final state
    if (time < 0) return simResult.final_nodes[key] || null;

    const networkPins = new Set<string>([key]);
    const queue = [{ device: deviceId, pin: pinId }];
    while (queue.length > 0) {
      const current = queue.shift();
      if (!current) break;
      for (const w of circuit.wires) {
        if (w.from.device === current.device && w.from.pin === current.pin) {
          const nextKey = `${w.to.device}.${w.to.pin}`;
          if (!networkPins.has(nextKey)) {
            networkPins.add(nextKey);
            queue.push(w.to);
          }
        }
        if (w.to.device === current.device && w.to.pin === current.pin) {
          const nextKey = `${w.from.device}.${w.from.pin}`;
          if (!networkPins.has(nextKey)) {
            networkPins.add(nextKey);
            queue.push(w.from);
          }
        }
      }
    }

    const valueAt = (values: { t: number; v: string }[]): string | null => {
      let val: string | null = null;
      for (const vp of values) {
        if (vp.t <= time) val = vp.v;
        else break;
      }
      return val;
    };

    // Prefer exact device.pin signals when available.
    for (const sig of simResult.waveform.signals) {
      if (networkPins.has(sig.id) || networkPins.has(sig.name)) {
        return valueAt(sig.values);
      }
    }

    // record_all may only expose node names such as IN/OUT. For a connected
    // display input, choose a matching waveform with real non-Z data.
    const pinNames = new Set([...networkPins].map(name => name.split('.')[1]));
    const candidates = simResult.waveform.signals.filter(sig =>
      pinNames.has(sig.name) || pinNames.has(sig.id)
    );
    const best = candidates.find(sig =>
      (sig.name === pinId || sig.id === pinId) && sig.values.some(vp => vp.v !== 'Z')
    ) ?? candidates.find(sig =>
      sig.values.some(vp => vp.v !== 'Z')
    ) ?? candidates[0];
    if (best) return valueAt(best.values);

    // Fall back to final_nodes
    return simResult.final_nodes[key] || null;
  },

  saveProject: async (name: string) => {
    const { circuit } = get();
    const meta = await api.createProject(name);
    await api.updateProject(meta.id, { circuit });
    return meta.id;
  },

  loadProject: async (id: string) => {
    const project = await api.getProject(id);
    syncNextId(project.circuit);
    set({ circuit: project.circuit, simResult: null, validationMessages: [] });
  },
}));
