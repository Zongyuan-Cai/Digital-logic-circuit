import { create } from 'zustand';
import type {
  DeviceDef, DeviceInstance, WireDef, WireEndpoint, CircuitDef,
  SimResult, ValidationMessage
} from '../types/circuit';
import { api } from '../api/client';

let _nextId = 1;
function uid(): string { return `d${_nextId++}`; }
function wid(): string { return `w${_nextId++}`; }
function syncNextId(circuit: CircuitDef): void {
  let max = 0;
  for (const item of [...circuit.devices, ...circuit.wires]) {
    const match = /^[dw](\d+)$/.exec(item.id);
    if (match) max = Math.max(max, Number(match[1]));
  }
  _nextId = Math.max(_nextId, max + 1);
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

  saveProject: (name: string) => Promise<string>;
  loadProject: (id: string) => Promise<void>;
}

export const useStore = create<AppState>((set, get) => ({
  devices: [],
  devicesLoaded: false,
  circuit: { version: '1.0', devices: [], wires: [] },
  selectedDeviceId: null,
  selectedWireId: null,
  selectedPin: null,
  simResult: null,
  simRunning: false,
  validationMessages: [],

  loadDevices: async () => {
    const devices = await api.listDevices();
    set({ devices, devicesLoaded: true });
  },

  addDevice: (type, x, y) => {
    const rx = Math.round(x / 20) * 20;
    const ry = Math.round(y / 20) * 20;
    const dev: DeviceInstance = { id: uid(), type, x: rx, y: ry, params: { delay: 1 } };
    set(s => ({
      circuit: { ...s.circuit, devices: [...s.circuit.devices, dev] },
      simResult: null,
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
    set(s => ({
      circuit: {
        ...s.circuit,
        devices: s.circuit.devices.filter(d => d.id !== id),
        wires: s.circuit.wires.filter(
          w => w.from.device !== id && w.to.device !== id
        ),
      },
      selectedDeviceId: s.selectedDeviceId === id ? null : s.selectedDeviceId,
      simResult: null,
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
    set(s => ({
      circuit: { ...s.circuit, wires: [...s.circuit.wires, wire] },
      selectedPin: null,
      simResult: null,
    }));
  },

  removeWire: (id) => {
    set(s => ({
      circuit: { ...s.circuit, wires: s.circuit.wires.filter(w => w.id !== id) },
      selectedWireId: s.selectedWireId === id ? null : s.selectedWireId,
      simResult: null,
    }));
  },

  cancelWire: () => {
    set({ selectedPin: null });
  },

  runSimulation: async () => {
    const { circuit } = get();
    set({ simRunning: true, simResult: null, validationMessages: [] });
    try {
      const validation = await api.validate(circuit);
      if (!validation.valid) {
        set({ simRunning: false, validationMessages: validation.errors });
        return;
      }
      const result = await api.run(circuit, {
        max_ticks: 100,
        record_all: true,
        default_delay: 1,
      });
      // Merge validation warnings into messages for display
      const simWarnings: ValidationMessage[] = [];
      for (const w of validation.warnings) {
        simWarnings.push({ severity: 'warning', message: w.message, detail: w.detail });
      }
      for (const w of result.warnings) {
        simWarnings.push({ severity: 'warning', message: w, detail: '' });
      }
      set({ simResult: result, simRunning: false, validationMessages: simWarnings });
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
    set({ simResult: null, validationMessages: [] });
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
