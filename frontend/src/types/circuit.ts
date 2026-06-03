/* TypeScript types matching backend Pydantic schemas */

export type SignalValue = '0' | '1' | 'X' | 'Z';
export type PinDirection = 'input' | 'output' | 'bidirectional';
export type ActiveLevel = 'high' | 'low';
export type DeviceCategory = 'gate' | 'flip_flop' | 'chip' | 'io';

export interface PinDef {
  id: string;
  name: string;
  direction: PinDirection;
  role: string;
  active: ActiveLevel;
}

export interface DeviceDef {
  type: string;
  name: string;
  category: DeviceCategory;
  sub_category?: string;
  family: string;
  description: string;
  aliases: string[];
  pins: PinDef[];
  params: Record<string, unknown>;
}

export interface DeviceInstance {
  id: string;
  type: string;
  x: number;
  y: number;
  params: Record<string, unknown>;
}

export interface WireEndpoint {
  device: string;
  pin: string;
}

export interface WireDef {
  id: string;
  from: WireEndpoint;
  to: WireEndpoint;
}

export interface CircuitDef {
  version: string;
  devices: DeviceInstance[];
  wires: WireDef[];
}

export interface SimOptions {
  max_ticks: number;
  max_events: number;
  record_all: boolean;
  record: string[];
  default_delay: number;
}

export interface WavePoint {
  t: number;
  v: string;
}

export interface WaveSignal {
  id: string;
  name: string;
  values: WavePoint[];
}

export interface WaveformData {
  time_unit: string;
  signals: WaveSignal[];
}

export interface SimResult {
  status: string;
  errors: string[];
  warnings: string[];
  final_nodes: Record<string, string>;
  waveform: WaveformData;
  ticks_elapsed: number;
  events_processed: number;
}

export interface ValidationMessage {
  severity: 'warning' | 'error';
  message: string;
  detail: string;
}

export interface ValidationResult {
  valid: boolean;
  errors: ValidationMessage[];
  warnings: ValidationMessage[];
}

export interface ProjectMeta {
  id: string;
  name: string;
  description: string;
  created_at: string;
  updated_at: string;
}

export interface ProjectData extends ProjectMeta {
  circuit: CircuitDef;
}
