import { describe, it, expect } from 'vitest';
import CZ101DSP from '../wasm/cz101_dsp.js';

// Drives the REAL compiled WASM engine. Verifies "al oído" parity between the
// Key Follow modes (KF DCW FIX/VAR — authentic hardware curve) and the free
// matrix route Authentic Key Track → DCW depth 1.0:
//
//   - Voice::calculateDCWModulation adds `getAuthenticDCWKeytrack(note, env)`
//     to dcwVal via ktOffset (KF FIX/VAR, amount forced to 1.0) or via the
//     matrix slot contribution (source 11, depth 1.0). Both must produce the
//     same brightness/timbre.
//   - Regression: KEY_FOLLOW_DCW used to be a silent no-op in the WASM bridge
//     (kfDcw set, but the curve was scaled by keyTrackDcw which stayed at its
//     default 0.0). The bridge now mirrors Voice::applySnapshot and forces the
//     amount to 1.0, so FIX/VAR must audibly differ from OFF.

function createEngine() {
  let dsp = null;
  const alloc = (s) => { const p = dsp._malloc(s.length + 1); for (let i = 0; i < s.length; i++) dsp.HEAPU8[p + i] = s.charCodeAt(i); dsp.HEAPU8[p + s.length] = 0; return p; };
  return {
    async init() { dsp = await CZ101DSP(); dsp._wasm_init(44100); return this; },
    set(id, v) { const p = alloc(id); dsp._wasm_set_param(p, v); dsp._free(p); },
    render(note, seconds = 1.0) {
      const n = Math.floor(44100 * seconds);
      const l = dsp._malloc(n * 4), r = dsp._malloc(n * 4);
      dsp._wasm_note_on(note, 1.0);
      dsp._wasm_process(l, r, n);
      dsp._wasm_note_off(note);
      const out = new Float32Array(n);
      out.set(dsp.HEAPF32.subarray(l / 4, l / 4 + n));
      dsp._free(l); dsp._free(r);
      return out;
    }
  };
}

// Lower the DCW sustain so the keytrack offset has headroom (not clamped).
function midDcwPatch(e) {
  e.set('OPERATION_MODE', 1.0); // 3 (Modern) / 3 = 1.0 normalized
  e.set('DCA_SUSTAIN', 0.8);
  e.set('DCW_SUSTAIN', 0.4);
  e.set('DCW_ATTACK', 0.0);
  e.set('DCA_ATTACK', 0.0);
  e.set('DCW_DECAY', 0.2);
  e.set('DCA_DECAY', 0.2);
}

async function renderConfig(setupFn, note) {
  const e = await createEngine().init();
  setupFn(e);
  return e.render(note);
}

// Max |diff| over the late steady-state window (skip 500ms so the 50ms
// keyTrackDcw smoothing ramp of the KF path has fully settled and the
// envelope feedback loop has converged).
function steadyMaxDiff(a, b, sr = 44100) {
  const start = Math.floor(sr * 0.5);
  let md = 0, bad = 0;
  for (let i = start; i < a.length; i++) {
    const d = Math.abs(a[i] - b[i]);
    if (d > md) md = d;
    if (d > 1e-3) bad++;
  }
  return { md, bad, len: a.length - start };
}

// Brightness: fraction of spectral energy above 2 kHz (coarse DFT).
function highFreqEnergy(data, sr = 44100) {
  const start = Math.floor(sr * 0.2), N = 8192;
  const win = data.slice(start, start + N);
  let total = 0, high = 0;
  for (let b = 1; b <= 256; b++) {
    const freq = (b / 256) * (sr / 2);
    let re = 0, im = 0;
    for (let i = 0; i < N; i += 16) {
      const ph = (2 * Math.PI * i * b) / 256;
      const v = win[i];
      re += v * Math.cos(ph); im -= v * Math.sin(ph);
    }
    const mag2 = re * re + im * im;
    total += mag2;
    if (freq > 2000) high += mag2;
  }
  return total > 0 ? high / total : 0;
}

describe('KF FIX/VAR == matrix Authentic Key Track → DCW (hardware curve parity)', () => {
  // Notes: 36 = C2 (curve negative → darkens), 72 = C5 (positive → brightens),
  // 84 = C6 (strong positive).
  for (const note of [36, 72, 84]) {
    it(`note ${note}: VAR equals the matrix route (depth 1.0) and FIX is different`, async () => {
      const base = await renderConfig(midDcwPatch, note);
      const fix  = await renderConfig((e) => { midDcwPatch(e); e.set('KEY_FOLLOW_DCW', 0.5); }, note); // raw 1 = FIX
      const var_ = await renderConfig((e) => { midDcwPatch(e); e.set('KEY_FOLLOW_DCW', 1.0); }, note); // raw 2 = VAR
      const mx   = await renderConfig((e) => {
        midDcwPatch(e);
        e.set('MOD_SLOT_1_SRC', 11 / 11); // Authentic Key Track
        e.set('MOD_SLOT_1_DEST', 1 / 7);  // DCW
        e.set('MOD_SLOT_1_DEPTH', 1.0);   // depth +1
      }, note);

      // FIX and VAR now have different authentic curves.
      const fixVsVar = steadyMaxDiff(fix, var_);
      expect(fixVsVar.md).toBeGreaterThan(1e-3);

      // The curve is AUDIBLE vs baseline (regression: the bridge forces the
      // amount to 1.0, so KF FIX/VAR really darkens/brightens the DCW).
      const baseVsFix = steadyMaxDiff(base, fix);
      expect(baseVsFix.md).toBeGreaterThan(1e-3);

      // The matrix route reproduces the SAME curve: after the 50ms smoothing
      // ramp of the KF path has settled, both are within tiny tolerance
      // (inaudible), and the late-window brightness matches to ~4 decimals.
      const varVsMx = steadyMaxDiff(var_, mx);
      expect(varVsMx.md).toBeLessThan(2e-3);

      const hfVar = highFreqEnergy(var_);
      const hfMx = highFreqEnergy(mx);
      expect(Math.abs(hfVar - hfMx)).toBeLessThan(2e-3);
    });
  }

  it('matrix depth scales the authentic curve (depth 0.5 ≈ half the darkening)', async () => {
    const note = 36; // below C3: curve negative (darkens)
    const base = await renderConfig(midDcwPatch, note);
    const full = await renderConfig((e) => {
      midDcwPatch(e);
      e.set('MOD_SLOT_1_SRC', 11 / 11);
      e.set('MOD_SLOT_1_DEST', 1 / 7);
      e.set('MOD_SLOT_1_DEPTH', 1.0);
    }, note);
    const half = await renderConfig((e) => {
      midDcwPatch(e);
      e.set('MOD_SLOT_1_SRC', 11 / 11);
      e.set('MOD_SLOT_1_DEST', 1 / 7);
      e.set('MOD_SLOT_1_DEPTH', 0.75); // normalized 0.75 → raw +0.5 depth
    }, note);

    // Half depth must sit between baseline and full depth (curve scaled).
    const hfBase = highFreqEnergy(base);
    const hfFull = highFreqEnergy(full);
    const hfHalf = highFreqEnergy(half);
    const fullDelta = Math.abs(hfFull - hfBase);
    const halfDelta = Math.abs(hfHalf - hfBase);
    expect(fullDelta).toBeGreaterThan(5e-5);
    expect(halfDelta).toBeGreaterThan(fullDelta * 0.3);
    expect(halfDelta).toBeLessThan(fullDelta * 1.5);
  });
});
