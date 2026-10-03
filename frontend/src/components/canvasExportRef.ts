let canvasSvgRef: SVGSVGElement | null = null;

export function setCanvasSvgRef(ref: SVGSVGElement | null) {
  canvasSvgRef = ref;
}

export function getCanvasSvgRef() {
  return canvasSvgRef;
}

let canvasCenter = () => ({ x: 400, y: 200 });
export function setCanvasCenter(getCenter: () => { x: number; y: number }) { canvasCenter = getCenter; }
export function getCanvasCenter() { return canvasCenter(); }
