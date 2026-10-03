import { useEffect, useState } from 'react';
import { useStore } from '../store/useStore';
import { displayName, FIXED_DEVICE_TYPES, matchesSearch } from '../lib/devices';
import { DeviceGlyph } from './DeviceGlyph';
import { Icon } from './Icon';
import { getCanvasCenter } from './canvasExportRef';
import type { DeviceDef } from '../types/circuit';

const filters = [
  { key: 'all', label: '全部' }, { key: 'gate', label: '门电路' }, { key: 'flip_flop', label: '触发器' },
  { key: 'combinational', label: '组合芯片' }, { key: 'sequential', label: '时序芯片' },
];
function groupName(device: DeviceDef) {
  return device.category === 'chip' ? device.sub_category === 'sequential' ? '时序逻辑芯片' : '组合逻辑芯片'
    : device.category === 'gate' ? '基本门电路' : device.category === 'flip_flop' ? '触发器与锁存器' : '输入输出';
}

export function DevicePanel({ side = 'logic' }: { side?: 'logic' | 'io' }) {
  const devices = useStore(s => s.devices);
  const loaded = useStore(s => s.devicesLoaded);
  const loading = useStore(s => s.devicesLoading);
  const error = useStore(s => s.devicesError);
  const loadDevices = useStore(s => s.loadDevices);
  const addDevice = useStore(s => s.addDevice);
  const [filter, setFilter] = useState('all');
  const [search, setSearch] = useState('');
  const [collapsed, setCollapsed] = useState<Record<string, boolean>>({});
  const [width, setWidth] = useState<number | null>(null);
  const [resizing, setResizing] = useState<{ x: number; width: number } | null>(null);
  useEffect(() => { void loadDevices(); }, [loadDevices]);

  const filtered = devices.filter(device => {
    if (FIXED_DEVICE_TYPES.has(device.type) || !matchesSearch(device, search)) return false;
    if (side === 'io') return device.category === 'io';
    if (device.category === 'io') return false;
    return filter === 'all' || device.category === filter || device.category === 'chip' && device.sub_category === filter;
  });
  const groups = new Map<string, DeviceDef[]>();
  for (const device of filtered) {
    const name = groupName(device);
    groups.set(name, [...(groups.get(name) ?? []), device]);
  }
  function add(type: string) {
    const center = getCanvasCenter();
    const offset = (useStore.getState().circuit.devices.length % 5) * 20;
    addDevice(type, center.x + offset, center.y + offset);
  }
  const deviceCard = (device: DeviceDef) => <button
    key={device.type} className={`device-card category-${device.category}`} draggable
    aria-label={`添加 ${device.type} ${displayName(device)}`} title={device.description}
    onClick={() => add(device.type)}
    onDragStart={event => { event.dataTransfer.setData('device-type', device.type); event.dataTransfer.effectAllowed = 'copy'; }}
  >
    <DeviceGlyph type={device.type} />
    {side === 'logic' && <span className="device-model">{device.type.replaceAll('_', ' ')}</span>}
    <span className="device-name">{displayName(device)}</span>
    <span className="device-add"><Icon name="plus" size={12} /></span>
  </button>;

  return <aside className={`device-panel ${side === 'io' ? 'io-panel' : 'library-panel'}`} style={side === 'logic' && width ? { width } : undefined} aria-label={side === 'io' ? '输入输出器件' : '器件库'}>
    <div className="panel-heading"><span><Icon name={side === 'io' ? 'sliders' : 'chip'} size={16} /><h2>{side === 'io' ? '输入输出' : '器件库'}</h2></span><span className="count-badge">{side === 'io' ? filtered.length : devices.filter(device => device.category !== 'io').length}</span></div>
    {side === 'logic' && <>
      <label className="search-field"><Icon name="search" size={15} /><input aria-label="搜索器件" placeholder="搜索名称、型号…" value={search} onChange={event => setSearch(event.target.value)} />{search && <button aria-label="清空搜索" onClick={() => setSearch('')}><Icon name="close" size={14} /></button>}</label>
      <div className="library-filters" aria-label="器件分类">{filters.map(item => <button key={item.key} aria-pressed={filter === item.key} className={filter === item.key ? 'active' : ''} onClick={() => setFilter(item.key)}>{item.label}</button>)}</div>
    </>}
    <div className="device-list">
      {loading && <div className="panel-message">正在加载器件库…</div>}
      {error && <div className="panel-message error-message"><Icon name="warning" /><p>无法加载器件库</p><small>{error}</small><button className="button secondary" onClick={() => void loadDevices()}>重新加载</button></div>}
      {loaded && !filtered.length && <div className="panel-message">没有匹配的器件<span>试试其他名称或分类</span></div>}
      {Array.from(groups, ([name, group]) => <section key={name} className="device-group">
        {side === 'logic' && <button className="group-heading" aria-expanded={!collapsed[name]} onClick={() => setCollapsed(state => ({ ...state, [name]: !state[name] }))}><Icon name="chevron" size={12} className={collapsed[name] ? '' : 'expanded'} /><span>{name}</span><small>{group.length}</small></button>}
        {!collapsed[name] && <div className="device-grid">{group.map(deviceCard)}</div>}
      </section>)}
    </div>
    {side === 'logic' && <div className="library-footnote"><Icon name="cursor" size={14} /><span>拖入画布，或点击添加器件</span></div>}
    {side === 'logic' && <div className="library-resize" role="separator" aria-label="调整器件库宽度" aria-orientation="vertical" aria-valuenow={width ?? 256} aria-valuemin={200} aria-valuemax={360} tabIndex={0}
      onPointerDown={event => { event.currentTarget.setPointerCapture?.(event.pointerId); setResizing({ x: event.clientX, width: event.currentTarget.parentElement?.getBoundingClientRect().width ?? 256 }); }}
      onPointerMove={event => { if (resizing) setWidth(Math.max(200, Math.min(360, resizing.width + event.clientX - resizing.x))); }}
      onPointerUp={() => setResizing(null)} onPointerCancel={() => setResizing(null)}
      onKeyDown={event => { if (event.key === 'ArrowLeft' || event.key === 'ArrowRight') { event.preventDefault(); setWidth(value => Math.max(200, Math.min(360, (value ?? 256) + (event.key === 'ArrowLeft' ? -20 : 20)))); } }} />}
  </aside>;
}
