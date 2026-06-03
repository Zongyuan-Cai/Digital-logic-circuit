import { useState, useEffect } from 'react';
import { useStore } from '../store/useStore';
import { api } from '../api/client';
import type { ProjectMeta } from '../types/circuit';

export function Toolbar() {
  const { simRunning, simResult, runSimulation, resetSimulation, saveProject, loadProject, circuit } = useStore();
  const [projectName, setProjectName] = useState('我的电路');
  const [currentProjectId, setCurrentProjectId] = useState<string | null>(null);
  const [projectList, setProjectList] = useState<ProjectMeta[]>([]);
  const [showList, setShowList] = useState(false);

  async function refreshList() {
    try { setProjectList(await api.listProjects()); } catch { /* ignore */ }
  }

  useEffect(() => { refreshList(); }, []);

  async function handleSave() {
    if (currentProjectId) {
      // Update existing
      await api.updateProject(currentProjectId, { name: projectName, circuit });
      setCurrentProjectId(currentProjectId);
    } else {
      const id = await saveProject(projectName);
      setCurrentProjectId(id);
    }
    refreshList();
  }

  async function handleSaveAs() {
    const id = await saveProject(projectName + ' (副本)');
    setCurrentProjectId(id);
    refreshList();
  }

  async function handleLoad(id: string) {
    await loadProject(id);
    setCurrentProjectId(id);
    const proj = projectList.find(p => p.id === id);
    if (proj) setProjectName(proj.name);
    setShowList(false);
  }

  async function handleDelete(id: string) {
    await api.deleteProject(id);
    if (currentProjectId === id) setCurrentProjectId(null);
    refreshList();
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
        <button style={styles.btn} onClick={handleSave} title={currentProjectId ? '保存到当前工程' : '创建新工程'}>
          💾 {currentProjectId ? '保存' : '新建'}
        </button>
        {currentProjectId && (
          <button style={styles.btn} onClick={handleSaveAs} title="另存为">📋 另存</button>
        )}
        <button style={styles.btn} onClick={() => { refreshList(); setShowList(!showList); }} title="工程列表">
          📂 {showList ? '收起' : '打开'}
        </button>
        {currentProjectId && (
          <span style={styles.savedMsg}>ID: {currentProjectId}</span>
        )}
      </div>

      {showList && (
        <div style={styles.dropdown}>
          <div style={styles.dropdownHeader}>
            工程列表 ({projectList.length})
            <button style={styles.smallBtn} onClick={() => setShowList(false)}>✕</button>
          </div>
          {projectList.length === 0 && (
            <div style={{ padding: 8, fontSize: 11, color: '#6c7086' }}>暂无工程</div>
          )}
          {projectList.map(p => (
            <div key={p.id} style={{
              ...styles.dropdownItem,
              background: p.id === currentProjectId ? '#313244' : 'transparent',
            }}>
              <span style={{ flex: 1, fontSize: 11, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' as const }}>
                {p.name}
                <span style={{ color: '#6c7086', marginLeft: 6, fontSize: 9 }}>{p.id}</span>
              </span>
              <button style={styles.smallBtn} onClick={() => handleLoad(p.id)}>加载</button>
              <button style={{ ...styles.smallBtn, color: '#f38ba8' }} onClick={() => handleDelete(p.id)}>🗑</button>
            </div>
          ))}
        </div>
      )}

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
    display: 'flex', alignItems: 'center', gap: 12, position: 'relative' as const,
    padding: '8px 16px',
    background: '#1e1e2e', borderBottom: '1px solid #313244',
    color: '#cdd6f4', flexWrap: 'wrap',
  },
  logo: { fontSize: 16, fontWeight: 700, marginRight: 16 },
  group: { display: 'flex', alignItems: 'center', gap: 6 },
  spacer: { flex: 1 },
  btn: {
    padding: '6px 14px', borderRadius: 6,
    border: '1px solid #45475a', background: '#313244',
    color: '#cdd6f4', fontSize: 12,
    cursor: 'pointer', whiteSpace: 'nowrap' as const,
  },
  runBtn: {
    background: '#a6e3a1', color: '#1e1e2e', borderColor: '#a6e3a1', fontWeight: 600,
  },
  input: {
    padding: '4px 8px', borderRadius: 4,
    border: '1px solid #313244', background: '#181825',
    color: '#cdd6f4', fontSize: 11, width: 120,
  },
  savedMsg: { fontSize: 10, color: '#a6e3a1' },
  stats: { fontSize: 11, color: '#a6adc8' },
  smallBtn: {
    padding: '1px 6px', fontSize: 10, borderRadius: 3,
    border: '1px solid #45475a', background: '#313244',
    color: '#cdd6f4', cursor: 'pointer',
  },
  dropdown: {
    position: 'absolute' as const, top: 44, left: 280, zIndex: 100,
    background: '#1e1e2e', border: '1px solid #45475a', borderRadius: 8,
    minWidth: 300, maxHeight: 300, overflowY: 'auto' as const,
    boxShadow: '0 8px 24px rgba(0,0,0,0.5)',
  },
  dropdownHeader: {
    display: 'flex', justifyContent: 'space-between', alignItems: 'center',
    padding: '6px 10px', fontSize: 11, fontWeight: 600,
    borderBottom: '1px solid #313244',
  },
  dropdownItem: {
    display: 'flex', alignItems: 'center', gap: 6,
    padding: '6px 10px', borderBottom: '1px solid #181825',
  },
};
