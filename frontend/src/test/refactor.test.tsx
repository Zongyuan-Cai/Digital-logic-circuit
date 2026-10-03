import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { act, cleanup, createEvent, fireEvent, render, screen, waitFor } from '@testing-library/react';
import { useStore } from '../store/useStore';
import { api } from '../api/client';
import { DevicePanel } from '../components/DevicePanel';
import { CircuitCanvas } from '../components/CircuitCanvas';
import { PropertyPanel } from '../components/PropertyPanel';
import { Oscilloscope } from '../components/Oscilloscope';
import { ProjectDialog } from '../components/ProjectDialog';
import { displayName, starterCircuit } from '../lib/devices';
import { halfAdder } from '../lib/examples';
import type { DeviceDef, SimResult } from '../types/circuit';

function definition(type: string, category: DeviceDef['category'], pins: string[], params = {}, sub_category = ''): DeviceDef {
  return { type, name: type === 'SWITCH' ? '开关' : type, category, family: '', description: '', aliases: [], sub_category, params, pins: pins.map(id => ({ id, name: id, direction: id === 'OUT' || id === 'Y' ? 'output' : 'input', role: 'data', active: 'high' })) };
}
const definitions = [
  definition('VCC', 'io', ['OUT']), definition('GND', 'io', ['OUT']),
  definition('CLOCK', 'io', ['OUT'], { period: 8, initial: '0', duty: 0.25, delay: 1 }),
  definition('SWITCH', 'io', ['OUT'], { value: 0, delay: 1 }), definition('LED', 'io', ['IN']),
  definition('AND', 'gate', ['I0', 'I1', 'Y'], { delay: 3 }), definition('XOR', 'gate', ['I0', 'I1', 'Y']),
  definition('74LS138', 'chip', ['A', 'Y'], {}, 'combinational'), definition('74161', 'chip', ['A', 'Y'], {}, 'sequential'),
];
const result: SimResult = { status: 'ok', errors: [], warnings: [], final_nodes: { 'd_clock.OUT': '1' }, ticks_elapsed: 2, events_processed: 4, waveform: { time_unit: 'tick', signals: [{ id: 'd_clock.OUT', name: 'd_clock.OUT', values: [{ t: 0, v: '1' }, { t: 1, v: '0' }, { t: 2, v: '1' }] }] } };

beforeEach(() => {
  useStore.getState().resetSimulation();
  const circuit = starterCircuit();
  useStore.setState({ ...useStore.getInitialState(), devices: definitions, devicesLoaded: true, circuit, savedCircuit: circuit, past: [], future: [] });
});
afterEach(() => { cleanup(); useStore.getState().stopPlayback(); vi.restoreAllMocks(); });

describe('Editor history and defaults', () => {
  it('copies each device definition’s parameters into independent instances', () => {
    const state = useStore.getState(); state.addDevice('SWITCH', 300, 200); state.addDevice('AND', 500, 200);
    expect(useStore.getState().circuit.devices.at(-2)?.params.value).toBe(0);
    expect(useStore.getState().circuit.devices.at(-1)?.params.delay).toBe(3);
    const id = useStore.getState().circuit.devices.at(-2)!.id;
    state.updateDeviceParam(id, 'value', 1);
    expect(definitions.find(def => def.type === 'SWITCH')!.params.value).toBe(0);
  });
  it('undoes and redoes device removal with all attached wires', () => {
    const state = useStore.getState(); state.addDevice('LED', 400, 200);
    const id = useStore.getState().circuit.devices.at(-1)!.id;
    state.startWire({ device: 'd_vcc', pin: 'OUT' }); state.completeWire({ device: id, pin: 'IN' });
    state.removeDevice(id); expect(useStore.getState().circuit.wires).toHaveLength(0);
    state.undo(); expect(useStore.getState().circuit.wires).toHaveLength(1);
    expect(useStore.getState().circuit.devices.some(device => device.id === id)).toBe(true);
    state.redo(); expect(useStore.getState().circuit.wires).toHaveLength(0);
  });
  it('groups a drag into one history entry and clears redo after a new edit', () => {
    const state = useStore.getState(), before = state.circuit;
    state.moveDevice('d_vcc', 180, 100, false); state.moveDevice('d_vcc', 220, 160, false); state.checkpoint(before);
    expect(useStore.getState().past).toHaveLength(1);
    state.undo(); expect(useStore.getState().circuit).toBe(before);
    state.addDevice('AND', 400, 200); expect(useStore.getState().future).toHaveLength(0);
  });
  it('protects all fixed sources and allows undoing clear canvas', () => {
    const state = useStore.getState(); state.removeDevice('d_vcc'); expect(useStore.getState().circuit.devices).toHaveLength(3);
    state.addDevice('AND', 400, 200); state.clearCanvas(); expect(useStore.getState().circuit.devices).toHaveLength(3);
    state.undo(); expect(useStore.getState().circuit.devices).toHaveLength(4);
  });
  it('does not reuse imported device IDs', () => {
    const circuit = starterCircuit();
    for (let i = 4; i < 200; i++) circuit.devices.push({ id: `d${i}`, type: 'LED', x: 400, y: 200, params: {} });
    useStore.setState({ circuit }); useStore.getState().addDevice('AND', 500, 200);
    const ids = useStore.getState().circuit.devices.map(device => device.id);
    expect(new Set(ids).size).toBe(ids.length);
  });
  it('preserves numeric and English information in chip names', () => {
    expect(displayName({ type: '74LS138', name: '74LS138 3-8线译码器' })).toBe('3-8线译码器');
    expect(displayName({ type: '74LS147', name: 'BCD优先编码器' })).toBe('BCD优先编码器');
  });
  it('uses actual library pin names in the half-adder example', () => {
    const circuit = halfAdder();
    for (const wire of circuit.wires) for (const endpoint of [wire.from, wire.to]) {
      const device = circuit.devices.find(item => item.id === endpoint.device)!;
      expect(definitions.find(def => def.type === device.type)?.pins.some(pin => pin.id === endpoint.pin)).toBe(true);
    }
  });
});

describe('Simulation request lifecycle', () => {
  it('adapts legacy clock parameters and avoids duplicated warnings', async () => {
    const circuit = starterCircuit(); circuit.devices[2].params = { period: 8, initial: '1', duty: 0.25 };
    useStore.setState({ circuit });
    const warning = { severity: 'warning' as const, message: 'Floating input', detail: 'A' };
    vi.spyOn(api, 'validate').mockResolvedValue({ valid: true, errors: [], warnings: [warning] });
    const run = vi.spyOn(api, 'run').mockResolvedValue({ ...result, warnings: ['Floating input: A', 'Max ticks reached'] });
    await useStore.getState().runSimulation();
    expect(run.mock.calls[0][0].devices[2].params).toMatchObject({ initial: 1, duty_pct: 25 });
    expect(useStore.getState().validationMessages).toEqual([warning]);
    expect(useStore.getState().playbackMaxTime).toBe(2);
  });
  it('displays engine errors instead of marking a failed calculation complete', async () => {
    vi.spyOn(api, 'validate').mockResolvedValue({ valid: true, errors: [], warnings: [] });
    vi.spyOn(api, 'run').mockResolvedValue({ ...result, status: 'error', errors: ['Unknown pin'] });
    await useStore.getState().runSimulation();
    expect(useStore.getState().simResult).toBeNull();
    expect(useStore.getState().validationMessages[0].message).toBe('Unknown pin');
    expect(useStore.getState().playbackActive).toBe(false);
  });
  it('ignores a stale result when the circuit is edited during calculation', async () => {
    vi.spyOn(api, 'validate').mockResolvedValue({ valid: true, errors: [], warnings: [] });
    let resolve!: (value: SimResult) => void;
    const run = vi.spyOn(api, 'run').mockReturnValue(new Promise<SimResult>(res => { resolve = res; }));
    const request = useStore.getState().runSimulation();
    await Promise.resolve(); expect(run).toHaveBeenCalled();
    useStore.getState().addDevice('AND', 400, 200); resolve(result); await request;
    expect(useStore.getState().simResult).toBeNull(); expect(useStore.getState().simRunning).toBe(false);
  });
  it('ignores validation that finishes after a reset', async () => {
    let resolve!: (value: Awaited<ReturnType<typeof api.validate>>) => void;
    vi.spyOn(api, 'validate').mockReturnValue(new Promise(res => { resolve = res; }));
    const run = vi.spyOn(api, 'run');
    const request = useStore.getState().runSimulation(); useStore.getState().resetSimulation();
    resolve({ valid: true, errors: [], warnings: [] }); await request;
    expect(run).not.toHaveBeenCalled(); expect(useStore.getState().simRunning).toBe(false);
  });
  it('pauses and seeks without losing the sampled playback time', () => {
    useStore.setState({ simResult: result, playbackMaxTime: 2 });
    const state = useStore.getState(); state.startPlayback(); state.seekPlayback(1); state.stopPlayback();
    expect(useStore.getState().playbackTime).toBe(1); expect(useStore.getState().getNodeValueAt('d_clock', 'OUT', 1)).toBe('0');
    state.seekPlayback(999); expect(useStore.getState().playbackTime).toBe(2);
  });
  it('shows recoverable device-library loading errors', async () => {
    useStore.setState({ devicesLoaded: false }); vi.spyOn(api, 'listDevices').mockRejectedValueOnce(new Error('offline')).mockResolvedValueOnce(definitions);
    await useStore.getState().loadDevices(); expect(useStore.getState().devicesError).toBe('offline');
    await useStore.getState().loadDevices(); expect(useStore.getState().devicesLoaded).toBe(true); expect(useStore.getState().devicesError).toBeNull();
  });
});

describe('Project persistence', () => {
  it('reuses a newly created ID when retrying a failed circuit save', async () => {
    const meta = { id: 'created', name: '实验', description: '', created_at: '', updated_at: '' };
    const create = vi.spyOn(api, 'createProject').mockResolvedValue(meta);
    const update = vi.spyOn(api, 'updateProject').mockRejectedValueOnce(new Error('offline')).mockResolvedValueOnce(meta);
    await expect(useStore.getState().saveProject('实验')).rejects.toThrow('offline');
    expect(useStore.getState().savedCircuit).toBeNull();
    await useStore.getState().saveProject('实验');
    expect(create).toHaveBeenCalledTimes(1); expect(update).toHaveBeenCalledTimes(2);
    expect(useStore.getState().projectId).toBe('created');
  });
  it('updates an existing project instead of creating duplicates', async () => {
    useStore.setState({ projectId: 'existing' });
    const create = vi.spyOn(api, 'createProject');
    const update = vi.spyOn(api, 'updateProject').mockResolvedValue({ id: 'existing', name: '实验', description: '', created_at: '', updated_at: '' });
    await useStore.getState().saveProject('实验');
    expect(create).not.toHaveBeenCalled(); expect(update).toHaveBeenCalledWith('existing', { name: '实验', circuit: useStore.getState().circuit });
    expect(useStore.getState().savedCircuit).toBe(useStore.getState().circuit);
  });
  it('keeps edits made during saving marked as unsaved', async () => {
    useStore.setState({ projectId: 'existing' });
    let resolve!: (value: Awaited<ReturnType<typeof api.updateProject>>) => void;
    vi.spyOn(api, 'updateProject').mockReturnValue(new Promise(res => { resolve = res; }));
    const before = useStore.getState().circuit, request = useStore.getState().saveProject('实验');
    useStore.getState().addDevice('AND', 400, 200); resolve({ id: 'existing', name: '实验', description: '', created_at: '', updated_at: '' }); await request;
    expect(useStore.getState().savedCircuit).toBe(before); expect(useStore.getState().circuit).not.toBe(before);
  });
  it('clears playback, selection, and history when opening another project', async () => {
    useStore.setState({ simResult: result, playbackActive: true, playbackTime: 1, selectedDeviceId: 'd_clock', selectedPin: { device: 'd_clock', pin: 'OUT' }, past: [starterCircuit()] });
    vi.spyOn(api, 'getProject').mockResolvedValue({ id: 'loaded', name: '已保存实验', description: '', created_at: '', updated_at: '', circuit: halfAdder() });
    await useStore.getState().loadProject('loaded');
    expect(useStore.getState()).toMatchObject({ projectId: 'loaded', simResult: null, playbackActive: false, playbackTime: -1, selectedDeviceId: null, selectedPin: null, past: [], future: [] });
  });
  it('keeps a failed save dialog open and exposes its error', async () => {
    vi.spyOn(api, 'createProject').mockRejectedValue(new Error('保存服务离线'));
    const onClose = vi.fn(); render(<ProjectDialog mode="save" onClose={onClose} onSaved={vi.fn()} />);
    fireEvent.click(screen.getByRole('button', { name: '保存工程' }));
    expect(await screen.findByRole('alert')).toHaveTextContent('保存服务离线'); expect(onClose).not.toHaveBeenCalled();
  });
});

describe('Workspace interactions', () => {
  it('selects and deselects devices after captured pointer gestures', () => {
    render(<CircuitCanvas />);
    const device = screen.getByTestId('device-d_vcc'), canvas = screen.getByLabelText('电路画布');
    fireEvent.pointerDown(device, { button: 0, clientX: 120, clientY: 140 });
    fireEvent.pointerUp(canvas, { button: 0, clientX: 120, clientY: 140 }); fireEvent.click(canvas);
    expect(useStore.getState().selectedDeviceId).toBe('d_vcc');
    fireEvent.pointerDown(device, { button: 0, clientX: 120, clientY: 140 });
    fireEvent.pointerUp(canvas, { button: 0, clientX: 120, clientY: 140 }); fireEvent.click(canvas);
    expect(useStore.getState().selectedDeviceId).toBeNull();
  });
  it('records a multi-move pointer drag as one undoable action', () => {
    render(<CircuitCanvas />);
    const device = screen.getByTestId('device-d_vcc'), canvas = screen.getByLabelText('电路画布');
    const before = useStore.getState().circuit;
    fireEvent.pointerDown(device, { button: 0, clientX: 120, clientY: 140 });
    fireEvent.pointerMove(canvas, { clientX: 160, clientY: 180 }); fireEvent.pointerMove(canvas, { clientX: 200, clientY: 220 });
    fireEvent.pointerUp(canvas, { clientX: 200, clientY: 220 }); fireEvent.click(canvas);
    expect(useStore.getState().past).toHaveLength(1); expect(useStore.getState().circuit.devices[0]).toMatchObject({ x: 200, y: 220 });
    act(() => { useStore.getState().undo(); }); expect(useStore.getState().circuit).toBe(before);
  });
  it('combines chip-category filtering with case-insensitive search', () => {
    render(<DevicePanel />); fireEvent.click(screen.getByRole('button', { name: '组合芯片' })); fireEvent.change(screen.getByLabelText('搜索器件'), { target: { value: '74ls138' } });
    expect(screen.getByRole('button', { name: /添加 74LS138/ })).toBeTruthy(); expect(screen.queryByRole('button', { name: /添加 74161/ })).toBeNull();
    fireEvent.change(screen.getByLabelText('搜索器件'), { target: { value: 'missing' } }); expect(screen.getByText('没有匹配的器件')).toBeTruthy();
  });
  it('adds devices by keyboard-accessible library buttons', () => {
    render(<DevicePanel />); fireEvent.click(screen.getByRole('button', { name: '添加 AND AND' }));
    expect(useStore.getState().circuit.devices.at(-1)?.type).toBe('AND');
  });
  it('converts drop coordinates through the current zoom transform', () => {
    vi.spyOn(SVGSVGElement.prototype, 'getBoundingClientRect').mockReturnValue({ x: 100, y: 50, left: 100, top: 50, right: 900, bottom: 450, width: 800, height: 400, toJSON: () => ({}) });
    render(<CircuitCanvas />); fireEvent.click(screen.getByRole('button', { name: '放大画布' }));
    const canvas = screen.getByLabelText('电路画布');
    const drop = createEvent.drop(canvas, { dataTransfer: { getData: () => 'AND' } });
    Object.defineProperties(drop, { clientX: { value: 220 }, clientY: { value: 170 } });
    fireEvent(canvas, drop);
    expect(useStore.getState().circuit.devices.at(-1)).toMatchObject({ type: 'AND', x: 160, y: 140 });
  });
  it('does not delete circuit devices while typing in form controls', () => {
    const state = useStore.getState(); state.addDevice('AND', 400, 200); const id = useStore.getState().selectedDeviceId;
    render(<><CircuitCanvas /><textarea aria-label="说明" /></>); fireEvent.keyDown(screen.getByLabelText('说明'), { key: 'Backspace' });
    expect(useStore.getState().circuit.devices.some(device => device.id === id)).toBe(true);
  });
  it('shows pin values at the playback cursor in the inspector', () => {
    useStore.setState({ selectedDeviceId: 'd_clock', simResult: result, playbackTime: 0, playbackMaxTime: 2 }); render(<PropertyPanel />);
    expect(screen.getByRole('cell', { name: '1' })).toBeTruthy();
    act(() => { useStore.getState().seekPlayback(1); }); expect(screen.getByRole('cell', { name: '0' })).toBeTruthy();
  });
  it('collapses the oscilloscope even before the first simulation', () => {
    render(<Oscilloscope />); fireEvent.click(screen.getByRole('button', { name: '收起示波器' }));
    expect(screen.queryByText('捕捉逻辑的每一次变化')).toBeNull(); fireEvent.click(screen.getByRole('button', { name: '展开示波器' })); expect(screen.getByText('捕捉逻辑的每一次变化')).toBeTruthy();
  });
  it('requires explicit confirmation before replacing an unsaved circuit', async () => {
    useStore.getState().addDevice('AND', 400, 200);
    vi.spyOn(api, 'listProjects').mockResolvedValue([{ id: 'saved', name: '另一实验', description: '', created_at: '', updated_at: '2026-10-03T00:00:00Z' }]);
    const open = vi.spyOn(api, 'getProject'); render(<ProjectDialog mode="open" onClose={vi.fn()} onSaved={vi.fn()} />);
    fireEvent.click(await screen.findByRole('button', { name: /另一实验/ }));
    await waitFor(() => expect(screen.getByRole('button', { name: '继续打开' })).toBeTruthy()); expect(open).not.toHaveBeenCalled();
  });
});
