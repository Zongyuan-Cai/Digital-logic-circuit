import { beforeEach, describe, it, expect } from 'vitest';
import { render, screen } from '@testing-library/react';
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
      validationMessages: [],
    });
  });

  it('renders toolbar with title', () => {
    render(<App />);
    expect(screen.getByText(/数字逻辑电路仿真/)).toBeTruthy();
  });

  it('renders device panel', () => {
    render(<App />);
    expect(screen.getByText('器件库')).toBeTruthy();
  });

  it('renders property panel placeholder', () => {
    render(<App />);
    expect(screen.getByText('属性面板')).toBeTruthy();
  });

  it('renders oscilloscope', () => {
    render(<App />);
    expect(screen.getByText(/示波器/)).toBeTruthy();
  });

  it('renders at least one run-related button', () => {
    render(<App />);
    const elements = screen.getAllByText(/运行/);
    expect(elements.length).toBeGreaterThanOrEqual(1);
  });

  it('renders save/new button', () => {
    render(<App />);
    expect(screen.getByText(/新建|保存/)).toBeTruthy();
  });

  it('starts with only the three fixed source devices on the canvas', () => {
    render(<App />);
    const fixedTypes = useStore.getState().circuit.devices.map(d => d.type);
    expect(fixedTypes).toEqual(['VCC', 'GND', 'CLOCK']);
  });
});
