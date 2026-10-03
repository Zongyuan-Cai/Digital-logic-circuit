import type { CircuitDef, SimResult, WaveSignal } from '../types/circuit';

export const SIGNAL_COLORS: Record<string, string> = {
  '1': '#6fe2be', '0': '#7f8c9f', X: '#f08089', Z: '#85b5ff',
};

export function connectedNetwork(circuit: CircuitDef, deviceId: string, pinId: string) {
  const keys = new Set([`${deviceId}.${pinId}`]);
  const pinNames = new Set([pinId]);
  const queue = [{ device: deviceId, pin: pinId }];
  for (let index = 0; index < queue.length; index++) {
    const current = queue[index];
    for (const wire of circuit.wires) {
      const next = wire.from.device === current.device && wire.from.pin === current.pin ? wire.to
        : wire.to.device === current.device && wire.to.pin === current.pin ? wire.from : null;
      if (!next || keys.has(`${next.device}.${next.pin}`)) continue;
      keys.add(`${next.device}.${next.pin}`);
      pinNames.add(next.pin);
      queue.push(next);
    }
  }
  return { keys, pinNames };
}

export function findNetworkSignal(circuit: CircuitDef, signals: WaveSignal[], deviceId: string, pinId: string) {
  // Exact endpoint names take priority; short names support older record_all responses.
  const exact = signals.find(signal => signal.id === `${deviceId}.${pinId}` || signal.name === `${deviceId}.${pinId}`);
  if (exact) return exact;
  const network = connectedNetwork(circuit, deviceId, pinId);
  return signals.find(signal => network.keys.has(signal.id) || network.keys.has(signal.name))
    ?? signals.find(signal => (signal.name === pinId || signal.id === pinId) && signal.values.some(point => point.v !== 'Z'))
    ?? signals.find(signal => network.pinNames.has(signal.name) && signal.values.some(point => point.v !== 'Z'))
    ?? signals.find(signal => network.pinNames.has(signal.name) || network.pinNames.has(signal.id));
}

export function valueAt(signal: WaveSignal, time: number): string | null {
  let value: string | null = null;
  for (const point of signal.values) {
    if (point.t > time) break;
    value = point.v;
  }
  return value;
}

export function nodeValue(circuit: CircuitDef, result: SimResult | null, deviceId: string, pinId: string, time: number) {
  if (!result) return null;
  if (time < 0) return result.final_nodes[`${deviceId}.${pinId}`] ?? null;
  const signal = findNetworkSignal(circuit, result.waveform.signals, deviceId, pinId);
  return signal ? valueAt(signal, time) : result.final_nodes[`${deviceId}.${pinId}`] ?? null;
}
