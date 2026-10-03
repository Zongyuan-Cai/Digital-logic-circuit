import type { CSSProperties } from 'react';
const paths = {
  circuit: 'M4 5h6v6H4z M14 13h6v6h-6z M10 8h7v5 M7 11v5h7',
  file: 'M14 3H6a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V9z M14 3v6h6',
  folder: 'M3 7V5h6l2 2h10v13H3z',
  save: 'M19 21H5a2 2 0 0 1-2-2V3h14l4 4v12a2 2 0 0 1-2 2z M7 3v6h9V3 M7 21v-8h10v8',
  plus: 'M12 5v14 M5 12h14', minus: 'M5 12h14',
  search: 'M21 21l-5-5 M18 10a8 8 0 1 1-16 0 8 8 0 0 1 16 0',
  chevron: 'M9 5l7 7-7 7', undo: 'M9 4 3 10l6 6 M3 10h10a7 7 0 0 1 7 7v3', redo: 'M15 4l6 6-6 6 M21 10H11a7 7 0 0 0-7 7v3',
  play: 'M7 4l14 8-14 8z', pause: 'M8 4v16 M16 4v16', reset: 'M3 11a9 9 0 1 1 2 7 M3 4v7h7',
  trash: 'M3 6h18 M9 6V3h6v3 M5 6l1 15h12l1-15 M10 10v7 M14 10v7', download: 'M12 3v12 M7 10l5 5 5-5 M4 16v5h16v-5',
  help: 'M21 12a9 9 0 1 1-18 0 9 9 0 0 1 18 0 M9 9a3 3 0 0 1 6 0c0 2-3 2-3 3 M12 17h.01', close: 'M6 6l12 12 M18 6 6 18',
  cursor: 'M5 3l14 10-7 1-3 7z', hand: 'M8 12V5a2 2 0 0 1 4 0v6-8a2 2 0 0 1 4 0v8-5a2 2 0 0 1 4 0v9c0 5-3 7-7 7h-1c-2 0-4-1-5-3l-4-5a2 2 0 0 1 3-3l2 2',
  fit: 'M8 3H3v5 M16 3h5v5 M3 16v5h5 M21 16v5h-5 M8 8h8v8H8z',
  chip: 'M6 6h12v12H6z M9 2v4 M15 2v4 M9 18v4 M15 18v4 M2 9h4 M2 15h4 M18 9h4 M18 15h4',
  wave: 'M2 17h5V7h6v10h6V7h3', check: 'M5 12l4 4L19 6',
  link: 'M10 14l4-4 M8 16l-2 2a4 4 0 0 1-6-6l4-4a4 4 0 0 1 6 0 M14 8l2-2a4 4 0 0 1 6 6l-4 4a4 4 0 0 1-6 0',
  sliders: 'M4 21V9 M4 5V3 M12 21v-6 M12 11V3 M20 21v-2 M20 15V3 M1 9h6 M9 15h6 M17 15h6',
  warning: 'M12 3 2 21h20z M12 9v5 M12 17h.01', grid: 'M3 3h7v7H3z M14 3h7v7h-7z M3 14h7v7H3z M14 14h7v7h-7z',
};
export function Icon({ name, size = 18, className, style }: { name: keyof typeof paths; size?: number; className?: string; style?: CSSProperties }) {
  return <svg width={size} height={size} viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.6" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true" className={className} style={style}><path d={paths[name]} /></svg>;
}
