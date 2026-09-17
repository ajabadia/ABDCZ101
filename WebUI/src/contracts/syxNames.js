// Pure helper: parses a .syx bank with the REAL compiled WASM engine (no audio
// context needed) to learn how many patches it holds and their names. The Bank
// Manager library uses this at startup to warm the factory banks so they never
// show as empty — the engine reports "Imported Preset" for every CZ-101 patch
// (the format stores no names), so callers can map those to "Patch NN".
import CZ101DSP from '../../wasm/cz101_dsp.js';

let dspPromise = null;

function getDsp() {
  if (!dspPromise) {
    dspPromise = CZ101DSP().then(dsp => {
      dsp._wasm_init(44100.0);
      return dsp;
    });
  }
  return dspPromise;
}

/**
 * Loads a .syx buffer into a throwaway WASM instance and returns the parsed
 * patch names. { count, names } — names.length === count; when the format
 * carries no names the engine yields "Imported Preset" for every patch.
 */
export async function parseSyxFile(bytes) {
  const dsp = await getDsp();
  const ptr = dsp._malloc(bytes.length);
  dsp.HEAPU8.set(bytes, ptr);
  const count = dsp._wasm_load_sysex(ptr, bytes.length);
  dsp._free(ptr);
  if (count <= 0) return { count: 0, names: [] };

  const names = [];
  const buf = dsp._malloc(256);
  for (let i = 0; i < count; i++) {
    dsp._wasm_get_preset_name(i, buf, 256);
    let name = '';
    for (let j = 0; j < 256; j++) {
      const c = dsp.HEAPU8[buf + j];
      if (c === 0) break;
      name += String.fromCharCode(c);
    }
    names.push(name);
  }
  dsp._free(buf);
  return { count, names };
}

/**
 * Maps raw engine names to human labels: the CZ-101 format has no names, so a
 * bare "Imported Preset" becomes "Patch 01" etc. Real names are kept.
 */
export function humanizePatchNames(names, baseLabel = 'Patch') {
  return names.map((name, i) => {
    const t = (name || '').trim();
    if (!t || t === 'Imported Preset') {
      return `${baseLabel} ${String(i + 1).padStart(2, '0')}`;
    }
    return t;
  });
}
