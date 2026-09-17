import { describe, it, expect, beforeEach } from 'vitest';
import CZ101DSP from '../wasm/cz101_dsp.js';

// Bank management parity against the REAL compiled WASM engine: rename / delete
// / move + active-index tracking (same semantics as the native PresetManager).
// The WebUI Bank Manager context menu drives these via the worklet.

describe('WASM bank management (rename/delete/move)', () => {
  let dsp;

  const allocateString = (str) => {
    const ptr = dsp._malloc(str.length + 1);
    for (let i = 0; i < str.length; i++) dsp.HEAPU8[ptr + i] = str.charCodeAt(i);
    dsp.HEAPU8[ptr + str.length] = 0;
    return ptr;
  };

  const getName = (index) => {
    const buf = dsp._malloc(256);
    dsp._wasm_get_preset_name(index, buf, 256);
    let name = '';
    for (let j = 0; j < 256; j++) {
      const c = dsp.HEAPU8[buf + j];
      if (c === 0) break;
      name += String.fromCharCode(c);
    }
    dsp._free(buf);
    return name;
  };

  const names = () => {
    const out = [];
    for (let i = 0; i < 64; i++) out.push(getName(i));
    return out;
  };

  const saveNamed = (index, name) => {
    const ptr = allocateString(name);
    dsp._wasm_save_preset(index, ptr);
    dsp._free(ptr);
  };

  const setParam = (paramId, normalizedValue) => {
    const ptr = allocateString(paramId);
    dsp._wasm_set_param(ptr, normalizedValue);
    dsp._free(ptr);
  };

  beforeEach(async () => {
    dsp = await CZ101DSP();
    dsp._wasm_init(44100.0);
    // Seed three recognizable names for the tests
    saveNamed(0, 'ALPHA');
    saveNamed(1, 'BRAVO');
    saveNamed(2, 'CHARLIE');
  });

  it('renames a slot in place', () => {
    const p = allocateString('RENAMED');
    dsp._wasm_rename_preset(1, p);
    dsp._free(p);
    expect(getName(1)).toBe('RENAMED');
    expect(getName(0)).toBe('ALPHA');
    expect(getName(2)).toBe('CHARLIE');
  });

  it('rejects out-of-range rename', () => {
    const p = allocateString('NOPE');
    dsp._wasm_rename_preset(99, p);
    dsp._free(p);
    expect(names().slice(0, 3)).toEqual(['ALPHA', 'BRAVO', 'CHARLIE']);
  });

  it('deletePreset removes the slot and shifts the rest up', () => {
    dsp._wasm_delete_preset(1);
    const n = names();
    expect(n[1]).toBe('CHARLIE'); // BRAVO removed, CHARLIE moved up
    // Active index clamps / shifts: we never loaded a preset, so it stays valid.
    const idx = dsp._wasm_get_current_preset_index();
    expect(idx).toBeGreaterThanOrEqual(0);
    expect(idx).toBeLessThan(64);
  });

  it('deletePreset keeps at least one preset when the bank empties', () => {
    for (let i = 63; i >= 0; i--) dsp._wasm_delete_preset(0);
    const n = names();
    expect(n[0]).toBe('Init');
    expect(dsp._wasm_get_current_preset_index()).toBe(0);
  });

  it('movePreset reorders and tracks the active preset', () => {
    // Load preset 0 (ALPHA) as active, then move slot 2 -> 0: CHARLIE is
    // inserted at the front, ALPHA shifts to index 1, so the active preset
    // (same sound) now lives at index 1.
    dsp._wasm_load_preset(0);
    dsp._wasm_move_preset(2, 0);
    expect(dsp._wasm_get_current_preset_index()).toBe(1);
    expect(names().slice(0, 3)).toEqual(['CHARLIE', 'ALPHA', 'BRAVO']);
  });

  it('movePreset is a no-op for out-of-range or same index', () => {
    dsp._wasm_load_preset(1);
    dsp._wasm_move_preset(0, 0);
    dsp._wasm_move_preset(-1, 2);
    dsp._wasm_move_preset(0, 99);
    expect(names().slice(0, 3)).toEqual(['ALPHA', 'BRAVO', 'CHARLIE']);
    expect(dsp._wasm_get_current_preset_index()).toBe(1);
  });

  it('movePreset keeps the active preset pointing at the same sound', () => {
    // Active = 2 (CHARLIE). Move slot 1 up to 0: ALPHA/BRAVO swap below it,
    // CHARLIE (active) must stay at index 2.
    dsp._wasm_load_preset(2);
    dsp._wasm_move_preset(1, 0);
    expect(dsp._wasm_get_current_preset_index()).toBe(2);
    expect(names().slice(0, 3)).toEqual(['BRAVO', 'ALPHA', 'CHARLIE']);
  });

  it('works after loading a full bank (round-trip with save)', () => {
    // Save a bank, edit, and confirm names survive a rename.
    const bankPtr = dsp._malloc(1024 * 1024);
    const len = dsp._wasm_save_bank(bankPtr, 1024 * 1024);
    let jsonStr = '';
    for (let i = 0; i < len; i++) jsonStr += String.fromCharCode(dsp.HEAPU8[bankPtr + i]);
    dsp._free(bankPtr);

    const jsonPtr = allocateString(jsonStr);
    dsp._wasm_load_bank(jsonPtr);
    dsp._free(jsonPtr);

    // Bank loaded -> slot 0 active; names should be present.
    expect(names()[0]).not.toBe('');
    const p = allocateString('BANK RENAMED');
    dsp._wasm_rename_preset(0, p);
    dsp._free(p);
    expect(getName(0)).toBe('BANK RENAMED');
  });
});
