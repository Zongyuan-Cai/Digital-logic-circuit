/* Zustand state management for the circuit editor */

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
  // Device library
  devices: DeviceDef[];
  devicesLoaded: boolean;

  // Circuit
  circuit: CircuitDef;
  selectedDeviceId: string | null;
  selectedPin: WireEndpoint | null; // for wiring: first click

  // Simulation
  simResult: SimResult | null;
  simRunning: boolean;
  validationMessages: ValidationMessage[];

  // Actions
  loadDevices: () => Promise<void>;
  addDevice: (type: string, x: number, y: number) => void;
  moveDevice: (id: string, x: number, y: number) => void;
  removeDevice: (id: string) => void;
  selectDevice: (id: string | null) => void;
  startWire: (endpoint: WireEndpoint) => void;
  completeWire: (endpoint: WireEndpoint) => void;
  removeWire: (id: string) => void;
  cancelWire: () => void;
  runSimulation: () => Promise<void>;
  resetSimulation: () => void;

  // Persistence
  saveProject: (name: string) => Promise<string>;
  loadProject: (id: string) => Promise<void>;
}

export const useStore = create<AppState>((set, get) => ({
  devices: [],
  devicesLoaded: false,
  circuit: { version: '1.0', devices: [], wires: [] },
  selectedDeviceId: null,
  selectedPin: null,
  simResult: null,
  simRunning: false,
  validationMessages: [],

  loadDevices: async () => {
    const devices = await api.listDevices();
    set({ devices, devicesLoaded: true });
  },

  addDevice: (type, x, y) => {
    const dev: DeviceInstance = { id: uid(), type, x, y, params: { delay: 1 } };
    set(s => ({
      circuit: {
        ...s.circuit,
        devices: [...s.circuit.devices, dev],
      },
      simResult: null,
    }));
  },

  moveDevice: (id, x, y) => {
    set(s => ({
      circuit: {
        ...s.circuit,
        devices: s.circuit.devices.map(d =>
          d.id === id ? { ...d, x, y } : d
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
    set({ selectedDeviceId: id });
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
    // Avoid duplicate wires
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
      circuit: {
        ...s.circuit,
        wires: s.circuit.wires.filter(w => w.id !== id),
      },
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
      set({ simResult: result, simRunning: false, validationMessages: [] });
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
    // Create project first
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
