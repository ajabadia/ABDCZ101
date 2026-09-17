import { describe, it, expect, beforeEach } from 'vitest';
import CZ101DSP from '../wasm/cz101_dsp.js';
import { pushScopeSamples, SCOPE_RING_SIZE, ringIndexForX } from '../src/contracts/scopeRing.js';

// Proves the oscilloscope is fed with REAL engine audio — not a decorative
// pattern. The worklet does exactly this: after _wasm_process fills the L
// output buffer, pushScopeSamples copies those samples into the 256-sample
// ring and posts AUDIO_SCOPE to the main thread. Here we replicate that path
// against the real WASM and assert the ring actually moves with the note.

describe('oscilloscope receives real WASM audio (parity with WaveformDisplay::pushBuffer)', () => {
  let dsp;

  const allocateString = (str) => {
    const ptr = dsp._malloc(str.length + 1);
    for (let i = 0; i < str.length; i++) dsp.HEAPU8[ptr + i] = str.charCodeAt(i);
    dsp.HEAPU8[ptr + str.length] = 0;
    return ptr;
  };

  const setParam = (paramId, normalizedValue) => {
    const ptr = allocateString(paramId);
    dsp._wasm_set_param(ptr, normalizedValue);
    dsp._free(ptr);
  };

  const allocFloat = (n) => {
    const ptr = dsp._malloc(n * 4);
    return { ptr, view: new Float32Array(dsp.HEAPF32.buffer, ptr, n) };
  };

  const freeFloat = (p) => dsp._free(p.ptr);

  // Mirror of the worklet: process blocks of 128 and feed the L channel into
  // the scope ring, exactly like the AUDIO_SCOPE path.
  const feedScope = (blocks, ring) => {
    const n = 128;
    const L = allocFloat(n);
    const R = allocFloat(n);
    let writePos = 0;
    let ringBuf = new Float32Array(SCOPE_RING_SIZE);
    for (let b = 0; b < blocks; b++) {
      dsp._wasm_process(L.ptr, R.ptr, n);
      const pushed = pushScopeSamples(ringBuf, L.view, writePos);
      ringBuf = pushed.ring;
      writePos = pushed.writePos;
    }
    freeFloat(L); freeFloat(R);
    return ringBuf;
  };

  const rms = (samples) => {
    let sum = 0;
    for (const s of samples) sum += s * s;
    return Math.sqrt(sum / samples.length);
  };

  beforeEach(async () => {
    dsp = await CZ101DSP();
    dsp._wasm_init(44100.0);
  });

  it('ring receives non-zero samples while a note is held (real audio, not zeros)', () => {
    dsp._wasm_note_on(60, 0.9);
    const ring = feedScope(30, SCOPE_RING_SIZE);
    dsp._wasm_note_off(60);
    expect(rms(ring)).toBeGreaterThan(0.01);
  });

  it('ring is silent (flat line) when no note is playing — real silence, not a fake wave', () => {
    const ring = feedScope(30, SCOPE_RING_SIZE);
    // Offline / silent: the native waveformData is zeros until pushBuffer is
    // fed with actual audio, so the scope must draw a flat center line.
    expect(rms(ring)).toBeLessThan(0.001);
  });

  it('a playing note visibly changes the scope compared to silence', () => {
    const silent = feedScope(10, SCOPE_RING_SIZE);
    dsp._wasm_note_on(72, 0.9);
    const playing = feedScope(10, SCOPE_RING_SIZE);
    dsp._wasm_note_off(72);

    const diff = Math.abs(rms(playing) - rms(silent));
    expect(diff).toBeGreaterThan(0.01);
  });

  it('ring is 256 samples and x-mapping stays in range (native waveformData size)', () => {
    dsp._wasm_note_on(60, 0.9);
    const ring = feedScope(10, SCOPE_RING_SIZE);
    dsp._wasm_note_off(60);
    expect(ring.length).toBe(256);
    for (let x = 0; x <= 200; x++) {
      const idx = ringIndexForX(x, 200, ring.length);
      expect(idx).toBeGreaterThanOrEqual(0);
      expect(idx).toBeLessThan(ring.length);
    }
  });

  it('high notes vs low notes produce visibly different scope patterns', () => {
    dsp._wasm_note_on(36, 0.9); // low C2
    const low = feedScope(20, SCOPE_RING_SIZE);
    dsp._wasm_note_off(36);
    dsp._wasm_note_on(108, 0.9); // high C8
    const high = feedScope(20, SCOPE_RING_SIZE);
    dsp._wasm_note_off(108);

    // Count sign changes (zero crossings) — a high note must cross more often.
    const crossings = (samples) => {
      let c = 0;
      for (let i = 1; i < samples.length; i++) {
        if ((samples[i - 1] < 0 && samples[i] >= 0) || (samples[i - 1] >= 0 && samples[i] < 0)) c++;
      }
      return c;
    };
    expect(crossings(high)).toBeGreaterThan(crossings(low));
  });
});
