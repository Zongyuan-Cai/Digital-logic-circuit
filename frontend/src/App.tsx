import { useCallback, useEffect, useState } from 'react';
import { DevicePanel } from './components/DevicePanel';
import { CircuitCanvas } from './components/CircuitCanvas';
import { Toolbar } from './components/Toolbar';
import { PropertyPanel } from './components/PropertyPanel';
import { Oscilloscope } from './components/Oscilloscope';
import { Icon } from './components/Icon';
import { useStore } from './store/useStore';
import { api } from './api/client';

export default function App() {
  const validationMessages = useStore(s => s.validationMessages);
  const dismissValidation = useStore(s => s.dismissValidation);
  const deviceCount = useStore(s => s.circuit.devices.length);
  const wireCount = useStore(s => s.circuit.wires.length);
  const selectedPin = useStore(s => s.selectedPin);
  const [panel, setPanel] = useState<'library' | 'inspector' | null>(null);
  const [health, setHealth] = useState<'checking' | 'ready' | 'unavailable' | 'offline'>('checking');
  const checkHealth = useCallback(async () => {
    setHealth('checking');
    try { const response = await api.health(); setHealth(response.sim_core_available ? 'ready' : 'unavailable'); }
    catch { setHealth('offline'); }
  }, []);
  useEffect(() => {
    let active = true;
    api.health().then(response => { if (active) setHealth(response.sim_core_available ? 'ready' : 'unavailable'); }).catch(() => { if (active) setHealth('offline'); });
    return () => { active = false; };
  }, []);
  return <div className="app-shell">
    <Toolbar />
    {validationMessages.length > 0 && <div className="validation-panel" role="alert"><div className="validation-heading"><Icon name="warning" size={16} /><strong>{validationMessages.filter(item => item.severity === 'error').length ? '请检查电路后重新运行' : '电路提示'}</strong><span>{validationMessages.length} 项</span><button className="icon-button" aria-label="关闭电路提示" onClick={dismissValidation}><Icon name="close" size={15} /></button></div><div className="validation-list">{validationMessages.map((message, index) => <div key={`${message.message}-${index}`} className={`validation-item ${message.severity}`}><span>{message.severity === 'error' ? '错误' : '提示'}</span><strong>{message.message}</strong><small>{message.detail}</small></div>)}</div></div>}
    <div className="mobile-panel-controls"><button aria-pressed={panel === 'library'} onClick={() => setPanel(value => value === 'library' ? null : 'library')}><Icon name="chip" size={15} />器件库</button><span>电路工作区</span><button aria-pressed={panel === 'inspector'} onClick={() => setPanel(value => value === 'inspector' ? null : 'inspector')}><Icon name="sliders" size={15} />属性</button></div>
    <main className="workspace" data-panel={panel ?? ''}>
      <DevicePanel side="logic" />
      <CircuitCanvas />
      <div className="inspector-column"><PropertyPanel /><DevicePanel side="io" /></div>
      {panel && <button className="panel-backdrop" aria-label="关闭侧栏" onClick={() => setPanel(null)} />}
    </main>
    <Oscilloscope />
    <footer className="app-statusbar"><button className="engine-status" title="点击重新检查后端连接" onClick={() => void checkHealth()}><span className={`status-dot ${health === 'ready' ? 'ready' : health === 'checking' ? '' : 'warning'}`} />{health === 'ready' ? '仿真引擎就绪' : health === 'checking' ? '正在连接引擎' : health === 'unavailable' ? '仿真核心未加载' : '后端未连接'}</button><span className="statusbar-divider" /><span><strong>{deviceCount}</strong> 个器件</span><span><strong>{wireCount}</strong> 条连线</span><span className="statusbar-hint">{selectedPin ? `连线起点 ${selectedPin.device}.${selectedPin.pin} · 选择目标引脚` : '连接想法，验证逻辑。'}</span><span className="signal-legend"><i className="high" />1<i className="low" />0<i className="unknown" />X<i className="high-z" />Z</span></footer>
  </div>;
}
