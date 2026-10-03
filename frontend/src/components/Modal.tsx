import { useEffect, useId, useRef } from 'react';
import type { ReactNode } from 'react';
import { Icon } from './Icon';

export function Modal({ title, onClose, children }: { title: string; onClose: () => void; children: ReactNode }) {
  const dialog = useRef<HTMLDialogElement>(null);
  const titleId = useId();
  useEffect(() => {
    const element = dialog.current;
    if (element?.showModal) element.showModal(); else element?.setAttribute('open', '');
    element?.querySelector<HTMLInputElement>('input')?.focus();
    return () => element?.close?.();
  }, []);
  return <dialog ref={dialog} className="modal" aria-labelledby={titleId} onCancel={event => { event.preventDefault(); onClose(); }}>
    <div className="modal-heading"><h2 id={titleId}>{title}</h2><button className="icon-button" aria-label="关闭弹窗" onClick={onClose}><Icon name="close" /></button></div>
    {children}
  </dialog>;
}
