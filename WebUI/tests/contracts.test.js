import { describe, it, expect } from 'vitest';
import { PARAMETER_REGISTRY, PARAM_MAP } from '../src/contracts/registry.gen.js';

describe('CZ-101 Parameter Registry Contract Tests', () => {
  it('should have valid schema version and plugin ID', () => {
    expect(PARAMETER_REGISTRY.schemaVersion).toBe(1);
    expect(PARAMETER_REGISTRY.pluginId).toBe('CZ101Emulator');
  });

  it('should contain expected parameter categories', () => {
    const categories = new Set(PARAMETER_REGISTRY.parameters.map(p => p.category));
    expect(categories.has('Oscillators')).toBe(true);
    expect(categories.has('LFO')).toBe(true);
    expect(categories.has('Envelopes')).toBe(true);
    expect(categories.has('Filter')).toBe(true);
    expect(categories.has('Effects')).toBe(true);
    expect(categories.has('System')).toBe(true);
  });

  it('should guarantee min <= default <= max for all parameters', () => {
    for (const param of PARAMETER_REGISTRY.parameters) {
      expect(param.min).toBeLessThanOrEqual(param.default);
      expect(param.default).toBeLessThanOrEqual(param.max);
    }
  });

  it('should allow fast lookup by param ID', () => {
    expect(PARAM_MAP.has('LINE_SELECT')).toBe(true);
    expect(PARAM_MAP.has('OSC1_WAVEFORM')).toBe(true);
    expect(PARAM_MAP.has('DCA_ATTACK')).toBe(true);
  });
});
