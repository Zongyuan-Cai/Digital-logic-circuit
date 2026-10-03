import type { CircuitDef } from '../types/circuit';
import { starterCircuit } from './devices';

export function halfAdder(): CircuitDef {
  const circuit = starterCircuit();
  return {
    ...circuit,
    devices: [
      ...circuit.devices,
      { id: 'input_a', type: 'SWITCH', x: 320, y: 160, params: { value: 1, delay: 1 } },
      { id: 'input_b', type: 'SWITCH', x: 320, y: 360, params: { value: 0, delay: 1 } },
      { id: 'sum_gate', type: 'XOR', x: 580, y: 160, params: { delay: 1 } },
      { id: 'carry_gate', type: 'AND', x: 580, y: 360, params: { delay: 1 } },
      { id: 'sum_led', type: 'LED', x: 840, y: 160, params: { delay: 1 } },
      { id: 'carry_led', type: 'LED', x: 840, y: 360, params: { delay: 1 } },
    ],
    wires: [
      { id: 'example_w1', from: { device: 'input_a', pin: 'OUT' }, to: { device: 'sum_gate', pin: 'I0' } },
      { id: 'example_w2', from: { device: 'input_b', pin: 'OUT' }, to: { device: 'sum_gate', pin: 'I1' } },
      { id: 'example_w3', from: { device: 'input_a', pin: 'OUT' }, to: { device: 'carry_gate', pin: 'I0' } },
      { id: 'example_w4', from: { device: 'input_b', pin: 'OUT' }, to: { device: 'carry_gate', pin: 'I1' } },
      { id: 'example_w5', from: { device: 'sum_gate', pin: 'Y' }, to: { device: 'sum_led', pin: 'IN' } },
      { id: 'example_w6', from: { device: 'carry_gate', pin: 'Y' }, to: { device: 'carry_led', pin: 'IN' } },
    ],
  };
}
