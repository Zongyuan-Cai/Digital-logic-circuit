import { beforeEach, describe, expect, it } from 'vitest';
import { fireEvent, render, screen } from '@testing-library/react';
import { CircuitCanvas } from '../components/CircuitCanvas';
import { DevicePanel } from '../components/DevicePanel';
import { Oscilloscope } from '../components/Oscilloscope';
import { PropertyPanel } from '../components/PropertyPanel';
import { useStore } from '../store/useStore';
import type { DeviceDef } from '../types/circuit';

const deviceDefs: DeviceDef[] = [
  {
    type: 'VCC',
    name: 'VCC 高电平',
    category: 'io',
    family: 'power',
    description: '',
    aliases: [],
    pins: [{ id: 'OUT', name: 'OUT', direction: 'output', role: 'power', active: 'high' }],
    params: { delay: 1 },
  },
  {
    type: 'GND',
    name: 'GND 低电平',
    category: 'io',
    family: 'power',
    description: '',
    aliases: [],
    pins: [{ id: 'OUT', name: 'OUT', direction: 'output', role: 'ground', active: 'high' }],
    params: { delay: 1 },
  },
  {
    type: 'CLOCK',
    name: '时钟源',
    category: 'io',
    family: 'input',
    description: '',
    aliases: [],
    pins: [{ id: 'OUT', name: 'OUT', direction: 'output', role: 'clock', active: 'high' }],
    params: { delay: 1, period: 2, duty: 0.5, initial: '0' },
  },
  {
    type: 'SWITCH',
    name: '开关',
    category: 'io',
    family: 'input',
    description: '',
    aliases: [],
    pins: [{ id: 'OUT', name: 'OUT', direction: 'output', role: 'data', active: 'high' }],
    params: { delay: 1, value: 0 },
  },
  {
    type: 'LED',
    name: 'LED 指示灯',
    category: 'io',
    family: 'output',
    description: '',
    aliases: [],
    pins: [{ id: 'IN', name: 'IN', direction: 'input', role: 'data', active: 'high' }],
    params: { delay: 1 },
  },
];

describe('Canvas interactions', () => {
  beforeEach(() => {
    useStore.setState({
      devices: deviceDefs,
      devicesLoaded: true,
      circuit: {
        version: '1.0',
        devices: [
          { id: 'd_vcc', type: 'VCC', x: 120, y: 160, params: { delay: 1 } },
          { id: 'd_gnd', type: 'GND', x: 120, y: 240, params: { delay: 1 } },
          { id: 'd_clock', type: 'CLOCK', x: 120, y: 320, params: { delay: 1, period: 2, duty: 0.5, initial: '0' } },
        ],
        wires: [],
      },
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

  it('renders only VCC, GND, and CLOCK as starter devices', () => {
    render(<CircuitCanvas />);

    expect(screen.getByTestId('device-d_vcc')).toBeTruthy();
    expect(screen.getByTestId('device-d_gnd')).toBeTruthy();
    expect(screen.getByTestId('device-d_clock')).toBeTruthy();
    expect(useStore.getState().circuit.devices.map(d => d.type)).toEqual(['VCC', 'GND', 'CLOCK']);
  });

  it('selects a device on first click and deselects it on second click', () => {
    render(<CircuitCanvas />);
    const vcc = screen.getByTestId('device-d_vcc');

    fireEvent.click(vcc);
    expect(useStore.getState().selectedDeviceId).toBe('d_vcc');

    fireEvent.click(vcc);
    expect(useStore.getState().selectedDeviceId).toBeNull();
  });

  it('keeps switch and led available in the right input/output panel', () => {
    render(<DevicePanel side="io" />);

    expect(screen.getByText('开关')).toBeTruthy();
    expect(screen.getByText(/指示灯/)).toBeTruthy();
    expect(screen.queryByText('VCC 高电平')).toBeNull();
  });

  it('keeps input/output devices out of the left logic panel', () => {
    render(<DevicePanel side="logic" />);

    expect(screen.queryByText('开关')).toBeNull();
    expect(screen.queryByText('LED 指示灯')).toBeNull();
  });

  it('keeps clock waveform visible without manual oscilloscope selection', () => {
    useStore.setState({
      simResult: {
        status: 'ok',
        errors: [],
        warnings: [],
        final_nodes: { 'd_clock.OUT': '1' },
        waveform: {
          time_unit: 'tick',
          signals: [
            { id: 'd_clock.OUT', name: 'd_clock.OUT', values: [{ t: 0, v: '1' }, { t: 1, v: '0' }] },
          ],
        },
        ticks_elapsed: 1,
        events_processed: 1,
      },
      waveformDeviceIds: [],
    });

    render(<Oscilloscope />);

    expect(screen.getByText('d_clock.OUT')).toBeTruthy();
  });

  it('shows a selected LED waveform even when it shares the clock network signal', () => {
    useStore.setState({
      circuit: {
        version: '1.0',
        devices: [
          { id: 'd_clock', type: 'CLOCK', x: 120, y: 320, params: { delay: 1, period: 2, duty: 0.5, initial: '0' } },
          { id: 'd_led', type: 'LED', x: 280, y: 320, params: { delay: 1 } },
        ],
        wires: [
          { id: 'w1', from: { device: 'd_clock', pin: 'OUT' }, to: { device: 'd_led', pin: 'IN' } },
        ],
      },
      simResult: {
        status: 'ok',
        errors: [],
        warnings: [],
        final_nodes: { 'd_clock.OUT': '1', 'd_led.IN': '1' },
        waveform: {
          time_unit: 'tick',
          signals: [
            { id: '1', name: 'd_led.IN', values: [{ t: 0, v: '1' }, { t: 1, v: '0' }] },
          ],
        },
        ticks_elapsed: 1,
        events_processed: 1,
      },
      waveformDeviceIds: ['d_led'],
    });

    render(<Oscilloscope />);

    expect(screen.getByText('d_clock.OUT')).toBeTruthy();
    expect(screen.getByText('d_led.IN')).toBeTruthy();
  });

  it('shows clock oscilloscope status as fixed in the property panel', () => {
    useStore.setState({ selectedDeviceId: 'd_clock' });

    render(<PropertyPanel />);

    expect(screen.getByText('时钟源波形常驻显示')).toBeTruthy();
    expect(screen.queryByText('在示波器显示')).toBeNull();
    expect(screen.queryByText('隐藏该器件波形')).toBeNull();
  });
});
