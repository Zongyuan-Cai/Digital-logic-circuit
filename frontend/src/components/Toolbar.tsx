/* Top toolbar with simulation controls */

import { useState } from 'react';
import { useStore } from '../store/useStore';

export function Toolbar() {
  const { simRunning, simResult, runSimulation, resetSimulation, saveProject, loadProject } = useStore();
  const [projectName, setProjectName] = useState('我的电路');
  const [loadId, setLoadId] = useState('');
  const [savedId, setSavedId] = useState<string | null>(null);

  async function handleSave() {
    const id = await saveProject(projectName);
    setSavedId(id);
    setTimeout(() => setSavedId(null), 3000);
  }

  async function handleLoad() {
    if (!loadId.trim()) return;
    await loadProject(loadId.trim());
  }

  return (
    <header style={styles.toolbar}>
      <span style={styles.logo}>🔌 数字逻辑电路仿真</span>

      <div style={styles.group}>
        <input
          style={styles.input}
          value={projectName}
          onChange={e => setProjectName(e.target.value)}
          placeholder="工程名称"
        />
        <button style={styles.btn} onClick={handleSave} title="保存工程">
          💾 保存
        </button>
        {savedId && <span style={styles.savedMsg}>已保存 ({savedId})</span>}
      </div>

      <div style={styles.group}>
        <input
          style={{ ...styles.input, width: 100 }}
          value={loadId}
          onChange={e => setLoadId(e.target.value)}
          placeholder="工程ID"
        />
        <button style={styles.btn} onClick={handleLoad} title="加载工程">
          📂 加载
        </button>
      </div>

      <div style={styles.spacer} />

      <div style={styles.group}>
        <button
          style={{ ...styles.btn, ...styles.runBtn }}
          onClick={runSimulation}
          disabled={simRunning}
          title="运行仿真"
        >
          {simRunning ? '⏳ 运行中...' : '▶ 运行'}
        </button>
        <button style={styles.btn} onClick={resetSimulation} title="复位">
          🔄 复位
        </button>
      </div>

      {simResult && (
        <span style={styles.stats}>
          ✅ {simResult.status} | ticks: {simResult.ticks_elapsed} | events: {simResult.events_processed}
        </span>
      )}
    </header>
  );
}

const styles: Record<string, React.CSSProperties> = {
  toolbar: {
    display: 'flex', alignItems: 'center', gap: 12,
    padding: '8px 16px',
    background: '#1e1e2e', borderBottom: '1px solid #313244',
    color: '#cdd6f4', flexWrap: 'wrap',
  },
  logo: {
    fontSize: 16, fontWeight: 700, marginRight: 16,
  },
  group: {
    display: 'flex', alignItems: 'center', gap: 6,
  },
  spacer: { flex: 1 },
  btn: {
    padding: '6px 14px', borderRadius: 6,
    border: '1px solid #45475a', background: '#313244',
    color: '#cdd6f4', fontSize: 12,
    cursor: 'pointer', whiteSpace: 'nowrap' as const,
  },
  runBtn: {
    background: '#a6e3a1', color: '#1e1e2e', borderColor: '#a6e3a1',
    fontWeight: 600,
  },
  input: {
    padding: '4px 8px', borderRadius: 4,
    border: '1px solid #313244', background: '#181825',
    color: '#cdd6f4', fontSize: 11, width: 120,
  },
  savedMsg: {
    fontSize: 10, color: '#a6e3a1',
  },
  stats: {
    fontSize: 11, color: '#a6adc8',
  },
};
