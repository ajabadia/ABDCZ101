import { describe, it, expect } from 'vitest';
import { rawToNormalized, normalizedToRaw } from '../src/contracts/registry.gen.js';

describe('JUCE Skew 0.3 Parity Tests', () => {
  it('should match skew 0.3 boundaries for MODERN_LPF_CUTOFF', () => {
    // Limits
    expect(rawToNormalized('MODERN_LPF_CUTOFF', 20)).toBeCloseTo(0.0, 5);
    expect(rawToNormalized('MODERN_LPF_CUTOFF', 20000)).toBeCloseTo(1.0, 5);
    
    expect(normalizedToRaw('MODERN_LPF_CUTOFF', 0.0)).toBeCloseTo(20, 5);
    expect(normalizedToRaw('MODERN_LPF_CUTOFF', 1.0)).toBeCloseTo(20000, 5);
  });

  it('should calculate the skew curve correctly at midpoint (norm = 0.5)', () => {
    // Midpoint: 20 + pow(0.5, 1/0.3) * (20000 - 20)
    // 0.5^(1/0.3) = 0.5^3.3333333 = 0.099212565
    // 20 + 0.099212565 * 19980 = 2002.267
    const expectedRaw = 20 + Math.pow(0.5, 1.0 / 0.3) * (20000 - 20); // ~2002.27
    const calculatedRaw = normalizedToRaw('MODERN_LPF_CUTOFF', 0.5);
    
    expect(calculatedRaw).toBeCloseTo(expectedRaw, 2);
    expect(rawToNormalized('MODERN_LPF_CUTOFF', calculatedRaw)).toBeCloseTo(0.5, 5);
  });

  it('should match skew 0.3 boundaries for MODERN_HPF_CUTOFF', () => {
    // MODERN_HPF_CUTOFF: min=20, max=10000, skew=0.3
    expect(rawToNormalized('MODERN_HPF_CUTOFF', 20)).toBeCloseTo(0.0, 5);
    expect(rawToNormalized('MODERN_HPF_CUTOFF', 10000)).toBeCloseTo(1.0, 5);

    const expectedMidRaw = 20 + Math.pow(0.5, 1.0 / 0.3) * (10000 - 20); // ~1010.14
    expect(normalizedToRaw('MODERN_HPF_CUTOFF', 0.5)).toBeCloseTo(expectedMidRaw, 2);
  });
});
