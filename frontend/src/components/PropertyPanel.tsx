/* Right sidebar: device property panel */

import { useStore } from '../store/useStore';

export function PropertyPanel() {
  const { circuit, devices, selectedDeviceId, simResult } = useStore();

  const dev = circuit.devices.find(d => d.id === selectedDeviceId);
  const def = dev ? devices.find(d => d.type === dev.type) : undefined;

  if (!dev || !def) {
    return (
      <aside style={styles.panel}>
        <h3 style={styles.title}>属性面板</h3>
        <p style={styles.placeholder}>选择一个器件查看属性</p>
      </aside>
    );
  }

  const pins = def.pins;
  const nodeVal = (pinId: string): string | null => {
    if (!simResult) return null;
    return simResult.final_nodes[`${dev.id}.${pinId}`] || null;
  };

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
      {def.description && (
        <div style={styles.section}>
          <div style={styles.label}>描述</div>
          <div style={{ ...styles.value, fontSize: 11 }}>{def.description}</div>
        </div>
      )}

      <div style={styles.section}>
        <div style={styles.label}>引脚</div>
        <table style={styles.table}>
          <tbody>
            {pins.map(pin => {
              const val = nodeVal(pin.id);
              return (
                <tr key={pin.id} style={styles.pinRow}>
                  <td style={styles.pinDir}>
                    {pin.direction === 'input' ? '←' : pin.direction === 'output' ? '→' : '↔'}
                  </td>
                  <td style={{
                    ...styles.pinName,
                    color: pin.active === 'low' ? '#f38ba8' : '#cdd6f4',
                  }}>
                    {pin.name}
                    {pin.active === 'low' && ' (低有效)'}
                  </td>
                  <td style={styles.pinRole}>{pin.role}</td>
                  <td>
                    {val && <span style={signalStyle(val)} />}
                    <span style={{ fontSize: 10, color: '#a6adc8' }}>{val || '--'}</span>
                  </td>
                </tr>
              );
            })}
          </tbody>
        </table>
      </div>
    </aside>
  );
}

const styles: Record<string, React.CSSProperties> = {
  panel: {
    width: 240, minWidth: 240,
    background: '#1e1e2e', color: '#cdd6f4',
    display: 'flex', flexDirection: 'column',
    borderLeft: '1px solid #313244',
    overflow: 'hidden',
  },
  title: {
    padding: '12px 16px', margin: 0,
    fontSize: 14, fontWeight: 600,
    borderBottom: '1px solid #313244',
  },
  placeholder: {
    padding: 16, color: '#6c7086', fontSize: 12,
  },
  section: {
    padding: '8px 16px', borderBottom: '1px solid #181825',
  },
  label: {
    fontSize: 10, color: '#6c7086', marginBottom: 2, textTransform: 'uppercase' as const,
  },
  value: {
    fontSize: 13, color: '#cdd6f4',
  },
  table: {
    width: '100%', borderCollapse: 'collapse' as const, marginTop: 4,
  },
  pinRow: {
    borderBottom: '1px solid #181825',
  },
  pinDir: {
    padding: '2px 4px', fontSize: 11, color: '#a6adc8', width: 20,
  },
  pinName: {
    padding: '2px 4px', fontSize: 11, fontWeight: 500,
  },
  pinRole: {
    padding: '2px 4px', fontSize: 9, color: '#6c7086',
  },
};
