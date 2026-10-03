import { useEffect, useState } from 'react';
import { useStore } from '../store/useStore';
import { exportPng } from '../lib/export';
import { Icon } from './Icon';
import { ProjectDialog } from './ProjectDialog';
import { Modal } from './Modal';

export function Toolbar() {
  const { simRunning, simResult, runSimulation, resetSimulation, clearCanvas, projectName, circuit, savedCircuit, projectId, past, future, undo, redo, playbackActive, stopPlayback, maxTicks, setMaxTicks, devicesLoaded } = useStore();
  const [dialog, setDialog] = useState<'save' | 'open' | 'new' | 'help' | null>(null);
  const [notice, setNotice] = useState<{ message: string; error?: boolean } | null>(null);
  const [exporting, setExporting] = useState(false);
  useEffect(() => {
    function key(event: KeyboardEvent) {
      if (document.querySelector('dialog[open]')) return;
      if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === 's') { event.preventDefault(); setDialog('save'); }
      if ((event.ctrlKey || event.metaKey) && event.key === 'Enter') { event.preventDefault(); if (useStore.getState().devicesLoaded) void useStore.getState().runSimulation(); }
    }
    window.addEventListener('keydown', key);
    return () => window.removeEventListener('keydown', key);
  }, []);
  useEffect(() => {
    if (!notice || notice.error) return;
    const timer = setTimeout(() => setNotice(null), 4000);
    return () => clearTimeout(timer);
  }, [notice]);
  async function download() {
    setExporting(true);
    try { await exportPng(projectName); setNotice({ message: 'PNG 图片已导出' }); }
    catch (error) { setNotice({ message: error instanceof Error ? error.message : '图片导出失败', error: true }); }
    finally { setExporting(false); }
  }
  return <>
    <header className="app-header"><div className="brand"><span className="brand-mark"><Icon name="circuit" size={25} /></span><div><strong>Logic<span>Lab</span><span className="brand-version">01</span></strong><span className="brand-subtitle">数字逻辑电路仿真</span></div></div><span className="header-divider" /><div className="project-title"><Icon name="file" size={16} /><span>{projectName}</span><span className={`save-state ${circuit !== savedCircuit ? 'unsaved' : ''}`}>{circuit !== savedCircuit ? '未保存更改' : projectId ? '已保存' : '本地草稿'}</span></div><div className="header-actions"><button className="icon-button" aria-label="使用帮助" title="使用帮助" onClick={() => setDialog('help')}><Icon name="help" /></button><button className="button secondary export-button" disabled={exporting} onClick={() => void download()}><Icon name="download" size={16} />{exporting ? '导出中…' : '导出 PNG'}</button><button className="button primary save-button" onClick={() => setDialog('save')}><Icon name="save" size={16} />保存工程</button></div></header>
    <div className="editor-toolbar"><div className="toolbar-project-actions"><button className="button ghost" onClick={() => setDialog('new')} title="新建工程"><Icon name="plus" size={16} />新建</button><button className="button ghost" onClick={() => setDialog('open')} title="打开工程"><Icon name="folder" size={16} />打开</button><span className="control-divider" /><button className="icon-button" disabled={!past.length} aria-label="撤销" title="撤销 (Ctrl+Z)" onClick={undo}><Icon name="undo" size={16} /></button><button className="icon-button" disabled={!future.length} aria-label="重做" title="重做 (Ctrl+Shift+Z)" onClick={redo}><Icon name="redo" size={16} /></button><span className="control-divider" /><button className="icon-button clear-button" aria-label="清空画布" title="清空画布（可撤销）" onClick={clearCanvas}><Icon name="trash" size={16} /></button></div><div className="toolbar-simulation-actions"><span className="simulation-indicator"><span className={`status-dot ${simResult ? 'ready' : ''}`} />{simRunning ? '正在计算' : simResult ? '仿真完成' : '编辑模式'}</span><span className="control-divider" /><label className="tick-control"><span>仿真时长</span><input type="number" aria-label="仿真时长" min="1" max="10000" value={maxTicks} onChange={event => setMaxTicks(Number(event.target.value))} /><span>tick</span></label><button className="button secondary reset-button" onClick={resetSimulation} title="复位仿真"><Icon name="reset" size={15} />复位</button>{playbackActive && <button className="icon-button" aria-label="暂停仿真回放" onClick={stopPlayback}><Icon name="pause" size={16} /></button>}<button className="button run-button" onClick={() => void runSimulation()} disabled={simRunning || !devicesLoaded} title="运行仿真 (Ctrl+Enter)"><Icon name="play" size={15} />{simRunning ? '计算中…' : '运行仿真'}<kbd>Ctrl ↵</kbd></button></div></div>
    {notice && <div className={`toast ${notice.error ? 'error' : ''}`} role={notice.error ? 'alert' : 'status'}><Icon name={notice.error ? 'warning' : 'check'} size={16} />{notice.message}<button aria-label="关闭通知" onClick={() => setNotice(null)}><Icon name="close" size={14} /></button></div>}
    {dialog && dialog !== 'help' && <ProjectDialog mode={dialog} onClose={() => setDialog(null)} onSaved={message => setNotice({ message })} />}
    {dialog === 'help' && <Modal title="开始一次逻辑实验" onClose={() => setDialog(null)}><div className="modal-body help-content"><p>从基本门电路到 74 系列芯片，用器件搭建想法，用波形验证逻辑。</p><ol><li><strong>放置器件</strong><span>拖入画布，或点击器件卡片添加。</span></li><li><strong>连接与配置</strong><span>依次点击两个引脚连线；选中器件调整参数。开关可直接在画布上切换。</span></li><li><strong>运行与观察</strong><span>运行仿真，在属性面板将器件加入示波器。拖动时间滑块或点击波形查看特定时刻。</span></li></ol><div className="shortcut-list"><span>保存工程<kbd>Ctrl / ⌘ + S</kbd></span><span>运行仿真<kbd>Ctrl / ⌘ + Enter</kbd></span><span>撤销 / 重做<kbd>Ctrl + Z / Shift + Z</kbd></span><span>适应画布<kbd>F</kbd></span><span>平移画布<kbd>Space + 拖动</kbd></span><span>取消连线 / 删除<kbd>Esc / Delete</kbd></span></div><p className="help-signal-legend">信号电平：<span style={{ color: '#6fe2be' }}>1 高</span><span style={{ color: '#7f8c9f' }}>0 低</span><span style={{ color: '#f08089' }}>X 未知</span><span style={{ color: '#85b5ff' }}>Z 高阻</span></p></div></Modal>}
  </>;
}
