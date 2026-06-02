import { describe, it, expect } from 'vitest';
import { render, screen } from '@testing-library/react';
import App from '../App';

describe('App', () => {
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

  it('renders save button', () => {
    render(<App />);
    expect(screen.getByText(/保存/)).toBeTruthy();
  });

  it('renders canvas hint when empty', () => {
    render(<App />);
    expect(screen.getByText(/从左侧拖拽器件/)).toBeTruthy();
  });
});
