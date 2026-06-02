/* Left sidebar: device library panel */

import { useEffect, useState } from 'react';
import { useStore } from '../store/useStore';
import type { DeviceCategory } from '../types/circuit';

const CATEGORIES: { key: DeviceCategory | 'all'; label: string }[] = [
  { key: 'all', label: '全部' },
  { key: 'gate', label: '门电路' },
  { key: 'flip_flop', label: '触发器' },
  { key: 'chip', label: '芯片' },
];

export function DevicePanel() {
  const { devices, devicesLoaded, loadDevices } = useStore();
  const [filter, setFilter] = useState<DeviceCategory | 'all'>('all');
  const [search, setSearch] = useState('');
  const [collapsed, setCollapsed] = useState<Record<string, boolean>>({});

  useEffect(() => {
    if (!devicesLoaded) loadDevices();
  }, [devicesLoaded, loadDevices]);

  const filtered = devices.filter(d => {
    if (filter !== 'all' && d.category !== filter) return false;
    if (search && !d.name.includes(search) && !d.type.includes(search)) return false;
    return true;
  });

  const grouped: Record<string, typeof filtered> = {};
  for (const d of filtered) {
    const cat = CATEGORIES.find(c => c.key === d.category)?.label || d.category;
    if (!grouped[cat]) grouped[cat] = [];
    grouped[cat].push(d);
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
      default: return '#9e9e9e';
    }
  };

  return (
    <aside style={styles.panel}>
      <h3 style={styles.title}>器件库</h3>
      <input
        style={styles.search}
        placeholder="搜索器件..."
        value={search}
        onChange={e => setSearch(e.target.value)}
      />
      <div style={styles.filters}>
        {CATEGORIES.map(c => (
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
                <span style={styles.deviceName}>{d.name || d.type}</span>
              </div>
            ))}
          </div>
        ))}
      </div>
    </aside>
  );
}

const styles: Record<string, React.CSSProperties> = {
  panel: {
    width: 220, minWidth: 220,
    background: '#1e1e2e', color: '#cdd6f4',
    display: 'flex', flexDirection: 'column',
    borderRight: '1px solid #313244',
    overflow: 'hidden',
  },
  title: {
    padding: '12px 16px', margin: 0,
    fontSize: 14, fontWeight: 600,
    borderBottom: '1px solid #313244',
  },
  search: {
    margin: 8, padding: '6px 10px',
    borderRadius: 4, border: '1px solid #313244',
    background: '#181825', color: '#cdd6f4',
    fontSize: 12,
  },
  filters: {
    display: 'flex', gap: 4, padding: '0 8px 8px',
    flexWrap: 'wrap',
  },
  filterBtn: {
    padding: '2px 8px', fontSize: 11,
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
    padding: '6px 4px', fontSize: 11,
    fontWeight: 600, color: '#a6adc8',
    cursor: 'pointer', userSelect: 'none' as const,
  },
  deviceItem: {
    display: 'flex', alignItems: 'center', gap: 8,
    padding: '6px 8px', marginBottom: 2,
    borderRadius: 4, cursor: 'grab',
    fontSize: 12, background: '#181825',
    border: '1px solid transparent',
    transition: 'border-color 0.2s',
  },
  dot: {
    width: 8, height: 8, borderRadius: '50%', flexShrink: 0,
  },
  deviceName: {
    overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' as const,
  },
};
