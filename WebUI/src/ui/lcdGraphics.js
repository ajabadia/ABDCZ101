export function drawLcdGraphics() {
  const drawWave = (canvas, type) => {
    const dpr = window.devicePixelRatio || 1;
    canvas.width = 36 * dpr;
    canvas.height = 28 * dpr;
    const ctx = canvas.getContext('2d');
    ctx.scale(dpr, dpr);

    ctx.clearRect(0, 0, 36, 28);
    // Use CSS variable for text or fallback to dark blue/black LCD color
    ctx.strokeStyle = getComputedStyle(document.documentElement).getPropertyValue('--color-lcd-text').trim() || '#1c221e';
    ctx.lineWidth = 1.5;
    ctx.beginPath();

    if (type === 'saw') {
      ctx.moveTo(6, 22);
      ctx.lineTo(18, 6);
      ctx.lineTo(18, 22);
      ctx.lineTo(30, 6);
    } else if (type === 'square') {
      ctx.moveTo(6, 22);
      ctx.lineTo(6, 6);
      ctx.lineTo(18, 6);
      ctx.lineTo(18, 22);
      ctx.lineTo(30, 22);
    } else if (type === 'lfo') {
      ctx.moveTo(6, 14);
      ctx.lineTo(12, 6);
      ctx.lineTo(24, 22);
      ctx.lineTo(30, 14);
    } else if (type === 'env') {
      ctx.moveTo(4, 22);
      ctx.lineTo(10, 6);
      ctx.lineTo(18, 12);
      ctx.lineTo(26, 12);
      ctx.lineTo(32, 22);
    }
    ctx.stroke();
  };

  const osc1 = document.getElementById('mini-canvas-osc1');
  const osc2 = document.getElementById('mini-canvas-osc2');
  const lfo = document.getElementById('mini-canvas-lfo');
  const dca = document.getElementById('mini-canvas-dca');
  const dcw = document.getElementById('mini-canvas-dcw');
  const pitch = document.getElementById('mini-canvas-pitch');

  if (osc1) drawWave(osc1, 'saw');
  if (osc2) drawWave(osc2, 'square');
  if (lfo) drawWave(lfo, 'lfo');
  if (dca) drawWave(dca, 'env');
  if (dcw) drawWave(dcw, 'env');
  if (pitch) drawWave(pitch, 'env');
}
