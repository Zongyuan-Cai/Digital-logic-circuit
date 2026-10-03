import { beforeEach, describe, it, expect } from 'vitest';
import { act, render, screen } from '@testing-library/react';
import App from '../App';
import { useStore } from '../store/useStore';

describe('App', () => {
  beforeEach(() => {
    useStore.setState({
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
      devicesLoaded: true,
      devicesLoading: false,
      past: [], future: [],
      validationMessages: [],
    });
  });

  it('renders toolbar with title', async () => {
    await act(async () => { render(<App />); });
    expect(screen.getByText(/数字逻辑电路仿真/)).toBeTruthy();
  });

  it('renders device panel', async () => {
    await act(async () => { render(<App />); });
    expect(screen.getByRole('heading', { name: '器件库' })).toBeTruthy();
  });

  it('renders property panel placeholder', async () => {
    await act(async () => { render(<App />); });
    expect(screen.getByText('属性面板')).toBeTruthy();
  });

  it('renders oscilloscope', async () => {
    await act(async () => { render(<App />); });
    expect(screen.getByRole('heading', { name: '示波器' })).toBeTruthy();
  });

  it('renders at least one run-related button', async () => {
    await act(async () => { render(<App />); });
    const elements = screen.getAllByText(/运行/);
    expect(elements.length).toBeGreaterThanOrEqual(1);
  });

  it('renders save/new button', async () => {
    await act(async () => { render(<App />); });
    expect(screen.getByRole('button', { name: '保存工程' })).toBeTruthy();
  });

  it('starts with only the three fixed source devices on the canvas', async () => {
    await act(async () => { render(<App />); });
    const fixedTypes = useStore.getState().circuit.devices.map(d => d.type);
    expect(fixedTypes).toEqual(['VCC', 'GND', 'CLOCK']);
  });
});
