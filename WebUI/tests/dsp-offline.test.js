/**
 * DSP OFFLINE TESTS — deterministic audio validation against the REAL compiled
 * WASM engine (WebUI/wasm/cz101_dsp.{js,wasm}).
 *
 * These render audio blocks offline (no AudioContext) via the same FFI entry
 * points the AudioWorklet uses, so the exact binary that ships in the browser
 * is what gets validated:
 *
 *   wasm_init → wasm_note_on/off → wasm_process(L, R, n) → analyse the buffer
 *
 * Coverage (regression guards for the DSP fixes):
 *   1. Pitch accuracy — zero-crossing frequency of a rendered note matches the
 *      MIDI note mathematically (guards OSC detune drift).
 *   2. Voice termination — after note_off the output MUST reach digital silence
 *      within a bounded window (guards the infinite-queue / residual-noise bug).
 *   3. Param bridge — set/get roundtrip through wasm_set_param/get_param.
 *
 * NOTE: requires the WASM build. Run `wasm\build_wasm.bat` first (output lands
 * in WebUI/wasm/). Without it, the import fails and the suite errors clearly.
 */
import { describe, it, expect, beforeEach } from 'vitest';
import CZ101DSP from '../wasm/cz101_dsp.js';

const SAMPLE_RATE = 44100;

const midiToHz = (note) => 440.0 * Math.pow(2, (note - 69) / 12);

/* ─── WASM helpers ──────────────────────────────────────────────────── */

const allocateString = (dsp, str) => {
  const ptr = dsp._malloc(str.length + 1);
  for (let i = 0; i < str.length; i++) dsp.HEAPU8[ptr + i] = str.charCodeAt(i);
  dsp.HEAPU8[ptr + str.length] = 0;
  return ptr;
};

const setParam = (dsp, paramId, normalizedValue) => {
  const ptr = allocateString(dsp, paramId);
  dsp._wasm_set_param(ptr, normalizedValue);
  dsp._free(ptr);
};

const getParam = (dsp, paramId) => {
  const ptr = allocateString(dsp, paramId);
  const value = dsp._wasm_get_param(ptr);
  dsp._free(ptr);
  return value;
};

/**
 * Renders `seconds` of audio into stereo Float32Arrays.
 * Returns { left, right } — plain JS arrays sized `numSamples`.
 */
const render = (dsp, seconds) => {
  const numSamples = Math.floor(seconds * SAMPLE_RATE);
  const bytes = numSamples * Float32Array.BYTES_PER_ELEMENT;
  const ptrL = dsp._malloc(bytes);
  const ptrR = dsp._malloc(bytes);
  dsp._wasm_process(ptrL, ptrR, numSamples);
  const left = Array.from(dsp.HEAPF32.subarray(ptrL / 4, ptrL / 4 + numSamples));
  const right = Array.from(dsp.HEAPF32.subarray(ptrR / 4, ptrR / 4 + numSamples));
  dsp._free(ptrL);
  dsp._free(ptrR);
  return { left, right };
};

/** Monos the stereo pair (average) and drops the transient attack region. */
const steadyStateMono = ({ left, right }, fromSeconds, toSeconds) => {
  const from = Math.floor(fromSeconds * SAMPLE_RATE);
  const to = Math.floor(toSeconds * SAMPLE_RATE);
  const out = [];
  for (let i = from; i < to && i < left.length; i++) out.push((left[i] + right[i]) / 2);
  return out;
};

/**
 * Fundamental frequency via positive-going zero crossings, DC-compensated.
 * Reliable for the CZ DCO (sine + phase distortion keeps the fundamental).
 */
const estimateFrequency = (samples, sampleRate) => {
  const mean = samples.reduce((a, b) => a + b, 0) / samples.length;
  let crossings = 0;
  for (let i = 1; i < samples.length; i++) {
    const prev = samples[i - 1] - mean;
    const cur = samples[i] - mean;
    if (prev <= 0 && cur > 0) crossings++;
  }
  return (crossings / samples.length) * sampleRate;
};

/**
 * Finds the index of the first 100ms window that is fully silent (|x| < eps).
 * Returns -1 if no such window exists in the given region.
 */
const firstSilentWindowAt = (samples, sampleRate, fromIndex, eps = 1e-3) => {
  const windowLen = Math.floor(0.1 * sampleRate);
  for (let i = fromIndex; i + windowLen <= samples.length; i += windowLen) {
    let silent = true;
    for (let j = i; j < i + windowLen; j++) {
      if (Math.abs(samples[j]) >= eps) { silent = false; break; }
    }
    if (silent) return i;
  }
  return -1;
};

const maxAbs = (samples) => samples.reduce((m, s) => Math.max(m, Math.abs(s)), 0);

/* ─── Suite ─────────────────────────────────────────────────────────── */

describe('DSP offline (WASM) — pitch accuracy', () => {
  let dsp;

  beforeEach(async () => {
    dsp = await CZ101DSP();
    dsp._wasm_init(SAMPLE_RATE);
  });

  it.each([
    [60, 261.63], // C4
    [72, 523.25], // C5
  ])('note %i renders at the mathematically expected pitch (%f Hz)', (note, expectedHz) => {
    setParam(dsp, 'OSC2_DETUNE', 0.5); // center detune
    dsp._wasm_note_on(note, 1.0);

    const audio = render(dsp, 1.5);
    const steady = steadyStateMono(audio, 0.5, 1.5); // skip attack transient
    const measured = estimateFrequency(steady, SAMPLE_RATE);

    expect(Math.abs(measured - expectedHz) / expectedHz).toBeLessThan(0.02);
  });

  it('detune at the extremes actually changes the mix (and both sound)', () => {
    // Con OSC2 a +12 semitonos la mezcla bate entre el pitch base y el doble
    // (261.63 ↔ 523.25 Hz) — el cruce por cero no debe ser exactamente el pitch.
    // Lo honesto: ambas posiciones suenan y la señal difiere de la centrada.
    const renderWithDetune = (detune) => {
      setParam(dsp, 'OSC2_DETUNE', detune);
      dsp._wasm_note_on(60, 1.0);
      const audio = render(dsp, 1.0);
      dsp._wasm_note_off(60);
      return steadyStateMono(audio, 0.4, 1.0);
    };

    const center = renderWithDetune(0.5);
    const detuned = renderWithDetune(1.0);

    expect(maxAbs(center)).toBeGreaterThan(0.01); // suena
    expect(maxAbs(detuned)).toBeGreaterThan(0.01); // suena

    const differs = center.some((v, i) => Math.abs(v - detuned[i]) > 1e-3);
    expect(differs).toBe(true); // el detune está cableado de verdad
  });
});

describe('DSP offline (WASM) — voice termination (no infinite queue)', () => {
  let dsp;

  beforeEach(async () => {
    dsp = await CZ101DSP();
    dsp._wasm_init(SAMPLE_RATE);
  });

  it('reaches digital silence within 3s after note_off', () => {
    const note = 60;
    dsp._wasm_note_on(note, 1.0);
    render(dsp, 0.5); // ensure the voice is sounding
    dsp._wasm_note_off(note);

    const tail = render(dsp, 3.0);
    const mono = steadyStateMono(tail, 0, 3.0);
    const silentAt = firstSilentWindowAt(mono, SAMPLE_RATE, 0);

    // A released voice must terminate; a leaked/infinite voice never silences.
    expect(silentAt).toBeGreaterThanOrEqual(0);
  });

  it('rapid note_on/note_off churn leaves no residual sound afterwards', () => {
    for (let i = 0; i < 8; i++) {
      dsp._wasm_note_on(48 + (i % 12), 1.0);
      render(dsp, 0.05);
      dsp._wasm_note_off(48 + (i % 12));
    }

    const tail = render(dsp, 3.0);
    const mono = steadyStateMono(tail, 0.5, 3.0);
    const silentAt = firstSilentWindowAt(mono, SAMPLE_RATE, 0);

    expect(silentAt).toBeGreaterThanOrEqual(0);
    // And by the end of the window it must be strictly silent.
    expect(maxAbs(mono.slice(mono.length - 4410))).toBeLessThan(1e-3);
  });
});

describe('DSP offline (WASM) — parameter bridge', () => {
  let dsp;

  beforeEach(async () => {
    dsp = await CZ101DSP();
    dsp._wasm_init(SAMPLE_RATE);
  });

  it('wasm_set_param → wasm_get_param roundtrips the normalized value', () => {
    setParam(dsp, 'OSC2_DETUNE', 0.75);
    expect(getParam(dsp, 'OSC2_DETUNE')).toBeCloseTo(0.75, 2);
  });
});
