export function DeviceGlyph({ type, className }: { type: string; className?: string }) {
  const inverted = ['NAND', 'NOR', 'XNOR', 'NOT'].includes(type);
  const gate = ['AND', 'NAND', 'OR', 'NOR', 'XOR', 'XNOR', 'NOT'].includes(type);
  return (
    <svg viewBox="0 0 80 48" className={className ?? 'device-glyph'} fill="none" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
      {gate ? <>
        <path d={type === 'NOT' ? 'M24 8 53 24 24 40Z' : ['AND', 'NAND'].includes(type) ? 'M24 8h13a16 16 0 0 1 0 32H24Z' : 'M20 8Q36 24 20 40Q48 40 57 24Q48 8 20 8Z'} />
        {['XOR', 'XNOR'].includes(type) && <path d="M15 8Q31 24 15 40" />}
        <path d={type === 'NOT' ? 'M9 24h15' : 'M9 16h16 M9 32h16'} />
        {inverted && <circle cx={type === 'NOT' ? 57 : type === 'NAND' ? 57 : 61} cy="24" r="3" />}
        <path d={`M${inverted ? 64 : 57} 24h9`} />
      </> : type === 'SWITCH' ? <><path d="M8 30h18 M52 30h20 M28 28 51 13" /><circle cx="27" cy="30" r="3" /><circle cx="52" cy="30" r="3" /></>
        : type === 'LED' ? <><circle cx="40" cy="24" r="12" /><path d="M8 24h20 M52 24h20 M37 17v14 M37 17l9 7-9 7 M46 17v14" /></>
        : type === 'CLOCK' ? <path d="M8 34h12V14h18v20h18V14h16" />
        : <><rect x="23" y="7" width="34" height="34" rx="4" />{[14, 24, 34].map(y => <path key={y} d={`M14 ${y}h9 M57 ${y}h9`} />)}<text x="40" y="28" textAnchor="middle" fontSize="11" stroke="none" fill="currentColor" fontFamily="monospace">{type === 'SEVEN_SEGMENT' ? '8' : type.includes('FF') || type.includes('LATCH') ? 'FF' : 'IC'}</text></>}
    </svg>
  );
}
