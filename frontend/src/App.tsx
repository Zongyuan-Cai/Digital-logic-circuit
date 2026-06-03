import { DevicePanel } from './components/DevicePanel';
import { CircuitCanvas } from './components/CircuitCanvas';
import { Toolbar } from './components/Toolbar';
import { PropertyPanel } from './components/PropertyPanel';
import { Oscilloscope } from './components/Oscilloscope';
import { useStore } from './store/useStore';

export default function App() {
  const validationMessages = useStore(s => s.validationMessages);

  return (
    <div style={styles.root}>
      <Toolbar />
      {validationMessages.length > 0 && (
        <div style={styles.errors}>
          {validationMessages.map((m, i) => (
            <div key={i} style={{
              ...styles.errorItem,
              background: m.severity === 'error' ? '#f38ba820' : '#f9e2af20',
              borderColor: m.severity === 'error' ? '#f38ba8' : '#f9e2af',
            }}>
              <strong>{m.severity === 'error' ? '❌' : '⚠️'} {m.message}</strong>
              {m.detail && <span style={{ marginLeft: 8, opacity: 0.75, fontSize: 10 }}>{m.detail}</span>}
            </div>
          ))}
        </div>
      )}
      <div style={styles.main}>
        <DevicePanel side="logic" />
        <CircuitCanvas />
        <div style={styles.rightColumn}>
          <DevicePanel side="io" />
          <PropertyPanel />
        </div>
      </div>
      <Oscilloscope />
    </div>
  );
}

const styles: Record<string, React.CSSProperties> = {
  root: {
    display: 'flex', flexDirection: 'column',
    height: '100dvh', width: '100%',
    background: '#11111b', color: '#cdd6f4',
    fontFamily: '-apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif',
    overflow: 'hidden',
  },
  main: {
    display: 'flex', flex: 1,
    minHeight: 0,
    minWidth: 0,
    overflow: 'hidden',
  },
  rightColumn: {
    display: 'flex',
    flexDirection: 'column',
    width: 240,
    minWidth: 240,
    flexShrink: 0,
    minHeight: 0,
    overflow: 'hidden',
  },
  errors: {
    display: 'flex', flexDirection: 'column', gap: 2,
    flexShrink: 0,
    padding: '4px 16px',
    background: '#181825',
    borderBottom: '1px solid #313244',
    maxHeight: 120, overflowY: 'auto' as const,
  },
  errorItem: {
    padding: '4px 10px', borderRadius: 4, border: '1px solid',
    fontSize: 11,
  },
};
