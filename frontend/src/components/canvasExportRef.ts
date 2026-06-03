let canvasSvgRef: SVGSVGElement | null = null;

export function setCanvasSvgRef(ref: SVGSVGElement | null) {
  canvasSvgRef = ref;
}

export function getCanvasSvgRef() {
  return canvasSvgRef;
}
