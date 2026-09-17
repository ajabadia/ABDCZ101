// Oscilloscope — real-audio waveform display (parity with the native
// WaveformDisplay). Extracted from app.js to keep responsibilities separated.
//
// Factory pattern: the canvas/ctx are stable DOM refs; the engine and the
// audio-started flag are live references (created/started later), so they come
// through getters. The native has NO decorative pattern — its waveformData
// starts at zeros and only moves when real audio arrives (AUDIO_SCOPE
// snapshots). So offline / silent we draw a flat center line over the grid,
// never a fake animated wave.
export function createOscilloscope(deps) {
  const { canvas, ctx, getAudioEngine, getIsAudioStarted } = deps;

  let juceScopeSamples = null;

  let lastNoteStr = "";

  const handleScopeData = (data) => {
    const payload = (data && data.detail) ? data.detail : data;
    if (payload) {
      if (payload.samples) {
        juceScopeSamples = payload.samples;
      } else if (Array.isArray(payload)) {
        juceScopeSamples = payload;
      }
      if (payload.note !== undefined && payload.note >= 0) {
        const noteNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
        const oct = Math.floor(payload.note / 12) - 1;
        const name = noteNames[payload.note % 12];
        lastNoteStr = `NOTE: ${name}${oct} (${payload.note})`;
      }
    }
  };

  const setupScopeListener = () => {
    const backend = window.getJuceBackend ? window.getJuceBackend() : ((window.__JUCE__ && window.__JUCE__.backend) || (window.Juce && window.Juce.backend));
    if (backend && backend.addEventListener) {
      backend.addEventListener('scopeUpdate', handleScopeData);
    }
  };
  setupScopeListener();
  setTimeout(setupScopeListener, 300);
  window.addEventListener('scopeUpdate', handleScopeData);

  function drawOscilloscope() {
    requestAnimationFrame(drawOscilloscope);

    const dpr = window.devicePixelRatio || 1;
    const cssW = 200; // HTML defined width
    const cssH = 60;  // HTML defined height
    
    if (canvas.width !== Math.floor(cssW * dpr) || canvas.height !== Math.floor(cssH * dpr)) {
      canvas.width = Math.floor(cssW * dpr);
      canvas.height = Math.floor(cssH * dpr);
    }
    
    const w = cssW;
    const h = cssH;
    const centerY = h / 2;

    ctx.save();
    ctx.scale(dpr, dpr);

    // Clear + grid (parity with the native paint: subtle vertical/horizontal
    // lines every 15 px).
    const computedStyle = getComputedStyle(document.body);
    const textColor = computedStyle.getPropertyValue('--color-lcd-text').trim() || '#1a1a1a';
    
    // Clear background (transparent so the container's LCD background shows through)
    ctx.clearRect(0, 0, w, h);
    
    // Grid
    ctx.globalAlpha = 0.15;
    ctx.strokeStyle = textColor;
    ctx.lineWidth = 1;
    ctx.beginPath();
    for (let x = 0; x < w; x += 15) { ctx.moveTo(x, 0); ctx.lineTo(x, h); }
    for (let y = 0; y < h; y += 15) { ctx.moveTo(0, y); ctx.lineTo(w, y); }
    ctx.stroke();

    // Real samples when the engine is running and has produced audio; otherwise
    // a flat line (native: waveformData is zeros until pushBuffer fills it).
    let scope = null;
    
    if (window.__JUCE__) {
      scope = juceScopeSamples;
    } else {
      const audioEngine = getAudioEngine();
      scope = (audioEngine && getIsAudioStarted() && audioEngine.scopeSamples && audioEngine.scopeSamples.length)
        ? audioEngine.scopeSamples
        : null;
    }

    ctx.globalAlpha = 1.0;
    ctx.strokeStyle = textColor;
    ctx.lineWidth = 1.5;
    ctx.beginPath();

    if (scope) {
      const n = scope.length;
      const amp = h * 0.45; // like the native WaveformDisplay scaling
      for (let i = 0; i < w; i++) {
        const idx = Math.floor((i / w) * n) % n;
        const x = i;
        const y = centerY - scope[idx] * amp;
        if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
      }
    } else {
      ctx.moveTo(0, centerY);
      ctx.lineTo(w, centerY);
    }

    ctx.stroke();
    
    // Draw the active MIDI note in small text
    if (lastNoteStr !== "") {
      ctx.fillStyle = textColor;
      ctx.font = "10px 'Share Tech Mono', monospace";
      ctx.globalAlpha = 0.8;
      ctx.fillText(lastNoteStr, 4, 12);
    }
    
    ctx.restore();
  }
  drawOscilloscope();
}
