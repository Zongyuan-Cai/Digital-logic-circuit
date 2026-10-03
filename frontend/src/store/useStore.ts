import { create } from 'zustand';
import type { DeviceDef, DeviceInstance, WireEndpoint, CircuitDef, SimResult, ValidationMessage } from '../types/circuit';
import { api } from '../api/client';
import { defaultParams, FIXED_DEVICE_IDS, starterCircuit } from '../lib/devices';
import { snap } from '../lib/geometry';
import { nodeValue } from '../lib/signals';

let nextId = 4;
let simulationRequest = 0;
let playbackTimer: ReturnType<typeof setInterval> | null = null;
function clearPlaybackTimer() {
  if (playbackTimer !== null) clearInterval(playbackTimer);
  playbackTimer = null;
}
function uid(prefix: string, circuit: CircuitDef) {
  const existing = new Set([...circuit.devices, ...circuit.wires].map(item => item.id));
  let id: string;
  do { id = `${prefix}${nextId++}`; } while (existing.has(id));
  return id;
}
function resetState() {
  simulationRequest++;
  clearPlaybackTimer();
  return {
    simResult: null, simRunning: false, validationMessages: [],
    playbackTime: -1, playbackMaxTime: 0, playbackActive: false, selectedPin: null,
  };
}
const initialCircuit = starterCircuit();

export interface AppState {
  devices: DeviceDef[];
  devicesLoaded: boolean;
  devicesLoading: boolean;
  devicesError: string | null;
  circuit: CircuitDef;
  selectedDeviceId: string | null;
  selectedWireId: string | null;
  selectedPin: WireEndpoint | null;
  simResult: SimResult | null;
  simRunning: boolean;
  validationMessages: ValidationMessage[];
  playbackTime: number;
  playbackMaxTime: number;
  playbackActive: boolean;
  waveformDeviceIds: string[];
  maxTicks: number;
  projectId: string | null;
  projectName: string;
  projectSession: number;
  savedCircuit: CircuitDef | null;
  past: CircuitDef[];
  future: CircuitDef[];
  loadDevices: () => Promise<void>;
  addDevice: (type: string, x: number, y: number) => void;
  moveDevice: (id: string, x: number, y: number, recordHistory?: boolean) => void;
  checkpoint: (before: CircuitDef) => void;
  undo: () => void;
  redo: () => void;
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
  newProject: () => void;
  replaceCircuit: (circuit: CircuitDef, name: string) => void;
  startPlayback: () => void;
  stopPlayback: () => void;
  seekPlayback: (time: number) => void;
  setMaxTicks: (value: number) => void;
  dismissValidation: () => void;
  toggleWaveformDevice: (deviceId: string) => void;
  getNodeValueAt: (deviceId: string, pinId: string, time: number) => string | null;
  saveProject: (name: string) => Promise<string>;
  loadProject: (id: string) => Promise<void>;
}

export const useStore = create<AppState>((set, get) => {
  function commit(circuit: CircuitDef) {
    set(state => ({
      ...resetState(), circuit,
      past: [...state.past.slice(-49), state.circuit], future: [],
      selectedDeviceId: circuit.devices.some(device => device.id === state.selectedDeviceId) ? state.selectedDeviceId : null,
      selectedWireId: circuit.wires.some(wire => wire.id === state.selectedWireId) ? state.selectedWireId : null,
      waveformDeviceIds: state.waveformDeviceIds.filter(id => circuit.devices.some(device => device.id === id)),
    }));
  }
  return {
    devices: [], devicesLoaded: false, devicesLoading: false, devicesError: null,
    circuit: initialCircuit, selectedDeviceId: null, selectedWireId: null, selectedPin: null,
    simResult: null, simRunning: false, validationMessages: [],
    playbackTime: -1, playbackMaxTime: 0, playbackActive: false, waveformDeviceIds: [],
    maxTicks: 100, projectId: null, projectName: '未命名电路', projectSession: 0, savedCircuit: initialCircuit, past: [], future: [],

    loadDevices: async () => {
      if (get().devicesLoading || get().devicesLoaded) return;
      set({ devicesLoading: true, devicesError: null });
      try {
        const devices = await api.listDevices();
        set({ devices, devicesLoaded: true });
      } catch (error) {
        set({ devicesError: error instanceof Error ? error.message : '器件库加载失败' });
      } finally {
        set({ devicesLoading: false });
      }
    },
    addDevice: (type, x, y) => {
      const { circuit, devices } = get();
      const device: DeviceInstance = { id: uid('d', circuit), type, x: snap(x), y: snap(y), params: defaultParams(devices.find(item => item.type === type)) };
      commit({ ...circuit, devices: [...circuit.devices, device] });
      set({ selectedDeviceId: device.id, selectedWireId: null });
    },
    moveDevice: (id, x, y, recordHistory = true) => {
      const { circuit } = get();
      const device = circuit.devices.find(item => item.id === id);
      if (!device || device.x === snap(x) && device.y === snap(y)) return;
      const next = { ...circuit, devices: circuit.devices.map(item => item.id === id ? { ...item, x: snap(x), y: snap(y) } : item) };
      if (recordHistory) get().checkpoint(circuit);
      set({ circuit: next });
    },
    checkpoint: before => set(state => ({ past: [...state.past.slice(-49), before], future: [] })),
    undo: () => {
      const { past, circuit, future } = get();
      if (!past.length) return;
      set({ ...resetState(), circuit: past[past.length - 1], past: past.slice(0, -1), future: [circuit, ...future], selectedDeviceId: null, selectedWireId: null });
    },
    redo: () => {
      const { past, circuit, future } = get();
      if (!future.length) return;
      set({ ...resetState(), circuit: future[0], past: [...past, circuit], future: future.slice(1), selectedDeviceId: null, selectedWireId: null });
    },
    removeDevice: id => {
      const { circuit } = get();
      if (FIXED_DEVICE_IDS.has(id) || !circuit.devices.some(device => device.id === id)) return;
      commit({ ...circuit, devices: circuit.devices.filter(device => device.id !== id), wires: circuit.wires.filter(wire => wire.from.device !== id && wire.to.device !== id) });
    },
    selectDevice: id => set({ selectedDeviceId: id, selectedWireId: null }),
    selectWire: id => set({ selectedWireId: id, selectedDeviceId: null }),
    deleteSelected: () => {
      const state = get();
      if (state.selectedDeviceId) state.removeDevice(state.selectedDeviceId);
      if (state.selectedWireId) state.removeWire(state.selectedWireId);
    },
    updateDeviceParam: (id, key, value) => {
      const { circuit } = get();
      const device = circuit.devices.find(item => item.id === id);
      if (!device || device.params[key] === value) return;
      commit({ ...circuit, devices: circuit.devices.map(item => item.id === id ? { ...item, params: { ...item.params, [key]: value } } : item) });
    },
    startWire: endpoint => set({ selectedPin: endpoint }),
    completeWire: endpoint => {
      const { selectedPin, circuit } = get();
      if (!selectedPin) return;
      const same = (a: WireEndpoint, b: WireEndpoint) => a.device === b.device && a.pin === b.pin;
      if (same(selectedPin, endpoint) || circuit.wires.some(wire => same(wire.from, selectedPin) && same(wire.to, endpoint) || same(wire.to, selectedPin) && same(wire.from, endpoint))) {
        set({ selectedPin: null });
        return;
      }
      commit({ ...circuit, wires: [...circuit.wires, { id: uid('w', circuit), from: selectedPin, to: endpoint }] });
    },
    removeWire: id => {
      const { circuit } = get();
      if (circuit.wires.some(wire => wire.id === id)) commit({ ...circuit, wires: circuit.wires.filter(wire => wire.id !== id) });
    },
    cancelWire: () => set({ selectedPin: null }),

    runSimulation: async () => {
      if (get().simRunning) return;
      const { circuit, devices, maxTicks } = get();
      const token = ++simulationRequest;
      clearPlaybackTimer();
      set({ simRunning: true, simResult: null, validationMessages: [], playbackActive: false, playbackTime: -1, playbackMaxTime: 0 });
      try {
        const validation = await api.validate(circuit);
        if (token !== simulationRequest) return;
        if (!validation.valid) {
          set({ simRunning: false, validationMessages: [...validation.errors, ...validation.warnings] });
          return;
        }
        const records = circuit.devices.flatMap(device => (devices.find(def => def.type === device.type)?.pins ?? []).map(pin => `${device.id}.${pin.id}`));
        // Adapt older saved clocks to the integer parameters consumed by pybind11.
        const normalized = { ...circuit, devices: circuit.devices.map(device => device.type === 'CLOCK' ? {
          ...device, params: { ...device.params, initial: Number(device.params.initial ?? 0), duty_pct: Number(device.params.duty_pct ?? Number(device.params.duty ?? 0.5) * 100) },
        } : device) };
        const result = await api.run(normalized, { max_ticks: maxTicks, record_all: !records.length, record: records, default_delay: 1 });
        if (token !== simulationRequest) return;
        const messages: ValidationMessage[] = [...validation.warnings];
        for (const warning of result.warnings) {
          if (warning === 'Max ticks reached' || validation.warnings.some(item => warning === `${item.message}: ${item.detail}`)) continue;
          messages.push({ severity: 'warning', message: warning, detail: '' });
        }
        if (result.status !== 'ok' || result.errors.length) {
          set({ simRunning: false, validationMessages: [...messages, ...((result.errors.length ? result.errors : ['仿真未完成']).map(message => ({ severity: 'error' as const, message, detail: '' })))] });
          return;
        }
        const maxTime = Math.max(result.ticks_elapsed, 0, ...result.waveform.signals.flatMap(signal => signal.values.map(point => point.t)));
        set({ simResult: result, simRunning: false, validationMessages: messages, playbackMaxTime: maxTime });
        get().startPlayback();
      } catch (error) {
        if (token !== simulationRequest) return;
        set({ simRunning: false, validationMessages: [{ severity: 'error', message: '仿真失败', detail: error instanceof Error ? error.message : String(error) }] });
      }
    },
    resetSimulation: () => set(resetState()),
    clearCanvas: () => {
      const { circuit } = get();
      commit({ ...circuit, devices: circuit.devices.filter(device => FIXED_DEVICE_IDS.has(device.id)), wires: [] });
      set({ selectedDeviceId: null, selectedWireId: null, waveformDeviceIds: [] });
    },
    newProject: () => {
      const circuit = starterCircuit();
      commit(circuit);
      set({ projectId: null, projectName: '未命名电路', projectSession: get().projectSession + 1, savedCircuit: circuit, selectedDeviceId: null, selectedWireId: null, waveformDeviceIds: [], past: [], future: [] });
    },
    replaceCircuit: (circuit, name) => {
      commit(circuit);
      set({ projectId: null, projectName: name, projectSession: get().projectSession + 1, savedCircuit: null, selectedDeviceId: null, selectedWireId: null, waveformDeviceIds: ['sum_gate', 'carry_gate'] });
    },
    startPlayback: () => {
      const { simResult, playbackTime, playbackMaxTime } = get();
      if (!simResult?.waveform.signals.length || playbackMaxTime === 0) return;
      clearPlaybackTimer();
      set({ playbackTime: playbackTime >= 0 && playbackTime < playbackMaxTime ? playbackTime : 0, playbackActive: true });
      playbackTimer = setInterval(() => {
        const state = get();
        if (!state.playbackActive) { clearPlaybackTimer(); return; }
        set({ playbackTime: state.playbackTime >= state.playbackMaxTime ? 0 : state.playbackTime + 1 });
      }, 200);
    },
    stopPlayback: () => { clearPlaybackTimer(); set({ playbackActive: false }); },
    seekPlayback: time => {
      clearPlaybackTimer();
      set({ playbackActive: false, playbackTime: Math.max(-1, Math.min(get().playbackMaxTime, Math.round(time))) });
    },
    setMaxTicks: value => set({ maxTicks: Math.max(1, Math.min(10000, Math.round(value) || 100)) }),
    dismissValidation: () => set({ validationMessages: [] }),
    toggleWaveformDevice: id => set(state => ({ waveformDeviceIds: state.waveformDeviceIds.includes(id) ? state.waveformDeviceIds.filter(item => item !== id) : [...state.waveformDeviceIds, id] })),
    getNodeValueAt: (deviceId, pinId, time) => nodeValue(get().circuit, get().simResult, deviceId, pinId, time),

    saveProject: async name => {
      const { circuit, projectId, projectSession } = get();
      const id = projectId ?? (await api.createProject(name)).id;
      // Remember a created ID so retrying a failed update does not create duplicates.
      if (!projectId && get().projectSession === projectSession) set({ projectId: id, projectName: name, savedCircuit: null });
      await api.updateProject(id, { name, circuit });
      if (get().projectSession === projectSession) {
        set({ projectId: id, projectName: name, savedCircuit: circuit });
      }
      return id;
    },
    loadProject: async id => {
      const project = await api.getProject(id);
      commit(project.circuit);
      set({ projectId: id, projectName: project.name, projectSession: get().projectSession + 1, savedCircuit: project.circuit, selectedDeviceId: null, selectedWireId: null, waveformDeviceIds: [], past: [], future: [] });
    },
  };
});
