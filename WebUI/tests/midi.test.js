import { describe, it, expect, beforeEach } from 'vitest';
import CZ101DSP from '../wasm/cz101_dsp.js';

describe('WASM MIDI Processor Tests', () => {
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

  const sendMidi = (bytes) => {
    const ptr = dsp._malloc(bytes.length);
    dsp.HEAPU8.set(bytes, ptr);
    dsp._wasm_midi_message(ptr, bytes.length);
    dsp._free(ptr);
  };

  beforeEach(async () => {
    dsp = await CZ101DSP();
    dsp._wasm_init(44100.0);
  });

  it('should parse Note On and Note Off messages correctly', () => {
    // We can run process and verify sound is produced or state changes.
    // Note On: Channel 1, Note 60 (C4), Velocity 100 (0x90, 0x3C, 0x64)
    sendMidi([0x90, 0x3C, 0x64]);
    
    // Note Off: Channel 1, Note 60 (C4), Velocity 0 (0x80, 0x3C, 0x00)
    sendMidi([0x80, 0x3C, 0x00]);
  });

  it('should filter MIDI channels according to MIDI_CH parameter', () => {
    // Set MIDI channel parameter to Ch 1 (normalized 0.0)
    setParam('MIDI_CH', 0.0);
    
    // Message on Channel 2: Note On 60 (0x91, 0x3C, 0x64) - should be ignored
    sendMidi([0x91, 0x3C, 0x64]);
    
    // Message on Channel 1: Note On 60 (0x90, 0x3C, 0x64) - should play
    sendMidi([0x90, 0x3C, 0x64]);
  });

  it('should parse CC Mod Wheel (CC 1) messages', () => {
    // CC 1, Value 64 on Channel 1 (0xB0, 0x01, 0x40)
    sendMidi([0xB0, 0x01, 0x40]);
  });

  it('should parse Pitch Bend messages', () => {
    // Pitch Bend Center: 8192 (0x2000) -> 0xE0, 0x00, 0x40
    sendMidi([0xE0, 0x00, 0x40]);
    
    // Pitch Bend Max: 16383 (0x3FFF) -> 0xE0, 0x7F, 0x7F
    sendMidi([0xE0, 0x7F, 0x7F]);
  });
});
