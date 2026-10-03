import type { DeviceDef, DeviceInstance, PinDef } from '../types/circuit';

export const DEVICE_WIDTH = 144;
export const PIN_SPACING = 22;
export const snap = (value: number) => Math.round(value / 20) * 20;

export function deviceHeight(def?: DeviceDef) {
  const inputs = def?.pins.filter(pin => pin.direction === 'input').length ?? 0;
  const outputs = (def?.pins.length ?? 0) - inputs;
  return Math.max(84, Math.max(inputs, outputs) * PIN_SPACING + 48);
}

export function pinPosition(device: DeviceInstance, def: DeviceDef, pin: PinDef) {
  const input = pin.direction === 'input';
  const group = def.pins.filter(item => (item.direction === 'input') === input);
  return {
    x: device.x + (input ? -DEVICE_WIDTH / 2 : DEVICE_WIDTH / 2),
    y: device.y - deviceHeight(def) / 2 + 42 + group.findIndex(item => item.id === pin.id) * PIN_SPACING,
  };
}

export function wirePath(from: { x: number; y: number }, to: { x: number; y: number }, offset = 0, loopY?: number, fromDirection = 1, toDirection = -1) {
  if (loopY !== undefined || to.x < from.x + 36 || fromDirection !== 1 || toDirection !== -1) {
    const y = loopY ?? Math.min(from.y, to.y) - 50 - Math.abs(offset);
    return `M ${from.x} ${from.y} H ${from.x + fromDirection * (28 + Math.abs(offset))} V ${y} H ${to.x + toDirection * (28 + Math.abs(offset))} V ${to.y} H ${to.x}`;
  }
  const middle = (from.x + to.x) / 2 + offset;
  return `M ${from.x} ${from.y} H ${middle} V ${to.y} H ${to.x}`;
}
