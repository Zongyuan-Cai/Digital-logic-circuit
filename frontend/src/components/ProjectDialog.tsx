import { useEffect, useState } from 'react';
import { api } from '../api/client';
import { useStore } from '../store/useStore';
import type { ProjectMeta } from '../types/circuit';
import { Modal } from './Modal';
import { Icon } from './Icon';

export function ProjectDialog({ mode, onClose, onSaved }: { mode: 'save' | 'open' | 'new'; onClose: () => void; onSaved: (message: string) => void }) {
  const projectName = useStore(s => s.projectName);
  const dirty = useStore(s => s.circuit !== s.savedCircuit);
  const [name, setName] = useState(projectName);
  const [projects, setProjects] = useState<ProjectMeta[] | null>(null);
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState('');
  const [pending, setPending] = useState<ProjectMeta | null>(null);
  const [retry, setRetry] = useState(0);
  useEffect(() => {
    if (mode !== 'open') return;
    let active = true;
    api.listProjects().then(list => { if (active) setProjects(list); }).catch(error => { if (active) setError(error instanceof Error ? error.message : '无法读取工程列表'); });
    return () => { active = false; };
  }, [mode, retry]);
  async function save(event: React.FormEvent) {
    event.preventDefault();
    if (!name.trim() || busy) return;
    setBusy(true); setError('');
    try { await useStore.getState().saveProject(name.trim()); onSaved('工程已保存'); onClose(); }
    catch (error) { setError(error instanceof Error ? error.message : '保存失败'); }
    finally { setBusy(false); }
  }
  async function open(project: ProjectMeta) {
    if (busy) return;
    setBusy(true); setError('');
    try { await useStore.getState().loadProject(project.id); window.dispatchEvent(new Event('circuit:fit')); onSaved(`已打开 ${project.name}`); onClose(); }
    catch (error) { setError(error instanceof Error ? error.message : '加载失败'); }
    finally { setBusy(false); }
  }
  return <Modal title={mode === 'save' ? '保存工程' : mode === 'open' ? '打开工程' : '新建工程'} onClose={() => { if (!busy) onClose(); }}>
    <div className="modal-body">
      {error && <div className="dialog-error" role="alert"><Icon name="warning" size={16} /><span>{error}</span>{mode === 'open' && !projects && <button onClick={() => { setError(''); setRetry(value => value + 1); }}>重试</button>}</div>}
      {mode === 'save' && <form onSubmit={save}><p className="modal-description">将电路和器件参数保存到工程库，方便下次继续实验。</p><label className="form-field">工程名称<input autoFocus maxLength={100} value={name} onChange={event => setName(event.target.value)} placeholder="为这个电路取个名字" required /></label><div className="modal-actions"><button type="button" className="button secondary" disabled={busy} onClick={onClose}>取消</button><button className="button primary" type="submit" disabled={busy || !name.trim()}><Icon name="save" size={16} />{busy ? '正在保存…' : '保存工程'}</button></div></form>}
      {mode === 'new' && <><p className="modal-description">新工程将从高电平、低电平和时钟源开始。{dirty && '当前电路有未保存的更改，请先保存需要保留的内容。'}</p><div className="modal-actions"><button className="button secondary" onClick={onClose}>取消</button><button className="button primary" onClick={() => { useStore.getState().newProject(); window.dispatchEvent(new Event('circuit:fit')); onClose(); }}>新建工程</button></div></>}
      {mode === 'open' && <>{pending ? <><p className="modal-description">当前电路有未保存的更改。打开「{pending.name}」将替换当前画布。</p><div className="modal-actions"><button className="button secondary" disabled={busy} onClick={() => setPending(null)}>返回列表</button><button className="button primary" disabled={busy} onClick={() => void open(pending)}>{busy ? '正在打开…' : '继续打开'}</button></div></> : <>
        {!projects && !error && <p className="modal-description">正在读取工程库…</p>}
        {projects?.length === 0 && <div className="project-list-empty"><Icon name="folder" size={36} /><h3>工程库还是空的</h3><p>保存第一个电路后，就能在这里找到它。</p></div>}
        <div className="project-list">{projects?.map(project => <button className="project-list-item" key={project.id} disabled={busy} onClick={() => { if (dirty) setPending(project); else void open(project); }}><span className="project-file-icon"><Icon name="circuit" size={22} /></span><span><strong>{project.name}</strong><small>{new Date(project.updated_at).toLocaleString('zh-CN', { timeZone: 'Asia/Shanghai' })}</small></span><Icon name="chevron" size={16} /></button>)}</div>
      </>}</>}
    </div>
  </Modal>;
}
