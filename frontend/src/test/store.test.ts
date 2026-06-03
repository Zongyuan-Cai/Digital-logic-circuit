import { describe, it, expect, beforeEach } from 'vitest';
import { useStore } from '../store/useStore';

describe('Circuit Store', () => {
  beforeEach(() => {
    useStore.setState({
      circuit: { version: '1.0', devices: [], wires: [] },
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
    expect(circuit.devices[0].x).toBe(100); // snapped to grid
    expect(circuit.devices[0].y).toBe(200);
  });

  it('snaps device to grid', () => {
    useStore.getState().addDevice('AND', 113, 207);
    const dev = useStore.getState().circuit.devices[0];
    expect(dev.x).toBe(120);
    expect(dev.y).toBe(200);
  });

  it('removes device and its wires', () => {
    const store = useStore.getState();
    store.addDevice('AND', 100, 100);
    store.addDevice('NOT', 200, 100);
    const devId = useStore.getState().circuit.devices[0].id;
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

  it('removes a deleted device from oscilloscope selection', () => {
    const store = useStore.getState();
    store.addDevice('AND', 100, 100);
    const devId = useStore.getState().circuit.devices[0].id;
    store.toggleWaveformDevice(devId);
    expect(useStore.getState().waveformDeviceIds).toEqual([devId]);

    store.removeDevice(devId);
    expect(useStore.getState().waveformDeviceIds).toEqual([]);
  });

  it('moves device', () => {
    useStore.getState().addDevice('AND', 100, 100);
    const id = useStore.getState().circuit.devices[0].id;
    useStore.getState().moveDevice(id, 300, 400);
    const dev = useStore.getState().circuit.devices[0];
    expect(dev.x).toBe(300);
    expect(dev.y).toBe(400);
  });

  it('snaps device move to grid', () => {
    useStore.getState().addDevice('AND', 100, 100);
    const id = useStore.getState().circuit.devices[0].id;
    useStore.getState().moveDevice(id, 313, 407);
    const dev = useStore.getState().circuit.devices[0];
    expect(dev.x).toBe(320);
    expect(dev.y).toBe(400);
  });

  it('selects device and deselects wire', () => {
    useStore.getState().addDevice('AND', 100, 100);
    const id = useStore.getState().circuit.devices[0].id;
    useStore.getState().selectWire('w1');
    useStore.getState().selectDevice(id);
    expect(useStore.getState().selectedDeviceId).toBe(id);
    expect(useStore.getState().selectedWireId).toBeNull();
  });

  it('selects wire and deselects device', () => {
    useStore.getState().addDevice('AND', 100, 100);
    const id = useStore.getState().circuit.devices[0].id;
    useStore.getState().selectDevice(id);
    useStore.getState().selectWire('w1');
    expect(useStore.getState().selectedWireId).toBe('w1');
    expect(useStore.getState().selectedDeviceId).toBeNull();
  });

  it('toggles devices in the oscilloscope selection', () => {
    const store = useStore.getState();
    store.toggleWaveformDevice('d1');
    expect(useStore.getState().waveformDeviceIds).toEqual(['d1']);

    store.toggleWaveformDevice('d2');
    expect(useStore.getState().waveformDeviceIds).toEqual(['d1', 'd2']);

    store.toggleWaveformDevice('d1');
    expect(useStore.getState().waveformDeviceIds).toEqual(['d2']);
  });

  it('deleteSelected removes selected device', () => {
    useStore.getState().addDevice('AND', 100, 100);
    const id = useStore.getState().circuit.devices[0].id;
    useStore.getState().selectDevice(id);
    useStore.getState().deleteSelected();
    expect(useStore.getState().circuit.devices).toHaveLength(0);
    expect(useStore.getState().selectedDeviceId).toBeNull();
  });

  it('deleteSelected removes selected wire', () => {
    useStore.getState().addDevice('AND', 100, 100);
    useStore.getState().addDevice('NOT', 300, 100);
    const a = useStore.getState().circuit.devices[0].id;
    const b = useStore.getState().circuit.devices[1].id;
    useStore.setState(s => ({
      circuit: { ...s.circuit, wires: [
        { id: 'w1', from: { device: a, pin: 'Y' }, to: { device: b, pin: 'I' } },
      ]},
    }));
    useStore.getState().selectWire('w1');
    useStore.getState().deleteSelected();
    expect(useStore.getState().circuit.wires).toHaveLength(0);
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
    expect(selectedPin).toBeNull();
  });

  it('prevents duplicate wires', () => {
    useStore.getState().addDevice('AND', 100, 100);
    useStore.getState().addDevice('NOT', 300, 100);
    const andId = useStore.getState().circuit.devices[0].id;
    const notId = useStore.getState().circuit.devices[1].id;
    useStore.getState().startWire({ device: andId, pin: 'Y' });
    useStore.getState().completeWire({ device: notId, pin: 'I' });
    useStore.getState().startWire({ device: andId, pin: 'Y' });
    useStore.getState().completeWire({ device: notId, pin: 'I' });
    expect(useStore.getState().circuit.wires).toHaveLength(1);
  });

  it('allows feedback wires between different pins on the same device', () => {
    useStore.getState().addDevice('SYNC_D_FF', 100, 100);
    const id = useStore.getState().circuit.devices[0].id;
    useStore.getState().startWire({ device: id, pin: 'Q' });
    useStore.getState().completeWire({ device: id, pin: 'D' });

    const { circuit, selectedPin } = useStore.getState();
    expect(circuit.wires).toHaveLength(1);
    expect(circuit.wires[0].from).toEqual({ device: id, pin: 'Q' });
    expect(circuit.wires[0].to).toEqual({ device: id, pin: 'D' });
    expect(selectedPin).toBeNull();
  });

  it('still cancels when completing a wire on the same exact pin', () => {
    useStore.getState().addDevice('SYNC_D_FF', 100, 100);
    const id = useStore.getState().circuit.devices[0].id;
    useStore.getState().startWire({ device: id, pin: 'Q' });
    useStore.getState().completeWire({ device: id, pin: 'Q' });

    expect(useStore.getState().circuit.wires).toHaveLength(0);
    expect(useStore.getState().selectedPin).toBeNull();
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

  it('reads playback values from the connected LED input waveform', () => {
    useStore.setState({
      circuit: {
        version: '1.0',
        devices: [
          { id: 'clk', type: 'CLOCK', x: 100, y: 100, params: { period: 2 } },
          { id: 'led', type: 'LED', x: 300, y: 100, params: {} },
        ],
        wires: [
          { id: 'w1', from: { device: 'clk', pin: 'OUT' }, to: { device: 'led', pin: 'IN' } },
        ],
      },
      simResult: {
        status: 'ok',
        errors: [],
        warnings: [],
        final_nodes: { 'clk.OUT': '1', 'led.IN': '1' },
        waveform: {
          time_unit: 'tick',
          signals: [
            { id: '0', name: 'OUT', values: [{ t: 0, v: 'Z' }] },
            { id: '1', name: 'IN', values: [{ t: 0, v: '1' }, { t: 1, v: '0' }, { t: 2, v: '1' }] },
          ],
        },
        ticks_elapsed: 2,
        events_processed: 2,
      },
    });

    expect(useStore.getState().getNodeValueAt('led', 'IN', 0)).toBe('1');
    expect(useStore.getState().getNodeValueAt('led', 'IN', 1)).toBe('0');
    expect(useStore.getState().getNodeValueAt('led', 'IN', 2)).toBe('1');
  });
});
