import { useStore } from '../store/useStore';
import { getCanvasSvgRef } from './canvasExportRef';

function exportPng() {
  const svg = getCanvasSvgRef();
  if (!svg) return;

  const clone = svg.cloneNode(true) as SVGSVGElement;
  const svgRect = svg.getBoundingClientRect();
  clone.setAttribute('width', String(svgRect.width));
  clone.setAttribute('height', String(svgRect.height));

  const serializer = new XMLSerializer();
  const svgStr = serializer.serializeToString(clone);
  const svgBlob = new Blob([svgStr], { type: 'image/svg+xml;charset=utf-8' });
  const url = URL.createObjectURL(svgBlob);

  const img = new Image();
  img.onload = () => {
    const canvas = document.createElement('canvas');
    canvas.width = svgRect.width;
    canvas.height = svgRect.height;
    const ctx = canvas.getContext('2d')!;
    ctx.fillStyle = '#11111b';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    ctx.drawImage(img, 0, 0);

    canvas.toBlob((blob) => {
      if (!blob) return;
      const a = document.createElement('a');
      a.download = 'circuit.png';
      a.href = URL.createObjectURL(blob);
      a.click();
      URL.revokeObjectURL(url);
    }, 'image/png');
  };
  img.src = url;
}

export function Toolbar() {
  const simRunning = useStore(s => s.simRunning);
  const simResult = useStore(s => s.simResult);
  const runSimulation = useStore(s => s.runSimulation);
  const resetSimulation = useStore(s => s.resetSimulation);
  const clearCanvas = useStore(s => s.clearCanvas);
  const deviceCount = useStore(s => s.circuit.devices.length);
  const wireCount = useStore(s => s.circuit.wires.length);
  const playbackActive = useStore(s => s.playbackActive);
  const playbackTime = useStore(s => s.playbackTime);
  const playbackMaxTime = useStore(s => s.playbackMaxTime);

  const statsText = simResult
    ? playbackActive
      ? `⏱ 仿真运行中 t=${playbackTime}/${playbackMaxTime}`
      : `✅ 完成 | t: ${simResult.ticks_elapsed} | ev: ${simResult.events_processed}`
    : `器件: ${deviceCount} | 连线: ${wireCount}`;

  return (
    <header style={styles.toolbar}>
      <span style={styles.logo}>🔌 数字逻辑电路仿真</span>

      <div style={styles.group}>
        <button style={styles.btn} onClick={exportPng} title="导出画布为PNG图片">
          💾 保存图片
        </button>
        <button style={styles.btn} onClick={clearCanvas} title="清除画布">
          🗑 清屏
        </button>
      </div>

      <div style={styles.spacer} />

      <div style={styles.group}>
        <button
          style={{ ...styles.btn, ...styles.runBtn }}
          onClick={runSimulation}
          disabled={simRunning || playbackActive}
          title="运行仿真"
        >
          {simRunning ? '⏳ 计算中...' : '▶ 运行'}
        </button>
        <button style={styles.btn} onClick={resetSimulation} title="复位">
          🔄 复位
        </button>
      </div>

      <span style={styles.stats}>{statsText}</span>
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
  stats: { fontSize: 11, color: '#a6adc8' },
};
