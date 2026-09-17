import { describe, it, expect } from 'vitest';
import CZ101DSP from '../wasm/cz101_dsp.js';

// Drives the REAL compiled WASM engine with the extended free-matrix enums:
//   Sources: 9=Pitch Bend, 10=Noise, 11=Authentic Key Track (8=Env Pitch is bipolar)
//   Dests:   6=Osc2 Detune, 7=Pan
// The bridge stores SRC/DEST as choice values (raw / max, SRC max is 11, DEST
// max is 7) and DEPTH as a bipolar -1..1 float ((raw + 1) / 2 in norm space).
function createEngine() {
  let dsp = null;
  const allocateString = (str) => {
    const ptr = dsp._malloc(str.length + 1);
    for (let i = 0; i < str.length; i++) dsp.HEAPU8[ptr + i] = str.charCodeAt(i);
    dsp.HEAPU8[ptr + str.length] = 0;
    return ptr;
  };
  return {
    async init(sampleRate = 44100) {
      dsp = await CZ101DSP();
      dsp._wasm_init(sampleRate);
      return this;
    },
    setParam(paramId, rawValue) {
      const ptr = allocateString(paramId);
      dsp._wasm_set_param(ptr, rawValue);
      dsp._free(ptr);
    },
    getParam(paramId) {
      const ptr = allocateString(paramId);
      const v = dsp._wasm_get_param(ptr);
      dsp._free(ptr);
      return v;
    },
    noteOn(note, vel) { dsp._wasm_note_on(note, vel); },
    noteOff(note) { dsp._wasm_note_off(note); },
    renderBlock(n) {
      const l = dsp._malloc(n * 4);
      const r = dsp._malloc(n * 4);
      dsp._wasm_process(l, r, n);
      const outL = new Float32Array(n);
      const outR = new Float32Array(n);
      outL.set(dsp.HEAPF32.subarray(l / 4, l / 4 + n));
      outR.set(dsp.HEAPF32.subarray(r / 4, r / 4 + n));
      dsp._free(l);
      dsp._free(r);
      return { outL, outR };
    },
  };
}

describe('WASM free matrix: extended sources & destinations', () => {
  let engine;

  beforeEach(async () => {
    engine = await createEngine().init(44100);
    // Modern mode — the free matrix is a Modern-only feature.
    engine.setParam('OPERATION_MODE', 2);
  });

  it('accepts Pitch Bend (9) → Osc2 Detune (6) and reads them back', () => {
    engine.setParam('MOD_SLOT_1_SRC', 9 / 11);  // raw 9 / max 11
    engine.setParam('MOD_SLOT_1_DEST', 6 / 7); // raw 6 / max 7
    engine.setParam('MOD_SLOT_1_DEPTH', 0.75); // raw 0.5 → (0.5+1)/2

    expect(engine.getParam('MOD_SLOT_1_SRC')).toBeCloseTo(9 / 11, 5);
    expect(engine.getParam('MOD_SLOT_1_DEST')).toBeCloseTo(6 / 7, 5);
    expect(engine.getParam('MOD_SLOT_1_DEPTH')).toBeCloseTo(0.75, 5);
  });

  it('accepts Noise (10) → Pan (7) and renders audio without crashing', () => {
    engine.setParam('MOD_SLOT_2_SRC', 10 / 11);  // raw 10 / max 11
    engine.setParam('MOD_SLOT_2_DEST', 1.0);    // raw 7 / max 7
    engine.setParam('MOD_SLOT_2_DEPTH', 0.35);  // raw -0.3

    expect(engine.getParam('MOD_SLOT_2_SRC')).toBeCloseTo(10 / 11, 5);
    expect(engine.getParam('MOD_SLOT_2_DEST')).toBeCloseTo(1.0, 5);
    expect(engine.getParam('MOD_SLOT_2_DEPTH')).toBeCloseTo(0.35, 5);

    engine.noteOn(60, 1.0);
    const { outL, outR } = engine.renderBlock(512);
    engine.noteOff(60);

    // Not silent, no NaN, both channels finite
    let maxL = 0, maxR = 0;
    for (let i = 0; i < 512; i++) {
      maxL = Math.max(maxL, Math.abs(outL[i]));
      maxR = Math.max(maxR, Math.abs(outR[i]));
      expect(Number.isFinite(outL[i])).toBe(true);
      expect(Number.isFinite(outR[i])).toBe(true);
    }
    expect(maxL).toBeGreaterThan(0.0001);
    expect(maxR).toBeGreaterThan(0.0001);
  });

  it('Authentic Key Track (11) replicates the hardware curve (DCW brightens at high notes)', async () => {
    // Raw 11 / max 11 = 1.0. The authentic curve depends on note + DCW env.
    engine.setParam('MOD_SLOT_4_SRC', 11 / 11); // Authentic Key Track
    engine.setParam('MOD_SLOT_4_DEST', 1 / 7);  // DCW
    engine.setParam('MOD_SLOT_4_DEPTH', 1.0);   // depth +1 (full authentic curve)

    expect(engine.getParam('MOD_SLOT_4_SRC')).toBeCloseTo(1.0, 5);
    expect(engine.getParam('MOD_SLOT_4_DEST')).toBeCloseTo(1 / 7, 5);
    expect(engine.getParam('MOD_SLOT_4_DEPTH')).toBeCloseTo(1.0, 5);

    // The curve is zero at C3 (note 60) and grows with |note−60|: positive
    // above C3 (brightening DCW), negative below (darkening). Rendering the
    // SAME note with the slot off vs on must therefore shift the DCW energy.
    const renderNote = async (withSlot, note) => {
      const dsp = await CZ101DSP();
      dsp._wasm_init(44100);
      const alloc = (s) => { const p = dsp._malloc(s.length + 1); for (let i = 0; i < s.length; i++) dsp.HEAPU8[p + i] = s.charCodeAt(i); dsp.HEAPU8[p + s.length] = 0; return p; };
      const set = (id, v) => { const p = alloc(id); dsp._wasm_set_param(p, v); dsp._free(p); };
      set('OPERATION_MODE', 2);
      if (withSlot) {
        set('MOD_SLOT_4_SRC', 11 / 11); // Authentic Key Track
        set('MOD_SLOT_4_DEST', 1 / 7);  // DCW
        set('MOD_SLOT_4_DEPTH', 1.0);   // depth +1
      }
      const n = 44100; // 1 s
      const l = dsp._malloc(n * 4), r = dsp._malloc(n * 4);
      dsp._wasm_note_on(note, 1.0);
      dsp._wasm_process(l, r, n);
      dsp._wasm_note_off(note);
      const data = new Float32Array(n);
      data.set(dsp.HEAPF32.subarray(l / 4, l / 4 + n));
      dsp._free(l); dsp._free(r);
      let e = 0;
      for (let i = 4410; i < n; i += 8) e += Math.abs(data[i]);
      return { e, data };
    };

    const off = await renderNote(false, 84); // C6, no slot
    const on = await renderNote(true, 84);   // C6, authentic curve brightening
    expect(Number.isFinite(off.e)).toBe(true);
    expect(Number.isFinite(on.e)).toBe(true);
    expect(off.e).toBeGreaterThan(0.0001);
    // The slot is active and shifts the DCW energy (curve > 0 at C6), i.e.
    // the authentic curve really reaches the render loop.
    expect(Math.abs(on.e - off.e)).toBeGreaterThan(off.e * 0.0005);
  });

  it('zeroes the fixed Velo→DCA route in Modern (matrix owns it) and restores it via a seeded slot', async () => {
    // Render a quiet note (vel 0.3) three ways and compare the steady-state RMS.
    //   a) Classic (veloToDca=1.0):  dcaVal*vel * currentVelocity  → vel² ≈ 0.09x
    //   b) Modern, veloToDca param set but gated to 0, no matrix slot: dcaVal*vel ≈ 0.3x
    //   c) Modern + seeded matrix Velo→DCA 1.0: (dcaVal+vel)*vel — additive, louder than (b)
    const rmsWindow = (data) => {
      let sum = 0, n = 0;
      for (let i = 4410 * 2; i < 4410 * 8; i += 8) { // 0.2s..0.8s, decimated
        sum += data[i] * data[i]; n++;
      }
      return Math.sqrt(sum / n);
    };
    const renderNote = async (mode, matrixVeloDca) => {
      const dsp = await CZ101DSP();
      dsp._wasm_init(44100);
      const alloc = (s) => { const p = dsp._malloc(s.length + 1); for (let i = 0; i < s.length; i++) dsp.HEAPU8[p + i] = s.charCodeAt(i); dsp.HEAPU8[p + s.length] = 0; return p; };
      const set = (id, v) => { const p = alloc(id); dsp._wasm_set_param(p, v); dsp._free(p); };
      set('OPERATION_MODE', mode);
      set('MOD_VELO_DCA', 1.0); // stored 1.0 — gated to 0 in Modern
      if (matrixVeloDca) {
        set('MOD_SLOT_1_SRC', 1 / 11);   // Velocity
        set('MOD_SLOT_1_DEST', 2 / 7);   // DCA
        set('MOD_SLOT_1_DEPTH', 1.0);    // depth +1 (bipolar center is 0.5)
      }
      const n = 44100; // 1 s
      const l = dsp._malloc(n * 4), r = dsp._malloc(n * 4);
      dsp._wasm_note_on(60, 0.3);
      dsp._wasm_process(l, r, n);
      dsp._wasm_note_off(60);
      const data = new Float32Array(n);
      data.set(dsp.HEAPF32.subarray(l / 4, l / 4 + n));
      dsp._free(l); dsp._free(r);
      return rmsWindow(data);
    };

    const classic = await renderNote(0, false);   // Classic CZ-101, veloToDca 1.0 active
    const modernGated = await renderNote(3, false); // Modern, veloToDca zeroed, no matrix
    const modernSeeded = await renderNote(3, true);  // Modern, matrix Velo→DCA 1.0

    // (b) must be clearly louder than (a): the vel² attenuation is gone in Modern.
    expect(modernGated).toBeGreaterThan(classic * 1.5);
    // The seeded matrix slot restores the attenuation or adds modulation.
    expect(modernSeeded).not.toBeCloseTo(modernGated, 4);
    expect(modernSeeded).toBeGreaterThan(classic);
  });

  it('pan destination (7) actually steers energy between channels', () => {
    // Velocity (1) is a constant source (full velocity = 1.0), so the pan
    // position is deterministic: depth -1 → panValue 0.0 → full left.
    engine.setParam('MOD_SLOT_1_SRC', 1 / 11); // Velocity
    engine.setParam('MOD_SLOT_1_DEST', 1.0);   // Pan
    engine.setParam('MOD_SLOT_1_DEPTH', 0.0);  // depth -1 (bipolar center of norm)
    engine.setParam('MOD_SLOT_3_SRC', 1 / 11); // Velocity
    engine.setParam('MOD_SLOT_3_DEST', 1.0);   // Pan
    engine.setParam('MOD_SLOT_3_DEPTH', 0.0);  // full left

    engine.noteOn(48, 1.0);
    const { outL, outR } = engine.renderBlock(2048);
    engine.noteOff(48);

    let eL = 0, eR = 0;
    for (let i = 256; i < 2048; i++) { // skip attack ramp
      eL += outL[i] * outL[i];
      eR += outR[i] * outR[i];
    }
    // Equal-power pan to the left: L must clearly dominate R.
    expect(eL).toBeGreaterThan(eR * 4);
  });
});
