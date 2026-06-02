/* Backend API client */

const BASE = '/api';

async function request<T>(path: string, options?: RequestInit): Promise<T> {
  const res = await fetch(`${BASE}${path}`, {
    headers: { 'Content-Type': 'application/json' },
    ...options,
  });
  if (!res.ok) {
    const err = await res.json().catch(() => ({ detail: res.statusText }));
    throw new Error(err.detail?.message || err.detail || res.statusText);
  }
  if (res.status === 204) return undefined as T;
  return res.json();
}

export const api = {
  // Devices
  listDevices: (category?: string) =>
    request<import('../types/circuit').DeviceDef[]>(
      `/devices${category ? `?category=${category}` : ''}`
    ),
  getDevice: (type: string) =>
    request<import('../types/circuit').DeviceDef>(`/devices/${encodeURIComponent(type)}`),

  // Projects
  listProjects: () =>
    request<import('../types/circuit').ProjectMeta[]>('/projects'),
  getProject: (id: string) =>
    request<import('../types/circuit').ProjectData>(`/projects/${id}`),
  createProject: (name: string) =>
    request<import('../types/circuit').ProjectMeta>('/projects', {
      method: 'POST', body: JSON.stringify({ name, description: '' }),
    }),
  updateProject: (id: string, data: { name?: string; circuit?: import('../types/circuit').CircuitDef }) =>
    request<import('../types/circuit').ProjectMeta>(`/projects/${id}`, {
      method: 'PUT', body: JSON.stringify(data),
    }),
  deleteProject: (id: string) =>
    request<void>(`/projects/${id}`, { method: 'DELETE' }),

  // Simulation
  validate: (circuit: import('../types/circuit').CircuitDef) =>
    request<import('../types/circuit').ValidationResult>('/circuits/validate', {
      method: 'POST', body: JSON.stringify(circuit),
    }),
  run: (circuit: import('../types/circuit').CircuitDef, options?: Partial<import('../types/circuit').SimOptions>) =>
    request<import('../types/circuit').SimResult>('/simulations/run', {
      method: 'POST', body: JSON.stringify({ circuit, options: options || {} }),
    }),
  health: () =>
    request<{ status: string; version: string; sim_core_available: boolean }>('/health'),
};
