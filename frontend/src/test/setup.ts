import '@testing-library/jest-dom';
import { vi } from 'vitest';

// Mock fetch for jsdom environment
globalThis.fetch = vi.fn(() =>
  Promise.resolve({
    ok: true,
    status: 200,
    json: () => Promise.resolve([]),
    text: () => Promise.resolve(''),
  } as Response)
);
