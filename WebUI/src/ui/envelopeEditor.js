// Envelope Editor — one editor per type (DCA/DCW/Pitch), each with its own
// line1/line2 canvases and controls (DOM ids suffixed with the type key).
// ENV_TYPE_INDEX keeps the engine's numeric convention (2 = DCA, 1 = DCW,
// 0 = Pitch). Extracted from app.js to keep responsibilities separated.
//
// Factory pattern (same as patchFlow's createCompareManager/createWriteManager):
// the module owns the envelope state + canvas drawing and receives the engine /
// undo / LCD refs through getters so app.js wires the live references.
import { PARAM_MAP } from '../contracts/registry.gen.js';

export function createEnvelopeEditor(deps) {
  const getAudioEngine = deps.getAudioEngine;
  const pushUndo = (...args) => deps.getPushUndo()(...args);
  const lcdLine1 = deps.lcdLine1;
  const lcdLine2 = deps.lcdLine2;

  const ENV_TYPES = ['dca', 'dcw', 'pitch'];
  const ENV_TYPE_INDEX = { dca: 2, dcw: 1, pitch: 0 };
  const ENV_ACCENTS = { dca: '#00ffcc', dcw: '#ffa500', pitch: '#d28eff' };
  const selectedNode = {
    dca: { line1: 0, line2: 0 },
    dcw: { line1: 0, line2: 0 },
    pitch: { line1: 0, line2: 0 }
  };
  let envClipboard = null;

  const envelopeState = {
    dca: {
      line1: { rates: [0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5], levels: [0.8, 0.6, 0.8, 0.8, 0.1, 0.3, 0.8, 0.9], sustainPoint: 2, endPoint: 3 },
      line2: { rates: [0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5], levels: [0.0, 0.0, 0.0, 0.8, 0.0, 0.0, 0.0, 0.0], sustainPoint: 3, endPoint: 4 }
    },
    dcw: {
      line1: { rates: [0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5], levels: [0.8, 0.6, 0.6, 0.0, 0.0, 0.0, 0.0, 0.0], sustainPoint: 2, endPoint: 3 },
      line2: { rates: [0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5], levels: [0.8, 0.6, 0.6, 0.0, 0.0, 0.0, 0.0, 0.0], sustainPoint: 2, endPoint: 3 }
    },
    pitch: {
      line1: { rates: [0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5], levels: [0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5], sustainPoint: 2, endPoint: 3 },
      line2: { rates: [0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5], levels: [0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5], sustainPoint: 2, endPoint: 3 }
    }
  };

  function redrawEnvelopes() {
    ENV_TYPES.forEach(typeKey => {
      drawEnvelope8(typeKey, 1);
      drawEnvelope8(typeKey, 2);
    });
    if (typeof updateMiniGraphics === 'function') {
      updateMiniGraphics();
    }
  }

  function getNodeXPositions(env, w) {
    let times = [];
    let totalTime = 0;
    for (let i = 0; i < 8; i++) {
      let r = Math.max(0, Math.min(99, env.rates[i] || 0)) / 99.0;
      // Exponential scaling: slower rates (lower r) are longer lines
      let t = Math.pow(1.0 - r, 3) + 0.02;
      times.push(t);
      totalTime += t;
    }
    let positions = [];
    let acc = 0;
    for (let i = 0; i < 8; i++) {
      acc += times[i];
      positions.push((acc / totalTime) * w);
    }
    return positions;
  }

  function drawEnvelope8(typeKey, lineNum) {
    const canvas = document.getElementById(`canvas-env-${typeKey}-line${lineNum}`);
    if (!canvas) return;
    const ctx = canvas.getContext('2d');

    const dpr = window.devicePixelRatio || 1;
    const cssW = canvas.clientWidth;
    const cssH = canvas.clientHeight;
    if (cssW > 0 && cssH > 0) {
      const pw = Math.max(1, Math.round(cssW * dpr));
      const ph = Math.max(1, Math.round(cssH * dpr));
      if (canvas.width !== pw || canvas.height !== ph) {
        canvas.width = pw;
        canvas.height = ph;
      }
      ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    }
    const w = cssW > 0 ? cssW : canvas.width / dpr;
    const h = cssH > 0 ? cssH : canvas.height / dpr;

    ctx.fillStyle = '#09090e';
    ctx.fillRect(0, 0, w, h);

    const lineKey = lineNum === 1 ? 'line1' : 'line2';
    const env = envelopeState[typeKey][lineKey];
    const activeNode = selectedNode[typeKey][lineKey];

    const accentColor = ENV_ACCENTS[typeKey];
    const xPositions = getNodeXPositions(env, w);

    // Draw grid lines at node positions instead of evenly
    ctx.strokeStyle = 'rgba(255, 255, 255, 0.04)';
    ctx.lineWidth = 1;
    for (let i = 0; i < 7; i++) {
      ctx.beginPath();
      ctx.moveTo(xPositions[i], 0);
      ctx.lineTo(xPositions[i], h);
      ctx.stroke();
    }
    for (let i = 1; i < 4; i++) {
      const y = (h / 4) * i;
      ctx.beginPath();
      ctx.moveTo(0, y);
      ctx.lineTo(w, y);
      ctx.stroke();
    }

    // Draw path
    ctx.strokeStyle = accentColor;
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.moveTo(0, h);
    for (let i = 0; i < 8; i++) {
      const x = xPositions[i];
      const y = h - (env.levels[i] * (h - 10) + 5);
      ctx.lineTo(x, y);
    }
    ctx.stroke();

    // Draw Sustain line (yellow)
    if (env.sustainPoint >= 0 && env.sustainPoint < 8) {
      const sx = xPositions[env.sustainPoint];
      ctx.strokeStyle = 'rgba(230, 180, 0, 0.5)';
      ctx.beginPath();
      ctx.moveTo(sx, 0);
      ctx.lineTo(sx, h);
      ctx.stroke();
    }

    // Draw End line (red)
    if (env.endPoint >= 0 && env.endPoint < 8) {
      const ex = xPositions[env.endPoint];
      ctx.strokeStyle = 'rgba(200, 50, 50, 0.5)';
      ctx.beginPath();
      ctx.moveTo(ex, 0);
      ctx.lineTo(ex, h);
      ctx.stroke();
    }

    // Draw nodes
    for (let i = 0; i < 8; i++) {
      const x = xPositions[i];
      const y = h - (env.levels[i] * (h - 10) + 5);
      const isSelected = i === activeNode;

      ctx.fillStyle = isSelected ? '#ffffff' : accentColor;
      ctx.beginPath();
      ctx.arc(x, y, isSelected ? 4.5 : 3, 0, Math.PI * 2);
      ctx.fill();

      ctx.strokeStyle = '#ffffff';
      ctx.lineWidth = isSelected ? 1.5 : 0.5;
      ctx.beginPath();
      ctx.arc(x, y, isSelected ? 5 : 3.5, 0, Math.PI * 2);
      ctx.stroke();
    }
  }

  function updateEnvelopeControls(typeKey, lineNum) {
    const lineKey = lineNum === 1 ? 'line1' : 'line2';
    const env = envelopeState[typeKey][lineKey];
    const activeNode = selectedNode[typeKey][lineKey];

    // Update DOM labels
    const lblNode = document.getElementById(`lbl-node-env-${typeKey}-line${lineNum}`);
    if (lblNode) lblNode.innerText = (activeNode + 1).toString();

    const sliderRate = document.getElementById(`slider-rate-env-${typeKey}-line${lineNum}`);
    const sliderLevel = document.getElementById(`slider-level-env-${typeKey}-line${lineNum}`);
    const valRate = document.getElementById(`val-rate-env-${typeKey}-line${lineNum}`);
    const valLevel = document.getElementById(`val-level-env-${typeKey}-line${lineNum}`);

    if (sliderRate) {
      sliderRate.value = env.rates[activeNode];
      sliderRate.dispatchEvent(new CustomEvent('filmstrip-update'));
    }
    if (sliderLevel) {
      sliderLevel.value = env.levels[activeNode];
      sliderLevel.dispatchEvent(new CustomEvent('filmstrip-update'));
    }
    if (valRate) valRate.innerText = env.rates[activeNode].toFixed(2);
    if (valLevel) valLevel.innerText = env.levels[activeNode].toFixed(2);

    // Update SUS and END buttons active states
    const btnSus = document.getElementById(`btn-sus-env-${typeKey}-line${lineNum}`);
    const btnEnd = document.getElementById(`btn-end-env-${typeKey}-line${lineNum}`);
    if (btnSus) btnSus.classList.toggle('active', env.sustainPoint === activeNode);
    if (btnEnd) btnEnd.classList.toggle('active', env.endPoint === activeNode);
  }

  const sendEnvStage = (typeKey, lineNum, stage, rate, level) => {
    const envType = ENV_TYPE_INDEX[typeKey];
    const backend = window.getJuceBackend ? window.getJuceBackend() : ((window.__JUCE__ && window.__JUCE__.backend) || (window.Juce && window.Juce.backend));
    if (backend) {
      if (typeof backend.setEnvelopeStage === 'function') backend.setEnvelopeStage(envType, lineNum, stage, rate, level);
      if (typeof backend.emitEvent === 'function') backend.emitEvent('setEnvelopeStage', { envType, line: lineNum, stage, rate, level });
    }
    const audioEngine = getAudioEngine();
    if (audioEngine) {
      audioEngine.setEnvelopeStage(envType, lineNum, stage, rate, level);
    }
  };

  const sendEnvSustain = (typeKey, lineNum, point) => {
    const envType = ENV_TYPE_INDEX[typeKey];
    const backend = window.getJuceBackend ? window.getJuceBackend() : ((window.__JUCE__ && window.__JUCE__.backend) || (window.Juce && window.Juce.backend));
    if (backend) {
      if (typeof backend.setEnvelopeSustain === 'function') backend.setEnvelopeSustain(envType, lineNum, point);
      if (typeof backend.emitEvent === 'function') backend.emitEvent('setEnvelopeSustain', { envType, line: lineNum, point });
    }
    const audioEngine = getAudioEngine();
    if (audioEngine) {
      audioEngine.setEnvelopeSustain(envType, lineNum, point);
    }
  };

  const sendEnvEnd = (typeKey, lineNum, point) => {
    const envType = ENV_TYPE_INDEX[typeKey];
    const backend = window.getJuceBackend ? window.getJuceBackend() : ((window.__JUCE__ && window.__JUCE__.backend) || (window.Juce && window.Juce.backend));
    if (backend) {
      if (typeof backend.setEnvelopeEnd === 'function') backend.setEnvelopeEnd(envType, lineNum, point);
      if (typeof backend.emitEvent === 'function') backend.emitEvent('setEnvelopeEnd', { envType, line: lineNum, point });
    }
    const audioEngine = getAudioEngine();
    if (audioEngine) {
      audioEngine.setEnvelopeEnd(envType, lineNum, point);
    }
  };

  function setupCanvasInteractions(typeKey, lineNum) {
    const canvas = document.getElementById(`canvas-env-${typeKey}-line${lineNum}`);
    if (!canvas) return;

    let isDragging = false;

    const getStageFromX = (clientX) => {
      const rect = canvas.getBoundingClientRect();
      const x = clientX - rect.left;
      const env = envelopeState[typeKey][lineNum === 1 ? 'line1' : 'line2'];
      const xPositions = getNodeXPositions(env, rect.width);
      
      let closest = 0;
      let minDistance = Infinity;
      for (let i = 0; i < 8; i++) {
        let dist = Math.abs(x - xPositions[i]);
        if (dist < minDistance) {
          minDistance = dist;
          closest = i;
        }
      }
      return closest;
    };

    const handleDrag = (clientY) => {
      const rect = canvas.getBoundingClientRect();
      const y = clientY - rect.top;
      const level = 1 - Math.max(0, Math.min(1, (y - 5) / (rect.height - 10)));

      const lineKey = lineNum === 1 ? 'line1' : 'line2';
      const activeNode = selectedNode[typeKey][lineKey];

      envelopeState[typeKey][lineKey].levels[activeNode] = parseFloat(level.toFixed(2));

      const rate = envelopeState[typeKey][lineKey].rates[activeNode];
      sendEnvStage(typeKey, lineNum, activeNode, rate, parseFloat(level.toFixed(2)));

      updateEnvelopeControls(typeKey, lineNum);
      redrawEnvelopes();
    };

    canvas.addEventListener('mousedown', (e) => {
      // Envelope drags are custom controls (ids are not in PARAM_MAP), so the
      // global undo-arm listeners never fire for them. Arm here so one drag
      // session = one undo step, mirroring the registry controls.
      if (!canvas.dataset.undoArmed) {
        canvas.dataset.undoArmed = '1';
        pushUndo();
      }
      isDragging = true;
      const stage = getStageFromX(e.clientX);
      selectedNode[typeKey][lineNum === 1 ? 'line1' : 'line2'] = stage;
      handleDrag(e.clientY);
    });

    canvas.addEventListener('mousemove', (e) => {
      if (isDragging) {
        handleDrag(e.clientY);
      }
    });

    window.addEventListener('mouseup', () => {
      isDragging = false;
      // Re-arm so the next drag starts a fresh undo step.
      if (canvas.dataset.undoArmed) canvas.dataset.undoArmed = '';
    });
  }

  function setupEnvelopeControlsListeners(typeKey, lineNum) {
    const sliderRate = document.getElementById(`slider-rate-env-${typeKey}-line${lineNum}`);
    const sliderLevel = document.getElementById(`slider-level-env-${typeKey}-line${lineNum}`);
    const lineKey = lineNum === 1 ? 'line1' : 'line2';
    
    const armUndo = (el) => {
      // One scrub session = one undo step (arm on first input, re-arm on change).
      if (!el.dataset.undoArmed) {
        el.dataset.undoArmed = '1';
        pushUndo();
      }
    };

    if (sliderRate) {
      sliderRate.addEventListener('input', (e) => {
        armUndo(sliderRate);
        const val = parseFloat(e.target.value);
        const activeNode = selectedNode[typeKey][lineKey];
        
        envelopeState[typeKey][lineKey].rates[activeNode] = val;
        const level = envelopeState[typeKey][lineKey].levels[activeNode];
        
        sendEnvStage(typeKey, lineNum, activeNode, val, level);
        document.getElementById(`val-rate-env-${typeKey}-line${lineNum}`).innerText = val.toFixed(2);
        redrawEnvelopes();
      });
      sliderRate.addEventListener('change', () => { sliderRate.dataset.undoArmed = ''; });
    }

    if (sliderLevel) {
      sliderLevel.addEventListener('input', (e) => {
        armUndo(sliderLevel);
        const val = parseFloat(e.target.value);
        const activeNode = selectedNode[typeKey][lineKey];
        
        envelopeState[typeKey][lineKey].levels[activeNode] = val;
        const rate = envelopeState[typeKey][lineKey].rates[activeNode];
        
        sendEnvStage(typeKey, lineNum, activeNode, rate, val);
        document.getElementById(`val-level-env-${typeKey}-line${lineNum}`).innerText = val.toFixed(2);
        redrawEnvelopes();
      });
      sliderLevel.addEventListener('change', () => { sliderLevel.dataset.undoArmed = ''; });
    }

    const btnSus = document.getElementById(`btn-sus-env-${typeKey}-line${lineNum}`);
    const btnEnd = document.getElementById(`btn-end-env-${typeKey}-line${lineNum}`);

    if (btnSus) {
      btnSus.addEventListener('click', () => {
        pushUndo();
        const env = envelopeState[typeKey][lineKey];
        const activeNode = selectedNode[typeKey][lineKey];

        const newPoint = env.sustainPoint === activeNode ? -1 : activeNode;
        env.sustainPoint = newPoint;
        sendEnvSustain(typeKey, lineNum, newPoint);
        updateEnvelopeControls(typeKey, lineNum);
        redrawEnvelopes();
      });
    }

    if (btnEnd) {
      btnEnd.addEventListener('click', () => {
        pushUndo();
        const env = envelopeState[typeKey][lineKey];
        const activeNode = selectedNode[typeKey][lineKey];

        const newPoint = env.endPoint === activeNode ? -1 : activeNode;
        env.endPoint = newPoint;
        sendEnvEnd(typeKey, lineNum, newPoint);
        updateEnvelopeControls(typeKey, lineNum);
        redrawEnvelopes();
      });
    }

    const btnCopy = document.getElementById(`btn-copy-env-${typeKey}-line${lineNum}`);
    const btnPaste = document.getElementById(`btn-paste-env-${typeKey}-line${lineNum}`);

    if (btnCopy) {
      btnCopy.addEventListener('click', () => {
        const env = envelopeState[typeKey][lineKey];
        
        envClipboard = {
          rates: [...env.rates],
          levels: [...env.levels],
          sustainPoint: env.sustainPoint,
          endPoint: env.endPoint
        };
        
        lcdLine1.innerText = "ENV COPIED";
        lcdLine2.innerText = `LINE ${lineNum} SAVED`;
      });
    }

    if (btnPaste) {
      btnPaste.addEventListener('click', () => {
        if (!envClipboard) {
          lcdLine1.innerText = "PASTE ERROR";
          lcdLine2.innerText = "CLIPBOARD EMPTY";
          return;
        }
        const env = envelopeState[typeKey][lineKey];

        pushUndo();
        env.rates = [...envClipboard.rates];
        env.levels = [...envClipboard.levels];
        env.sustainPoint = envClipboard.sustainPoint;
        env.endPoint = envClipboard.endPoint;
        
        for (let i = 0; i < 8; i++) {
          sendEnvStage(typeKey, lineNum, i, env.rates[i], env.levels[i]);
        }
        sendEnvSustain(typeKey, lineNum, env.sustainPoint);
        sendEnvEnd(typeKey, lineNum, env.endPoint);
        
        updateEnvelopeControls(typeKey, lineNum);
        redrawEnvelopes();
        
        lcdLine1.innerText = "ENV PASTED";
        lcdLine2.innerText = `LINE ${lineNum} UPDATED`;
      });
    }
  }

  const copyEnvelopesToState = (envelopes) => {
    if (!envelopes) return;
    ['dca', 'dcw', 'pitch'].forEach(typeKey => {
      ['line1', 'line2'].forEach(lineKey => {
        if (envelopes[typeKey] && envelopes[typeKey][lineKey]) {
          const src = envelopes[typeKey][lineKey];
          const dest = envelopeState[typeKey][lineKey];
          dest.rates = [...src.rates];
          dest.levels = [...src.levels];
          dest.sustainPoint = src.sustainPoint;
          dest.endPoint = src.endPoint;
        }
      });
    });
    ENV_TYPES.forEach(typeKey => {
      updateEnvelopeControls(typeKey, 1);
      updateEnvelopeControls(typeKey, 2);
    });
    redrawEnvelopes();
  };

  // DCO Mini Waveform Renderer
  function drawMiniDcoWave(canvasId, waveVal1, waveVal2) {
    const canvas = document.getElementById(canvasId);
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    const w = canvas.width;
    const h = canvas.height;
    ctx.clearRect(0, 0, w, h);

    // Draw grid lines
    ctx.strokeStyle = 'rgba(232, 163, 61, 0.08)';
    ctx.lineWidth = 0.5;
    ctx.beginPath();
    ctx.moveTo(w / 2, 0); ctx.lineTo(w / 2, h);
    ctx.moveTo(0, h / 2); ctx.lineTo(w, h / 2);
    ctx.stroke();

    ctx.strokeStyle = '#e8a33d';
    ctx.lineWidth = 1.2;
    ctx.beginPath();

    function drawWaveformSegment(wave, startX, endX) {
      const midY = h / 2;
      const amp = h / 2 - 4;
      const len = endX - startX;
      
      if (wave === 0) { // Sawtooth
        ctx.moveTo(startX, midY + amp);
        ctx.lineTo(endX, midY - amp);
        ctx.lineTo(endX, midY + amp);
      } else if (wave === 1) { // Square
        ctx.moveTo(startX, midY + amp);
        ctx.lineTo(startX, midY - amp);
        ctx.lineTo(startX + len / 2, midY - amp);
        ctx.lineTo(startX + len / 2, midY + amp);
        ctx.lineTo(endX, midY + amp);
        ctx.lineTo(endX, midY - amp);
      } else if (wave === 2) { // Pulse
        ctx.moveTo(startX, midY + amp);
        ctx.lineTo(startX, midY - amp);
        ctx.lineTo(startX + len / 4, midY - amp);
        ctx.lineTo(startX + len / 4, midY + amp);
        ctx.lineTo(endX, midY + amp);
        ctx.lineTo(endX, midY - amp);
      } else if (wave === 3) { // Double Sine
        for (let x = startX; x <= endX; x++) {
          const t = (x - startX) / len;
          const y = midY - Math.sin(t * Math.PI * 4) * amp;
          if (x === startX) ctx.moveTo(x, y);
          else ctx.lineTo(x, y);
        }
      } else if (wave === 4) { // Saw-Pulse
        ctx.moveTo(startX, midY + amp);
        ctx.lineTo(startX + len / 2, midY - amp);
        ctx.lineTo(startX + len / 2, midY + amp);
        ctx.lineTo(startX + 3 * len / 4, midY + amp);
        ctx.lineTo(startX + 3 * len / 4, midY - amp);
        ctx.lineTo(endX, midY - amp);
      } else { // Resonance (5, 6, 7)
        for (let x = startX; x <= endX; x++) {
          const t = (x - startX) / len;
          const decay = Math.exp(-t * 2.2);
          const y = midY - Math.sin(t * Math.PI * 6.5) * amp * decay;
          if (x === startX) ctx.moveTo(x, y);
          else ctx.lineTo(x, y);
        }
      }
    }

    const w1 = parseInt(waveVal1 || 0);
    const w2 = parseInt(waveVal2 || 0);
    if (w2 > 0) {
      drawWaveformSegment(w1, 3, w / 2);
      drawWaveformSegment(w2 - 1, w / 2, w - 3); // waveVal2 choice offset
    } else {
      drawWaveformSegment(w1, 3, w - 3);
    }
    ctx.stroke();
  }

  // LFO Mini Waveform Renderer
  function drawMiniLfoWave(canvasId, waveVal) {
    const canvas = document.getElementById(canvasId);
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    const w = canvas.width;
    const h = canvas.height;
    ctx.clearRect(0, 0, w, h);

    ctx.strokeStyle = 'rgba(232, 163, 61, 0.08)';
    ctx.lineWidth = 0.5;
    ctx.beginPath();
    ctx.moveTo(w / 2, 0); ctx.lineTo(w / 2, h);
    ctx.moveTo(0, h / 2); ctx.lineTo(w, h / 2);
    ctx.stroke();

    ctx.strokeStyle = '#1c221e';
    ctx.lineWidth = 1.2;
    ctx.beginPath();

    const midY = h / 2;
    const amp = h / 2 - 4;
    const val = parseInt(waveVal || 0);

    if (val === 0) { // Triangle
      ctx.moveTo(3, midY);
      ctx.lineTo(w / 4, midY - amp);
      ctx.lineTo(3 * w / 4, midY + amp);
      ctx.lineTo(w - 3, midY);
    } else if (val === 1) { // Sawtooth Up
      ctx.moveTo(3, midY + amp);
      ctx.lineTo(w - 3, midY - amp);
      ctx.lineTo(w - 3, midY + amp);
    } else if (val === 2) { // Sawtooth Down
      ctx.moveTo(3, midY - amp);
      ctx.lineTo(w - 3, midY + amp);
      ctx.lineTo(w - 3, midY - amp);
    } else { // Square (val === 3)
      ctx.moveTo(3, midY + amp);
      ctx.lineTo(3, midY - amp);
      ctx.lineTo(w / 2, midY - amp);
      ctx.lineTo(w / 2, midY + amp);
      ctx.lineTo(w - 3, midY + amp);
      ctx.lineTo(w - 3, midY - amp);
    }
    ctx.stroke();
  }

  // Mini Envelope Renderer (superimposed Line 1 & Line 2)
  function drawMiniEnvelope(canvasId, typeKey) {
    const canvas = document.getElementById(canvasId);
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    const w = canvas.width;
    const h = canvas.height;
    ctx.clearRect(0, 0, w, h);

    ctx.strokeStyle = 'rgba(28, 34, 30, 0.15)';
    ctx.lineWidth = 0.5;
    ctx.beginPath();
    const stepWidth = w / 8;
    for (let i = 1; i < 8; i++) {
      const x = stepWidth * i;
      ctx.moveTo(x, 0); ctx.lineTo(x, h);
    }
    for (let y = h / 4; y < h; y += h / 4) {
      ctx.moveTo(0, y); ctx.lineTo(w, y);
    }
    ctx.stroke();

    // Draw Line 1 (solid)
    drawEnvLine(ctx, w, h, typeKey, 1, '#1c221e', false);
    // Draw Line 2 (dashed)
    drawEnvLine(ctx, w, h, typeKey, 2, 'rgba(28, 34, 30, 0.5)', true);
  }

  function drawEnvLine(ctx, w, h, typeKey, lineNum, color, isDashed) {
    const lineKey = lineNum === 1 ? 'line1' : 'line2';
    const env = envelopeState[typeKey][lineKey];
    if (!env || !env.levels) return;

    ctx.strokeStyle = color;
    ctx.lineWidth = 1.2;
    if (isDashed) {
      ctx.setLineDash([2, 1]);
    } else {
      ctx.setLineDash([]);
    }

    ctx.beginPath();
    ctx.moveTo(0, h - 2);
    const stepWidth = w / 8;
    for (let i = 0; i < 8; i++) {
      const x = (i + 1) * stepWidth;
      const y = h - (env.levels[i] * (h - 6) + 3);
      ctx.lineTo(x, y);
    }
    ctx.stroke();
    ctx.setLineDash([]);
  }

  // Master update caller for LCD mini matrix
  function updateMiniGraphics() {
    const osc1w1 = parseInt(document.getElementById('OSC1_WAVEFORM')?.value || '0', 10);
    const osc1w2 = parseInt(document.getElementById('OSC1_WAVEFORM2')?.value || '0', 10);
    const osc2w1 = parseInt(document.getElementById('OSC2_WAVEFORM')?.value || '0', 10);
    const osc2w2 = parseInt(document.getElementById('OSC2_WAVEFORM2')?.value || '0', 10);

    const applyWaveSprite = (boxId, w1, isW2) => {
      const box = document.getElementById(boxId);
      if (!box) return;
      const waveIdx = isW2 ? (w1 - 1) : w1;
      if (waveIdx < 0) {
        box.style.backgroundImage = 'none';
      } else {
        box.style.backgroundImage = 'url(src/assets/czwavs.png)';
        box.style.backgroundSize = '100% 800%';
        box.style.backgroundRepeat = 'no-repeat';
        const yPct = (waveIdx / 7) * 100;
        box.style.backgroundPosition = `0% ${yPct}%`;
      }
    };

    applyWaveSprite('mini-box-osc1-1', osc1w1, false);
    applyWaveSprite('mini-box-osc1-2', osc1w2, true);
    applyWaveSprite('mini-box-osc2-1', osc2w1, false);
    applyWaveSprite('mini-box-osc2-2', osc2w2, true);

    const lfoW = document.getElementById('LFO_WAVE')?.value || 0;
    // For LFO and envelopes we keep drawing them on their respective canvases
    // which are now inside the env-sprite boxes
    drawMiniLfoWave('mini-canvas-lfo', lfoW);

    drawMiniEnvelope('mini-canvas-dca', 'dca');
    drawMiniEnvelope('mini-canvas-dcw', 'dcw');
    drawMiniEnvelope('mini-canvas-pitch', 'pitch');
  }

  // ADSR macro sliders (DCA_*/DCW_*) refresh the 8-stage envelope graphs through
  // this alias. Previously it referenced a function that was never defined, which
  // threw and aborted parameter application for any DCA_/DCW_ change.
  function updateEnvelopeGraphs() {
    redrawEnvelopes();
  }

  // Wire up one envelope editor per type (DCA/DCW/Pitch) — canvases, sliders,
  // SUS/END and COPY/PASTE. Runs at startup so the graphs are visible as soon as
  // the drawer opens (the engine init re-calls it via updateEnvelopeGraphs).
  function setupEnvelopeEditors() {
    ENV_TYPES.forEach(typeKey => {
      setupCanvasInteractions(typeKey, 1);
      setupCanvasInteractions(typeKey, 2);
      setupEnvelopeControlsListeners(typeKey, 1);
      setupEnvelopeControlsListeners(typeKey, 2);
      updateEnvelopeControls(typeKey, 1);
      updateEnvelopeControls(typeKey, 2);
    });
    redrawEnvelopes();
    updateMiniGraphics();
  }

  return {
    ENV_TYPES,
    ENV_TYPE_INDEX,
    ENV_ACCENTS,
    envelopeState,
    redrawEnvelopes,
    updateEnvelopeControls,
    copyEnvelopesToState,
    updateMiniGraphics,
    updateEnvelopeGraphs,
    setupEnvelopeEditors
  };
}
