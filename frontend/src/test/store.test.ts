import { describe, it, expect, beforeEach, vi } from 'vitest';
import { api } from '../api/client';
import { useStore } from '../store/useStore';

vi.mock('../api/client', () => ({
  api: {
    getProject: vi.fn(),
  },
}));

describe('Circuit Store', () => {
  beforeEach(() => {
    // Reset store state
    useStore.setState({
      circuit: { version: '1.0', devices: [], wires: [] },
      selectedDeviceId: null,
      selectedPin: null,
      simResult: null,
      simRunning: false,
      validationMessages: [],
    });
  });

  it('starts with empty circuit', () => {
    const { circuit } = useStore.getState();
    expect(circuit.devices).toHaveLength(0);
    expect(circuit.wires).toHaveLength(0);
  });

  it('adds device to circuit', () => {
    useStore.getState().addDevice('AND', 100, 200);
    const { circuit } = useStore.getState();
    expect(circuit.devices).toHaveLength(1);
    expect(circuit.devices[0].type).toBe('AND');
    expect(circuit.devices[0].x).toBe(100);
    expect(circuit.devices[0].y).toBe(200);
  });

  it('removes device and its wires', () => {
    const store = useStore.getState();
    store.addDevice('AND', 100, 100);
    store.addDevice('NOT', 200, 100);
    const devId = useStore.getState().circuit.devices[0].id;

    // Add a wire connected to this device
    const dev2Id = useStore.getState().circuit.devices[1].id;
    useStore.setState(s => ({
      circuit: {
        ...s.circuit,
        wires: [...s.circuit.wires, {
          id: 'w1', from: { device: devId, pin: 'Y' }, to: { device: dev2Id, pin: 'I' },
        }],
      },
    }));

    expect(useStore.getState().circuit.wires).toHaveLength(1);
    store.removeDevice(devId);
    expect(useStore.getState().circuit.devices).toHaveLength(1);
    expect(useStore.getState().circuit.wires).toHaveLength(0);
  });

  it('moves device', () => {
    useStore.getState().addDevice('AND', 100, 100);
    const id = useStore.getState().circuit.devices[0].id;
    useStore.getState().moveDevice(id, 300, 400);
    const dev = useStore.getState().circuit.devices[0];
    expect(dev.x).toBe(300);
    expect(dev.y).toBe(400);
  });

  it('selects device', () => {
    useStore.getState().addDevice('AND', 100, 100);
    const id = useStore.getState().circuit.devices[0].id;
    useStore.getState().selectDevice(id);
    expect(useStore.getState().selectedDeviceId).toBe(id);
  });

  it('deselects device', () => {
    useStore.getState().addDevice('AND', 100, 100);
    const id = useStore.getState().circuit.devices[0].id;
    useStore.getState().selectDevice(id);
    useStore.getState().selectDevice(null);
    expect(useStore.getState().selectedDeviceId).toBeNull();
  });

  it('adds wire between two pins', () => {
    useStore.getState().addDevice('AND', 100, 100);
    useStore.getState().addDevice('NOT', 300, 100);
    const andId = useStore.getState().circuit.devices[0].id;
    const notId = useStore.getState().circuit.devices[1].id;

    useStore.getState().startWire({ device: andId, pin: 'Y' });
    useStore.getState().completeWire({ device: notId, pin: 'I' });

    const { circuit, selectedPin } = useStore.getState();
    expect(circuit.wires).toHaveLength(1);
    expect(circuit.wires[0].from.device).toBe(andId);
    expect(circuit.wires[0].to.device).toBe(notId);
    expect(selectedPin).toBeNull();
  });

  it('prevents duplicate wires', () => {
    useStore.getState().addDevice('AND', 100, 100);
    useStore.getState().addDevice('NOT', 300, 100);
    const andId = useStore.getState().circuit.devices[0].id;
    const notId = useStore.getState().circuit.devices[1].id;

    useStore.getState().startWire({ device: andId, pin: 'Y' });
    useStore.getState().completeWire({ device: notId, pin: 'I' });
    // Try again
    useStore.getState().startWire({ device: andId, pin: 'Y' });
    useStore.getState().completeWire({ device: notId, pin: 'I' });

    expect(useStore.getState().circuit.wires).toHaveLength(1);
  });

  it('cancels wire selection', () => {
    useStore.getState().addDevice('AND', 100, 100);
    const id = useStore.getState().circuit.devices[0].id;
    useStore.getState().startWire({ device: id, pin: 'Y' });
    expect(useStore.getState().selectedPin).not.toBeNull();
    useStore.getState().cancelWire();
    expect(useStore.getState().selectedPin).toBeNull();
  });

  it('resets simulation', () => {
    useStore.setState({
      simResult: { status: 'ok', errors: [], warnings: [], final_nodes: {}, waveform: { time_unit: 'tick', signals: [] }, ticks_elapsed: 0, events_processed: 0 },
      validationMessages: [{ severity: 'error', message: 'test', detail: '' }],
    });
    useStore.getState().resetSimulation();
    expect(useStore.getState().simResult).toBeNull();
    expect(useStore.getState().validationMessages).toHaveLength(0);
  });

  it('does not reuse IDs after loading a project', async () => {
    vi.mocked(api.getProject).mockResolvedValue({
      id: 'p1',
      name: 'loaded',
      description: '',
      created_at: '',
      updated_at: '',
      circuit: {
        version: '1.0',
        devices: [
          { id: 'd20', type: 'AND', x: 100, y: 100, params: {} },
        ],
        wires: [
          { id: 'w21', from: { device: 'd20', pin: 'Y' }, to: { device: 'd22', pin: 'I' } },
        ],
      },
    });

    await useStore.getState().loadProject('p1');
    useStore.getState().addDevice('NOT', 200, 100);

    const ids = useStore.getState().circuit.devices.map(d => d.id);
    expect(ids).toEqual(['d20', 'd22']);
  });
});
