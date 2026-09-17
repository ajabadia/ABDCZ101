import { describe, it, expect } from 'vitest';
import CZ101DSP from '../wasm/cz101_dsp.js';

// Drives the REAL compiled WASM engine. KEY_TRANSPOSE shifts the input note
// before the voice manager. The cleanest assertion: a note played WITH
// transpose must sound identical to the target note played WITHOUT it.
//   - wasm_note_on applies gKeyTranspose (web keyboard path)
//   - MIDIProcessor::handleNoteOn applies keyTranspose (incoming MIDI path)

function createEngine() {
  let dsp = null;
  const alloc = (s) => { const p = dsp._malloc(s.length + 1); for (let i = 0; i < s.length; i++) dsp.HEAPU8[p + i] = s.charCodeAt(i); dsp.HEAPU8[p + s.length] = 0; return p; };
  return {
    async init() { dsp = await CZ101DSP(); dsp._wasm_init(44100); return this; },
    setParam(id, norm) { const p = alloc(id); dsp._wasm_set_param(p, norm); dsp._free(p); },
    noteOn(note, vel = 0.8) { dsp._wasm_note_on(note, vel); },
    noteOff(note) { dsp._wasm_note_off(note); },
    midiMsg(bytes) {
      const p = dsp._malloc(bytes.length);
      dsp.HEAPU8.set(bytes, p);
      dsp._wasm_midi_message(p, bytes.length);
      dsp._free(p);
    },
    render(seconds = 0.8) {
      const n = Math.floor(44100 * seconds);
      const l = dsp._malloc(n * 4), r = dsp._malloc(n * 4);
      dsp._wasm_process(l, r, n);
      const out = new Float32Array(n);
      out.set(dsp.HEAPF32.subarray(l / 4, l / 4 + n));
      dsp._free(l); dsp._free(r);
      return out;
    },
  };
}

// Max |diff| over the steady-state window (skip attack/decay tails).
function steadyMaxDiff(a, b, sr = 44100) {
  const start = Math.floor(sr * 0.2), end = Math.floor(sr * 0.7);
  let md = 0;
  for (let i = start; i < end; i++) {
    const d = Math.abs(a[i] - b[i]);
    if (d > md) md = d;
  }
  return md;
}

describe('KEY_TRANSPOSE (native "TRANSPOSE" system param)', () => {
  it('web keyboard: C4 with +12 transpose sounds identical to plain C5', async () => {
    const e = await createEngine().init();

    // Plain C5 (note 72)
    e.noteOn(72, 0.8);
    const plainC5 = e.render();
    e.noteOff(72);

    const e2 = await createEngine().init();
    // C4 (note 60) with KEY_TRANSPOSE = raw +12 → normalized (12+12)/24 = 1.0
    e2.setParam('KEY_TRANSPOSE', 1.0);
    e2.noteOn(60, 0.8);
    const transposed = e2.render();
    e2.noteOff(60);

    const md = steadyMaxDiff(plainC5, transposed);
    // Same note → near-identical output (float-level differences only).
    expect(md).toBeLessThan(1e-3);
  });

  it('web keyboard: transpose 0 leaves the note unchanged', async () => {
    const e = await createEngine().init();
    e.setParam('KEY_TRANSPOSE', 0.5); // raw 0 (center of -12..12)

    e.noteOn(60, 0.8);
    const withZero = e.render();
    e.noteOff(60);

    // Reference: fresh engine, no transpose param touched (default 0).
    const e2 = await createEngine().init();
    e2.noteOn(60, 0.8);
    const plain = e2.render();
    e2.noteOff(60);

    expect(steadyMaxDiff(plain, withZero)).toBeLessThan(1e-3);
  });

  it('incoming MIDI: C4 +6 transpose matches plain F#4 (note 66)', async () => {
    const e = await createEngine().init();
    // raw +6 → normalized (6+12)/24 = 0.75
    e.setParam('KEY_TRANSPOSE', 0.75);

    e.midiMsg([0x90, 60, 100]); // C4 with transpose → F#4
    const midiTransposed = e.render();
    e.midiMsg([0x80, 60, 0]);

    const e2 = await createEngine().init();
    e2.midiMsg([0x90, 66, 100]); // plain F#4
    const plainFsharp4 = e2.render();
    e2.midiMsg([0x80, 66, 0]);

    expect(steadyMaxDiff(plainFsharp4, midiTransposed)).toBeLessThan(1e-3);
  });
});
