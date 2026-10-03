import type { DeviceDef, DeviceInstance } from '../types/circuit';

export const FIXED_DEVICE_IDS = new Set(['d_vcc', 'd_gnd', 'd_clock']);
export const FIXED_DEVICE_TYPES = new Set(['VCC', 'GND', 'CLOCK']);

export function starterCircuit() {
  return {
    version: '1.0',
    devices: [
      { id: 'd_vcc', type: 'VCC', x: 120, y: 140, params: { delay: 1 } },
      { id: 'd_gnd', type: 'GND', x: 120, y: 240, params: { delay: 1 } },
      { id: 'd_clock', type: 'CLOCK', x: 120, y: 340, params: { delay: 1, period: 2, duty_pct: 50, initial: 0 } },
    ] as DeviceInstance[],
    wires: [],
  };
}

export function displayName(device: Pick<DeviceDef, 'name' | 'type'>): string {
  const name = device.name.startsWith(`${device.type} `) ? device.name.slice(device.type.length).trim() : device.name;
  return name || device.type;
}

export function matchesSearch(device: DeviceDef, search: string): boolean {
  const query = search.trim().toLocaleLowerCase();
  return !query || [device.type, device.name, device.description, ...device.aliases]
    .some(value => value.toLocaleLowerCase().includes(query));
}

export function defaultParams(device?: DeviceDef): Record<string, unknown> {
  const params: Record<string, unknown> = { delay: 1, ...device?.params };
  if (device?.type === 'CLOCK') {
    params.initial = Number(params.initial ?? 0);
    params.duty_pct = Number(params.duty_pct ?? Number(params.duty ?? 0.5) * 100);
    delete params.duty;
  }
  return params;
}
