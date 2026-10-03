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

HTMLDialogElement.prototype.showModal = function () { this.open = true; };
HTMLDialogElement.prototype.close = function () { this.open = false; };
Object.defineProperty(globalThis, 'PointerEvent', { value: MouseEvent, configurable: true });
