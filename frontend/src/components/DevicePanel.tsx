/* Device library panel */

import { useEffect, useState, useRef, useCallback } from 'react';
import { useStore } from '../store/useStore';
import type { DeviceCategory } from '../types/circuit';

// VCC, GND, CLOCK are fixed starter sources on the canvas.
const ALWAYS_ON_CANVAS = new Set(['VCC', 'GND', 'CLOCK']);

type FilterKey = DeviceCategory | 'all' | 'combinational_chip' | 'sequential_chip';

const FILTER_BUTTONS: { key: FilterKey; label: string }[] = [
  { key: 'all', label: '全部' },
  { key: 'gate', label: '门电路' },
  { key: 'flip_flop', label: '触发器' },
  { key: 'combinational_chip', label: '组合逻辑芯片' },
  { key: 'sequential_chip', label: '时序逻辑芯片' },
];

// Strip English type prefix from device name for Chinese-only display
function displayName(d: { name: string; type: string }): string {
  // Pattern: "TYPE Chinese Name" → "Chinese Name"
  const idx = d.name.search(/[一-鿿]/);
  if (idx > 0) return d.name.slice(idx);
  // Fallback: if name starts with type, strip it
  if (d.name.startsWith(d.type)) {
    const rest = d.name.slice(d.type.length).trim();
    if (rest) return rest;
  }
  return d.name;
}

// Map device category+sub_category to display group label
function getGroupLabel(d: { category: DeviceCategory; sub_category?: string }): string {
  if (d.category === 'chip') {
    if (d.sub_category === 'combinational') return '组合逻辑芯片';
    if (d.sub_category === 'sequential') return '时序逻辑芯片';
  }
  if (d.category === 'io') return '输入输出';
  const found = FILTER_BUTTONS.find(c => c.key === d.category);
  return found?.label || d.category;
}

const DEFAULT_WIDTH = 240;

interface DevicePanelProps {
  side?: 'logic' | 'io';
}

export function DevicePanel({ side = 'logic' }: DevicePanelProps) {
  const { devices, devicesLoaded, loadDevices } = useStore();
  const [filter, setFilter] = useState<FilterKey>('all');
  const [search, setSearch] = useState('');
  const [collapsed, setCollapsed] = useState<Record<string, boolean>>({});
  const [width, setWidth] = useState(DEFAULT_WIDTH);
  const resizing = useRef(false);
  const panelWidth = side === 'io' ? '100%' : width;

  useEffect(() => {
    if (!devicesLoaded) loadDevices();
  }, [devicesLoaded, loadDevices]);

  const onMouseDown = useCallback((e: React.MouseEvent) => {
    e.preventDefault();
    resizing.current = true;
    const startX = e.clientX;
    const startW = width;
    function onMove(ev: MouseEvent) {
      const delta = ev.clientX - startX;
      const maxWidth = Math.max(180, Math.min(360, window.innerWidth - 360));
      const next = Math.max(180, Math.min(maxWidth, startW + delta));
      setWidth(next);
    }
    function onUp() {
      resizing.current = false;
      document.removeEventListener('mousemove', onMove);
      document.removeEventListener('mouseup', onUp);
    }
    document.addEventListener('mousemove', onMove);
    document.addEventListener('mouseup', onUp);
  }, [width]);

  const filtered = devices.filter(d => {
    // Hide the fixed starter sources.
    if (ALWAYS_ON_CANVAS.has(d.type)) return false;
    if (side === 'io') return d.category === 'io' && matchesSearch(d, search);
    if (d.category === 'io') return false;
    if (filter === 'combinational_chip') return d.category === 'chip' && d.sub_category === 'combinational';
    if (filter === 'sequential_chip') return d.category === 'chip' && d.sub_category === 'sequential';
    if (filter !== 'all' && d.category !== filter) return false;
    if (!matchesSearch(d, search)) return false;
    return true;
  });

  // Group by display label (chips split into 组合逻辑/时序逻辑)
  const grouped: Record<string, typeof filtered> = {};
  for (const d of filtered) {
    const groupKey = getGroupLabel(d);
    if (!grouped[groupKey]) grouped[groupKey] = [];
    grouped[groupKey].push(d);
  }

  function handleDragStart(e: React.DragEvent, type: string) {
    e.dataTransfer.setData('device-type', type);
    e.dataTransfer.effectAllowed = 'copy';
  }

  const getColor = (cat: DeviceCategory): string => {
    switch (cat) {
      case 'gate': return '#4caf50';
      case 'flip_flop': return '#ff9800';
      case 'chip': return '#2196f3';
      case 'io': return '#9e9e9e';
      default: return '#9e9e9e';
    }
  };

  return (
    <aside style={{
      ...styles.panel,
      ...(side === 'io' ? styles.rightPanel : styles.leftPanel),
      width: panelWidth,
      minWidth: panelWidth,
      ...(side === 'io' ? styles.ioPanel : {}),
    }}>
      <h3 style={styles.title}>{side === 'io' ? '输入输出' : '器件库'}</h3>
      <input
        style={styles.search}
        placeholder="搜索器件..."
        value={search}
        onChange={e => setSearch(e.target.value)}
      />
      {side === 'logic' && (
        <div style={styles.filters}>
          {FILTER_BUTTONS.map(c => (
            <button
              key={c.key}
              style={{
                ...styles.filterBtn,
                ...(filter === c.key ? styles.filterBtnActive : {}),
              }}
              onClick={() => setFilter(c.key)}
            >
              {c.label}
            </button>
          ))}
        </div>
      )}
      <div style={styles.list}>
        {Object.entries(grouped).map(([cat, devs]) => (
          <div key={cat}>
            <div
              style={styles.catHeader}
              onClick={() => setCollapsed(s => ({ ...s, [cat]: !s[cat] }))}
            >
              <span>{collapsed[cat] ? '▶' : '▼'}</span> {cat} ({devs.length})
            </div>
            {!collapsed[cat] && devs.map(d => (
              <div
                key={d.type}
                draggable
                onDragStart={e => handleDragStart(e, d.type)}
                style={styles.deviceItem}
                title={d.description}
              >
                <span style={{ ...styles.dot, background: getColor(d.category) }} />
                <span style={styles.deviceName}>{displayName(d)}</span>
              </div>
            ))}
          </div>
        ))}
      </div>
      {side === 'logic' && (
        <div
          style={styles.resizeHandle}
          onMouseDown={onMouseDown}
        />
      )}
    </aside>
  );
}

function matchesSearch(d: { name: string; type: string }, search: string): boolean {
  return !search || d.name.includes(search) || d.type.includes(search);
}

const styles: Record<string, React.CSSProperties> = {
  panel: {
    background: '#1e1e2e', color: '#cdd6f4',
    display: 'flex', flexDirection: 'column',
    overflow: 'hidden',
    position: 'relative',
  },
  leftPanel: {
    borderRight: '1px solid #313244',
  },
  rightPanel: {
    borderLeft: '1px solid #313244',
  },
  ioPanel: {
    height: 260,
    flexShrink: 0,
    borderBottom: '1px solid #313244',
  },
  title: {
    padding: '12px 16px', margin: 0,
    fontSize: 15, fontWeight: 600,
    borderBottom: '1px solid #313244',
  },
  search: {
    margin: 8, padding: '6px 10px',
    borderRadius: 4, border: '1px solid #313244',
    background: '#181825', color: '#cdd6f4',
    fontSize: 13,
  },
  filters: {
    display: 'flex', gap: 4, padding: '0 8px 8px',
    flexWrap: 'wrap',
  },
  filterBtn: {
    padding: '3px 10px', fontSize: 12,
    border: '1px solid #313244', borderRadius: 12,
    background: '#181825', color: '#a6adc8',
    cursor: 'pointer',
  },
  filterBtnActive: {
    background: '#89b4fa', color: '#1e1e2e', borderColor: '#89b4fa',
  },
  list: {
    flex: 1, overflowY: 'auto' as const,
    padding: '0 8px 8px',
  },
  catHeader: {
    padding: '6px 4px', fontSize: 12,
    fontWeight: 600, color: '#a6adc8',
    cursor: 'pointer', userSelect: 'none' as const,
  },
  deviceItem: {
    display: 'flex', alignItems: 'center', gap: 8,
    padding: '6px 8px', marginBottom: 2,
    borderRadius: 4, cursor: 'grab',
    fontSize: 13, background: '#181825',
    border: '1px solid transparent',
    transition: 'border-color 0.2s',
  },
  dot: {
    width: 8, height: 8, borderRadius: '50%', flexShrink: 0,
  },
  deviceName: {
    overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' as const,
  },
  resizeHandle: {
    position: 'absolute',
    right: 0, top: 0, bottom: 0,
    width: 4,
    background: 'transparent',
    transition: 'background 0.15s',
  },
};
