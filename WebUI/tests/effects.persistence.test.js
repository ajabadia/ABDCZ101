import { describe, it, expect } from 'vitest';
import CZ101DSP from '../wasm/cz101_dsp.js';

// Drives the real WASM engine (the same exports the AudioWorklet uses) so the
// save/load round-trip is verified against the actual C++ bridge logic.
function createEngine() {
  let dsp = null;

  const allocateString = (str) => {
    const ptr = dsp._malloc(str.length + 1);
    for (let i = 0; i < str.length; i++) dsp.HEAPU8[ptr + i] = str.charCodeAt(i);
    dsp.HEAPU8[ptr + str.length] = 0;
    return ptr;
  };

  const readString = (ptr) => {
    let out = '';
    for (let i = 0; i < 256; i++) {
      const c = dsp.HEAPU8[ptr + i];
      if (c === 0) break;
      out += String.fromCharCode(c);
    }
    return out;
  };

  return {
    async init(sampleRate = 44100) {
      dsp = await CZ101DSP();
      dsp._wasm_init(sampleRate);
      return this;
    },
    setParam(paramId, normalizedValue) {
      const ptr = allocateString(paramId);
      dsp._wasm_set_param(ptr, normalizedValue);
      dsp._free(ptr);
    },
    getParam(paramId) {
      const ptr = allocateString(paramId);
      const value = dsp._wasm_get_param(ptr);
      dsp._free(ptr);
      return value;
    },
    savePreset(index, name) {
      const ptr = allocateString(name);
      dsp._wasm_save_preset(index, ptr);
      dsp._free(ptr);
    },
    loadPreset(index) {
      dsp._wasm_load_preset(index);
    },
    getPresetName(index) {
      const buf = dsp._malloc(256);
      dsp._wasm_get_preset_name(index, buf, 256);
      const name = readString(buf);
      dsp._free(buf);
      return name;
    },
    saveBank() {
      const maxLen = 1024 * 1024;
      const bufPtr = dsp._malloc(maxLen);
      const actualLen = dsp._wasm_save_bank(bufPtr, maxLen);
      let json = '';
      for (let i = 0; i < actualLen; i++) {
        json += String.fromCharCode(dsp.HEAPU8[bufPtr + i]);
      }
      dsp._free(bufPtr);
      return json;
    },
    loadBank(jsonStr) {
      const ptr = allocateString(jsonStr);
      dsp._wasm_load_bank(ptr);
      dsp._free(ptr);
    }
  };
}

// Normalized values sent by the WebUI (rawToNormalized output) for each effect
// parameter. These are the values the worklet posts via wasm_set_param.
const FX_VALUES = {
  DRIVE_MIX: 0.7,
  DRIVE_AMOUNT: 0.5,
  CHORUS_MIX: 0.45,
  CHORUS_RATE: (3.0 - 0.1) / (10.0 - 0.1), // 3.0 Hz
  DELAY_MIX: 0.33,
  DELAY_TIME: 0.75 / 2.0, // 0.75 s
  REVERB_MIX: 0.55,
  REVERB_SIZE: 0.7,
  MODERN_LPF_CUTOFF: 0.5, // skew 0.3 -> raw ~2002.3 Hz
  MODERN_LPF_RESO: 0.25,
  MODERN_HPF_CUTOFF: 0.3
};

const FX_MUTATED = {
  DRIVE_MIX: 0.1,
  DRIVE_AMOUNT: 0.9,
  CHORUS_MIX: 0.8,
  CHORUS_RATE: 0.05,
  DELAY_MIX: 0.9,
  DELAY_TIME: 0.1,
  REVERB_MIX: 0.05,
  REVERB_SIZE: 0.2,
  MODERN_LPF_CUTOFF: 0.9,
  MODERN_LPF_RESO: 0.9,
  MODERN_HPF_CUTOFF: 0.8
};

describe('WASM effect persistence (presets & banks)', () => {
  it('round-trips effect settings through a preset slot', async () => {
    const engine = await createEngine().init();

    // 1. Set the desired effect values
    Object.entries(FX_VALUES).forEach(([id, norm]) => engine.setParam(id, norm));
    engine.savePreset(7, 'FX Round Trip');

    // 2. Mutate the live values (as a user would after saving)
    Object.entries(FX_MUTATED).forEach(([id, norm]) => engine.setParam(id, norm));

    // 3. Load the saved preset back
    engine.loadPreset(7);

    // 4. Every effect parameter must be restored
    Object.entries(FX_VALUES).forEach(([id, expectedNorm]) => {
      expect(engine.getParam(id)).toBeCloseTo(expectedNorm, 4);
    });
    expect(engine.getPresetName(7)).toBe('FX Round Trip');
  });

  it('round-trips effect settings through bank export/import', async () => {
    const engine = await createEngine().init();

    Object.entries(FX_VALUES).forEach(([id, norm]) => engine.setParam(id, norm));
    engine.savePreset(0, 'Bank FX A');
    engine.savePreset(1, 'Bank FX B');

    const bankJson = engine.saveBank();
    const bank = JSON.parse(bankJson);

    // The bank file must carry the effect parameters in raw (APVTS) units
    expect(bank.presets).toHaveLength(64);
    const presetA = bank.presets[0];
    expect(presetA.name).toBe('Bank FX A');
    expect(presetA.params.DRIVE_MIX).toBeCloseTo(0.7, 4);
    expect(presetA.params.CHORUS_RATE).toBeCloseTo(3.0, 4);
    expect(presetA.params.DELAY_TIME).toBeCloseTo(0.75, 4);
    expect(presetA.params.REVERB_SIZE).toBeCloseTo(0.7, 4);
    // Skewed cutoff: norm 0.5 -> raw = 20 + pow(0.5, 1/0.3) * 19980 = ~2002.3 Hz
    expect(presetA.params.MODERN_LPF_CUTOFF).toBeCloseTo(20 + Math.pow(0.5, 1 / 0.3) * 19980, 1);
    // Skewed HPF: norm 0.3 -> raw = 20 + pow(0.3, 1/0.3) * 9980 = ~200.4 Hz
    expect(presetA.params.MODERN_HPF_CUTOFF).toBeCloseTo(20 + Math.pow(0.3, 1 / 0.3) * 9980, 1);

    // 2. Mutate live values, then import the bank
    Object.entries(FX_MUTATED).forEach(([id, norm]) => engine.setParam(id, norm));
    engine.loadBank(bankJson);

    // 3. Loading a bank applies preset 0 -> effect settings restored
    Object.entries(FX_VALUES).forEach(([id, expectedNorm]) => {
      expect(engine.getParam(id)).toBeCloseTo(expectedNorm, 4);
    });
    expect(engine.getPresetName(0)).toBe('Bank FX A');
    expect(engine.getPresetName(1)).toBe('Bank FX B');
  });

  it('restores the skewed filter cutoff exactly through the raw/normalized bridge', async () => {
    const engine = await createEngine().init();

    // norm 0.5 on MODERN_LPF_CUTOFF must survive: wasm_set_param stores the
    // normalized value, wasm_save_preset converts to raw with the JUCE skew
    // (2002.3 Hz), and applyWasmPreset converts back to the same normalized 0.5.
    engine.setParam('MODERN_LPF_CUTOFF', 0.5);
    engine.savePreset(3, 'Skew Test');
    engine.setParam('MODERN_LPF_CUTOFF', 0.05);
    engine.loadPreset(3);

    expect(engine.getParam('MODERN_LPF_CUTOFF')).toBeCloseTo(0.5, 4);

    // Same check for the HPF (also skew 0.3 in the APVTS)
    engine.setParam('MODERN_HPF_CUTOFF', 0.7);
    engine.savePreset(3, 'Skew Test');
    engine.setParam('MODERN_HPF_CUTOFF', 0.2);
    engine.loadPreset(3);
    expect(engine.getParam('MODERN_HPF_CUTOFF')).toBeCloseTo(0.7, 4);
  });
});
