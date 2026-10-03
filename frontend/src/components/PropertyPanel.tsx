import { useStore } from '../store/useStore';
import { defaultParams, displayName, FIXED_DEVICE_IDS } from '../lib/devices';
import { nodeValue, SIGNAL_COLORS } from '../lib/signals';
import { DeviceGlyph } from './DeviceGlyph';
import { Icon } from './Icon';

export function PropertyPanel() {
  const { circuit, devices, selectedDeviceId, selectedWireId, simResult, playbackTime, removeDevice, removeWire, updateDeviceParam, waveformDeviceIds, toggleWaveformDevice } = useStore();
  const device = circuit.devices.find(item => item.id === selectedDeviceId);
  const def = device ? devices.find(item => item.type === device.type) : undefined;
  const wire = circuit.wires.find(item => item.id === selectedWireId);
  const heading = <div className="panel-heading"><span><Icon name="sliders" size={16} /><h2>属性面板</h2></span><span className="subtle-label">INSPECTOR</span></div>;
  if (wire) return <aside className="property-panel" aria-label="属性面板">{heading}<div className="property-body" key={wire.id}>
    <div className="selection-summary"><span className="selection-icon"><Icon name="link" size={24} /></span><div><h3>信号连线</h3><code>{wire.id}</code></div></div>
    <section className="property-section"><h4>连接信息</h4><div className="wire-endpoint"><span>起点</span><code>{wire.from.device}.{wire.from.pin}</code></div><div className="wire-endpoint"><span>终点</span><code>{wire.to.device}.{wire.to.pin}</code></div></section>
    <p className="inspector-note">连线将两个引脚接入同一信号网络。输出冲突会在运行前自动检查。</p>
    <button className="button danger full-width" onClick={() => removeWire(wire.id)}><Icon name="trash" size={15} />删除连线</button>
  </div></aside>;
  if (!device || !def) return <aside className="property-panel" aria-label="属性面板">{heading}<div className="inspector-empty"><span className="empty-selection-icon"><Icon name="cursor" size={28} /></span><h3>探索每一个信号</h3><p>选择画布上的器件或连线，<br />查看引脚、参数与实时电平。</p><div className="inspector-tips"><div><Icon name="cursor" size={15} /><span>点击器件，查看属性</span></div><div><Icon name="link" size={15} /><span>点击两个引脚，建立连线</span></div><div><Icon name="wave" size={15} /><span>添加到示波器，观察时序</span></div></div><small><kbd>Delete</kbd> 删除选中项 <span>·</span> <kbd>Esc</kbd> 取消</small></div></aside>;
  const defaults = defaultParams(def);
  const deviceId = device.id;
  const param = (key: string) => device.params[key] ?? defaults[key];
  const floating = def.pins.filter(pin => pin.direction === 'input' && !circuit.wires.some(item => item.from.device === device.id && item.from.pin === pin.id || item.to.device === device.id && item.to.pin === pin.id));
  const inScope = waveformDeviceIds.includes(device.id);
  function numberField(key: string, label: string, min: number, max: number, unit: string) {
    return <label className="parameter-field"><span>{label}</span><div><input aria-label={label} type="number" min={min} max={max} value={Number(param(key)) || min} onChange={event => { const value = event.target.valueAsNumber; if (Number.isFinite(value)) updateDeviceParam(deviceId, key, Math.max(min, Math.min(max, Math.round(value)))); }} /><span>{unit}</span></div></label>;
  }
  return <aside className="property-panel" aria-label="属性面板">{heading}<div className="property-body" key={device.id}>
    <div className={`selection-summary category-${def.category}`}><span className="selection-icon"><DeviceGlyph type={device.type} /></span><div><h3>{displayName(def)}</h3><code>{device.type}</code></div></div>
    <div className="property-meta"><span>器件 ID</span><code>{device.id}</code></div>
    <p className="device-description">{def.description}</p>
    <section className="property-section"><h4>器件参数</h4>
      {numberField('delay', '传播延迟', 1, 100, 'tick')}
      {device.type === 'SWITCH' && <div className="parameter-field"><span>输出电平</span><div className="level-toggle">{[0, 1].map(value => <button key={value} className={Number(param('value')) === value ? 'active' : ''} aria-pressed={Number(param('value')) === value} onClick={() => updateDeviceParam(device.id, 'value', value)}>{value}<small>{value ? 'HIGH' : 'LOW'}</small></button>)}</div></div>}
      {device.type === 'CLOCK' && <>{numberField('period', '时钟周期', 2, 1000, 'tick')}{numberField('duty_pct', '占空比', 1, 99, '%')}<label className="parameter-field"><span>初始电平</span><select aria-label="初始电平" value={Number(param('initial'))} onChange={event => updateDeviceParam(device.id, 'initial', Number(event.target.value))}><option value={0}>0 · LOW</option><option value={1}>1 · HIGH</option></select></label></>}
    </section>
    <section className="property-section"><h4>引脚状态 <span>{def.pins.length} PINS</span></h4><table className="pin-table"><thead><tr><th>引脚</th><th>方向</th><th>电平</th></tr></thead><tbody>{def.pins.map(pin => {
      const signal = nodeValue(circuit, simResult, device.id, pin.id, playbackTime);
      const unconnected = floating.some(item => item.id === pin.id);
      return <tr key={pin.id}><td><span className={pin.active === 'low' ? 'active-low' : ''} title={pin.active === 'low' ? '低电平有效' : pin.role}>{pin.name}</span>{unconnected && <span className="floating-pin" title="悬空未连接">!</span>}</td><td>{pin.direction === 'input' ? '输入' : pin.direction === 'output' ? '输出' : '双向'}</td><td><span className="signal-pill" style={{ color: signal ? SIGNAL_COLORS[signal] : undefined }}>{signal ?? '—'}</span></td></tr>;
    })}</tbody></table>{floating.length > 0 && <div className="floating-notice"><Icon name="warning" size={14} /><span>{floating.map(pin => pin.id).join(', ')} 悬空未连接</span></div>}</section>
    <section className="property-section"><h4>波形分析</h4>{device.type === 'CLOCK' ? <div className="scope-fixed"><Icon name="wave" size={16} />时钟源波形常驻显示</div> : <button className={`button full-width ${inScope ? 'scope-selected' : 'secondary'}`} aria-pressed={inScope} onClick={() => toggleWaveformDevice(device.id)}><Icon name={inScope ? 'check' : 'wave'} size={16} />{inScope ? '隐藏该器件波形' : '在示波器显示'}</button>}</section>
    <button className="button danger full-width" disabled={FIXED_DEVICE_IDS.has(device.id)} onClick={() => removeDevice(device.id)}><Icon name={FIXED_DEVICE_IDS.has(device.id) ? 'check' : 'trash'} size={15} />{FIXED_DEVICE_IDS.has(device.id) ? '固定信号源' : '删除器件'}</button>
  </div></aside>;
}
