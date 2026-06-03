import { useStore } from '../store/useStore';

export function PropertyPanel() {
  const {
    circuit, devices, selectedDeviceId, selectedWireId,
    simResult, removeDevice, removeWire,
    updateDeviceParam, waveformDeviceIds, toggleWaveformDevice,
  } = useStore();

  // Show wire info
  if (selectedWireId) {
    const wire = circuit.wires.find(w => w.id === selectedWireId);
    return (
      <aside style={styles.panel}>
        <h3 style={styles.title}>属性面板</h3>
        <div style={styles.section}>
          <div style={styles.label}>连线</div>
          <div style={styles.value}>{wire?.id}</div>
        </div>
        {wire && (
          <>
            <div style={styles.section}>
              <div style={styles.label}>起点</div>
              <div style={styles.value}>{wire.from.device}.{wire.from.pin}</div>
            </div>
            <div style={styles.section}>
              <div style={styles.label}>终点</div>
              <div style={styles.value}>{wire.to.device}.{wire.to.pin}</div>
            </div>
          </>
        )}
        <div style={styles.actions}>
          <button style={styles.dangerBtn} onClick={() => removeWire(selectedWireId)}>
            🗑 删除连线
          </button>
        </div>
      </aside>
    );
  }

  const dev = circuit.devices.find(d => d.id === selectedDeviceId);
  const def = dev ? devices.find(d => d.type === dev.type) : undefined;

  if (!dev || !def) {
    return (
      <aside style={styles.panel}>
        <h3 style={styles.title}>属性面板</h3>
        <p style={styles.placeholder}>
          选择一个器件查看属性<br />
          <span style={{ fontSize: 10, color: '#6c7086' }}>Delete 键删除选中器件/连线</span>
        </p>
      </aside>
    );
  }

  const pins = def.pins;
  const isClock = dev.type === 'CLOCK';
  const shownInScope = waveformDeviceIds.includes(dev.id);
  const nodeVal = (pinId: string): string | null => {
    if (!simResult) return null;
    return simResult.final_nodes[`${dev.id}.${pinId}`] || null;
  };

  // Check for floating inputs
  const floatingPins: string[] = [];
  for (const pin of pins) {
    if (pin.direction === 'input') {
      const wired = circuit.wires.some(w =>
        (w.to.device === dev.id && w.to.pin === pin.id) ||
        (w.from.device === dev.id && w.from.pin === pin.id)
      );
      if (!wired) floatingPins.push(pin.id);
    }
  }

  const signalStyle = (v: string): React.CSSProperties => ({
    display: 'inline-block', width: 10, height: 10, borderRadius: '50%',
    background: v === '1' ? '#a6e3a1' : v === '0' ? '#585b70' : v === 'X' ? '#f38ba8' : '#89b4fa',
    marginRight: 6,
  });

  return (
    <aside style={styles.panel}>
      <h3 style={styles.title}>属性面板</h3>

      <div style={styles.section}>
        <div style={styles.label}>器件名称</div>
        <div style={styles.value}>{def.name}</div>
      </div>
      <div style={styles.section}>
        <div style={styles.label}>型号</div>
        <div style={styles.value}>{dev.type}</div>
      </div>
      <div style={styles.section}>
        <div style={styles.label}>ID</div>
        <div style={styles.value}>{dev.id}</div>
      </div>

      <div style={styles.section}>
        {isClock ? (
          <div style={styles.scopeFixed}>时钟源波形常驻显示</div>
        ) : (
          <button
            style={{
              ...styles.scopeBtn,
              ...(shownInScope ? styles.scopeBtnActive : {}),
            }}
            onClick={() => toggleWaveformDevice(dev.id)}
          >
            {shownInScope ? '隐藏该器件波形' : '在示波器显示'}
          </button>
        )}
      </div>

      <div style={styles.section}>
        <div style={styles.label}>引脚</div>
        <table style={styles.table}>
          <tbody>
            {pins.map(pin => {
              const val = nodeVal(pin.id);
              const isFloating = floatingPins.includes(pin.id);
              return (
                <tr key={pin.id} style={{
                  ...styles.pinRow,
                  background: isFloating ? '#f9e2af10' : 'transparent',
                }}>
                  <td style={styles.pinDir}>
                    {pin.direction === 'input' ? '←' : pin.direction === 'output' ? '→' : '↔'}
                  </td>
                  <td style={{
                    ...styles.pinName,
                    color: pin.active === 'low' ? '#f38ba8' : '#cdd6f4',
                  }}>
                    {pin.name}
                    {pin.active === 'low' && ' (低有效)'}
                    {isFloating && <span style={{ color: '#f9e2af', fontSize: 9, marginLeft: 4 }}>悬空</span>}
                  </td>
                  <td style={styles.pinRole}>{pin.role}</td>
                  <td style={styles.pinVal}>
                    {val && <span style={signalStyle(val)} />}
                    <span style={{ fontSize: 10, color: val ? '#cdd6f4' : '#6c7086' }}>
                      {val || '--'}
                    </span>
                  </td>
                </tr>
              );
            })}
          </tbody>
        </table>
      </div>

      {floatingPins.length > 0 && (
        <div style={{
          ...styles.section,
          background: '#f9e2af10', borderLeft: '3px solid #f9e2af',
        }}>
          <div style={{ fontSize: 10, color: '#f9e2af' }}>
            ⚠ {floatingPins.join(', ')} 悬空未连接
          </div>
        </div>
      )}

      {(dev.type === 'SWITCH' || dev.type === 'CLOCK') && (
        <div style={styles.section}>
          <div style={styles.label}>参数</div>
          {dev.type === 'SWITCH' && (
            <div style={{ display: 'flex', alignItems: 'center', gap: 8, marginTop: 4 }}>
              <span style={{ fontSize: 11, color: '#a6adc8' }}>输出值:</span>
              <button
                style={{
                  ...styles.editBtn,
                  background: dev.params?.value === 1 ? '#a6e3a1' : '#313244',
                  color: dev.params?.value === 1 ? '#1e1e2e' : '#cdd6f4',
                }}
                onClick={() => updateDeviceParam(dev.id, 'value', 1)}
              >1</button>
              <button
                style={{
                  ...styles.editBtn,
                  background: dev.params?.value === 0 ? '#f38ba8' : '#313244',
                  color: dev.params?.value === 0 ? '#1e1e2e' : '#cdd6f4',
                }}
                onClick={() => updateDeviceParam(dev.id, 'value', 0)}
              >0</button>
            </div>
          )}
          {dev.type === 'CLOCK' && (
            <>
              <div style={{ display: 'flex', alignItems: 'center', gap: 8, marginTop: 4 }}>
                <span style={{ fontSize: 11, color: '#a6adc8' }}>周期:</span>
                <input
                  type="number" min={1} max={100}
                  style={styles.editInput}
                  value={String(dev.params?.period || 2)}
                  onChange={e => updateDeviceParam(dev.id, 'period', Math.max(1, Number(e.target.value)))}
                />
              </div>
              <div style={{ display: 'flex', alignItems: 'center', gap: 8, marginTop: 4 }}>
                <span style={{ fontSize: 11, color: '#a6adc8' }}>初始:</span>
                <select
                  style={styles.editInput}
                  value={String(dev.params?.initial || '0')}
                  onChange={e => updateDeviceParam(dev.id, 'initial', e.target.value)}
                >
                  <option value="0">0</option>
                  <option value="1">1</option>
                </select>
              </div>
            </>
          )}
        </div>
      )}

      {dev.type === 'AND' || dev.type === 'OR' || dev.type === 'NAND' || dev.type === 'NOR' || dev.type === 'XOR' || dev.type === 'XNOR' ? (
        <div style={styles.section}>
          <div style={styles.label}>参数</div>
          <div style={{ display: 'flex', alignItems: 'center', gap: 8, marginTop: 4 }}>
            <span style={{ fontSize: 11, color: '#a6adc8' }}>delay:</span>
            <input
              type="number" min={1} max={100}
              style={styles.editInput}
              value={String(dev.params?.delay || 1)}
              onChange={e => updateDeviceParam(dev.id, 'delay', Math.max(1, Number(e.target.value)))}
            />
          </div>
        </div>
      ) : null}

      <div style={styles.actions}>
        <button style={styles.dangerBtn} onClick={() => removeDevice(dev.id)}>
          🗑 删除器件
        </button>
      </div>
    </aside>
  );
}

const styles: Record<string, React.CSSProperties> = {
  panel: {
    width: '100%', minWidth: 0,
    background: '#1e1e2e', color: '#cdd6f4',
    display: 'flex', flexDirection: 'column',
    borderLeft: '1px solid #313244',
    flex: 1,
    minHeight: 0,
    overflowY: 'auto',
    overflowX: 'hidden',
  },
  title: {
    padding: '12px 16px', margin: 0, fontSize: 14, fontWeight: 600,
    borderBottom: '1px solid #313244',
  },
  placeholder: { padding: 16, color: '#6c7086', fontSize: 12, lineHeight: 1.8 },
  section: { padding: '8px 16px', borderBottom: '1px solid #181825' },
  label: { fontSize: 10, color: '#6c7086', marginBottom: 2, textTransform: 'uppercase' as const },
  value: { fontSize: 13, color: '#cdd6f4' },
  table: { width: '100%', borderCollapse: 'collapse' as const, marginTop: 4 },
  pinRow: { borderBottom: '1px solid #181825' },
  pinDir: { padding: '2px 4px', fontSize: 11, color: '#a6adc8', width: 18 },
  pinName: { padding: '2px 4px', fontSize: 11, fontWeight: 500 },
  pinRole: { padding: '2px 4px', fontSize: 9, color: '#6c7086' },
  pinVal: { padding: '2px 4px' },
  actions: { padding: '12px 16px', display: 'flex', gap: 8 },
  dangerBtn: {
    padding: '6px 14px', borderRadius: 6, fontSize: 12,
    border: '1px solid #f38ba8', background: '#f38ba820',
    color: '#f38ba8', cursor: 'pointer',
  },
  editBtn: {
    padding: '2px 10px', borderRadius: 4, fontSize: 11,
    border: '1px solid #45475a', cursor: 'pointer',
  },
  editInput: {
    width: 60, padding: '2px 6px', borderRadius: 4,
    border: '1px solid #45475a', background: '#181825',
    color: '#cdd6f4', fontSize: 11,
  },
  scopeBtn: {
    width: '100%',
    padding: '7px 10px',
    borderRadius: 4,
    border: '1px solid #45475a',
    background: '#313244',
    color: '#cdd6f4',
    fontSize: 12,
    cursor: 'pointer',
  },
  scopeBtnActive: {
    borderColor: '#89b4fa',
    background: '#89b4fa30',
    color: '#89b4fa',
  },
  scopeFixed: {
    padding: '7px 10px',
    borderRadius: 4,
    border: '1px solid #89b4fa',
    background: '#89b4fa20',
    color: '#89b4fa',
    fontSize: 12,
    textAlign: 'center',
  },
};
