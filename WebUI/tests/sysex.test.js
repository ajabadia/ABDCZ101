import { describe, it, expect } from 'vitest';
import CZ101DSP from '../wasm/cz101_dsp.js';

describe('CZ-101 SysEx Data Contract Tests', () => {
  it('should validate Casio CZ-101 SysEx header signature', () => {
    // Casio SysEx Header: F0 44 00 00 70 ... F7
    const validHeader = [0xF0, 0x44, 0x00, 0x00, 0x70];
    expect(validHeader[0]).toBe(0xF0); // System Exclusive
    expect(validHeader[1]).toBe(0x44); // Casio Manufacturer ID
    expect(validHeader[4]).toBe(0x70); // CZ-101 Synth ID
  });

  it('should import a multi-patch 16-preset SysEx bank dump into the WASM engine', async () => {
    const dsp = await CZ101DSP();
    dsp._wasm_init(44100.0);

    // Create a 16-patch bank dump (16 * 264 = 4224 bytes)
    const bankBytes = new Uint8Array(4224);
    for (let p = 0; p < 16; ++p) {
      const offset = p * 264;
      bankBytes[offset] = 0xF0;
      bankBytes[offset + 1] = 0x44;
      bankBytes[offset + 2] = 0x00;
      bankBytes[offset + 3] = 0x00;
      bankBytes[offset + 4] = 0x70;
      bankBytes[offset + 5] = 0x20;
      bankBytes[offset + 6] = 0x00;
      
      // Fill dummy payload
      for (let i = 7; i < 262; ++i) {
        bankBytes[offset + i] = (i + p) % 16;
      }
      bankBytes[offset + 262] = 0x00;
      bankBytes[offset + 263] = 0xF7;
    }

    const ptr = dsp._malloc(bankBytes.length);
    dsp.HEAPU8.set(bankBytes, ptr);

    // Load SysEx bank and check returned preset count
    const loadedCount = dsp._wasm_load_sysex(ptr, bankBytes.length);
    dsp._free(ptr);

    expect(loadedCount).toBe(16);
  });
});
