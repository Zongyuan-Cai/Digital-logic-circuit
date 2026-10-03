import { getCanvasSvgRef } from '../components/canvasExportRef';

export async function exportPng(name: string): Promise<void> {
  const svg = getCanvasSvgRef();
  if (!svg) throw new Error('画布尚未准备好');
  const rect = svg.getBoundingClientRect();
  if (!rect.width || !rect.height) throw new Error('画布尺寸无效');
  const clone = svg.cloneNode(true) as SVGSVGElement;
  clone.setAttribute('width', String(rect.width)); clone.setAttribute('height', String(rect.height));
  clone.setAttribute('xmlns', 'http://www.w3.org/2000/svg');
  // Inline the small set of CSS-dependent glyph styles for standalone SVG rendering.
  clone.querySelectorAll('.device-glyph, .canvas-glyph').forEach(glyph => { glyph.setAttribute('width', '100%'); glyph.setAttribute('height', '100%'); });
  const url = URL.createObjectURL(new Blob([new XMLSerializer().serializeToString(clone)], { type: 'image/svg+xml;charset=utf-8' }));
  try {
    const image = new Image();
    await new Promise<void>((resolve, reject) => { image.onload = () => resolve(); image.onerror = () => reject(new Error('电路图片生成失败')); image.src = url; });
    const canvas = document.createElement('canvas');
    canvas.width = Math.round(rect.width * 2); canvas.height = Math.round(rect.height * 2);
    const context = canvas.getContext('2d');
    if (!context) throw new Error('浏览器不支持图片导出');
    context.scale(2, 2); context.fillStyle = '#10161d'; context.fillRect(0, 0, rect.width, rect.height); context.drawImage(image, 0, 0);
    const blob = await new Promise<Blob>((resolve, reject) => canvas.toBlob(blob => blob ? resolve(blob) : reject(new Error('无法生成 PNG')), 'image/png'));
    const downloadUrl = URL.createObjectURL(blob);
    const anchor = document.createElement('a');
    anchor.download = `${name.replace(/[<>:"/\\|?*]/g, '_')}.png`; anchor.href = downloadUrl; anchor.click();
    setTimeout(() => URL.revokeObjectURL(downloadUrl), 1000);
  } finally { URL.revokeObjectURL(url); }
}
