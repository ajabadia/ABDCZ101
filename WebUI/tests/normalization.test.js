import { describe, it, expect } from 'vitest';
import { rawToNormalized, normalizedToRaw, PARAM_MAP } from '../src/contracts/registry.gen.js';

describe('CZ-101 Parameter Normalization Bridge Tests', () => {
  it('should map boundary min/max correctly to 0.0 and 1.0', () => {
    const cutoffSpec = PARAM_MAP.get('MODERN_LPF_CUTOFF');
    expect(rawToNormalized('MODERN_LPF_CUTOFF', cutoffSpec.min)).toBe(0.0);
    expect(rawToNormalized('MODERN_LPF_CUTOFF', cutoffSpec.max)).toBe(1.0);
    expect(normalizedToRaw('MODERN_LPF_CUTOFF', 0.0)).toBe(cutoffSpec.min);
    expect(normalizedToRaw('MODERN_LPF_CUTOFF', 1.0)).toBe(cutoffSpec.max);
  });

  it('should perform roundtrip equality for float parameters', () => {
    // MASTER_TUNE is -50..50 cents in the real APVTS (Parameters.cpp)
    const rawInput = 25.0;
    const norm = rawToNormalized('MASTER_TUNE', rawInput);
    const roundtrip = normalizedToRaw('MASTER_TUNE', norm);
    expect(roundtrip).toBeCloseTo(rawInput, 4);
  });

  it('should apply the APVTS skew 0.3 to MODERN_LPF_CUTOFF normalization', () => {
    const cutoffSpec = PARAM_MAP.get('MODERN_LPF_CUTOFF');
    expect(cutoffSpec.skew).toBe(0.3);

    // juce::NormalisableRange(20, 20000, 0, 0.3) non-symmetric:
    //   convertFrom0to1(0.5) = 20 + pow(0.5, 1/0.3) * 19980 = 2002.3 Hz
    expect(normalizedToRaw('MODERN_LPF_CUTOFF', 0.5)).toBeCloseTo(2002.3, 1);

    // convertTo0to1 is the inverse: raw 2002.3 -> 0.5
    expect(rawToNormalized('MODERN_LPF_CUTOFF', 2002.3)).toBeCloseTo(0.5, 3);

    // Mid-range raw (10000 Hz) maps above the linear 0.5 -> ~0.812
    expect(rawToNormalized('MODERN_LPF_CUTOFF', 10000.0)).toBeCloseTo(0.812, 2);

    // Round-trip is consistent under skew
    const norm = rawToNormalized('MODERN_LPF_CUTOFF', 5000.0);
    expect(normalizedToRaw('MODERN_LPF_CUTOFF', norm)).toBeCloseTo(5000.0, 2);
  });

  it('should apply the same skew 0.3 to MODERN_HPF_CUTOFF (APVTS parity)', () => {
    const hpfSpec = PARAM_MAP.get('MODERN_HPF_CUTOFF');
    expect(hpfSpec.skew).toBe(0.3);
    // juce::NormalisableRange(20, 10000, 0, 0.3): convertFrom0to1(0.5) = 20 + pow(0.5, 1/0.3) * 9980
    const expectedHpf = 20.0 + Math.pow(0.5, 1.0 / 0.3) * 9980.0;
    expect(normalizedToRaw('MODERN_HPF_CUTOFF', 0.5)).toBeCloseTo(expectedHpf, 6);
  });

  it('should leave linear parameters unaffected when no skew is defined', () => {
    expect(PARAM_MAP.get('LFO_RATE').skew).toBeUndefined();
    expect(normalizedToRaw('LFO_RATE', 0.5)).toBeCloseTo(15.05, 4);
    expect(rawToNormalized('LFO_RATE', 15.05)).toBeCloseTo(0.5, 4);
    // Boundaries still hold for skewed params (0.0 and 1.0 are fixed points)
    expect(rawToNormalized('MODERN_LPF_CUTOFF', 20.0)).toBe(0.0);
    expect(rawToNormalized('MODERN_LPF_CUTOFF', 20000.0)).toBe(1.0);
    expect(normalizedToRaw('MODERN_LPF_CUTOFF', 0.0)).toBe(20.0);
    expect(normalizedToRaw('MODERN_LPF_CUTOFF', 1.0)).toBe(20000.0);
  });

  it('should use the real APVTS ranges from Parameters.cpp', () => {
    // OSC2_DETUNE: -12..12 semitones
    expect(PARAM_MAP.get('OSC2_DETUNE').min).toBe(-12);
    expect(PARAM_MAP.get('OSC2_DETUNE').max).toBe(12);
    expect(rawToNormalized('OSC2_DETUNE', 0.0)).toBeCloseTo(0.5, 4);

    // LFO_RATE: 0.1..30 Hz
    expect(PARAM_MAP.get('LFO_RATE').min).toBe(0.1);
    expect(PARAM_MAP.get('LFO_RATE').max).toBe(30.0);
    expect(rawToNormalized('LFO_RATE', 5.0)).toBeCloseTo((5.0 - 0.1) / (30.0 - 0.1), 4);

    // MASTER_TUNE: cents, not Hz
    expect(PARAM_MAP.get('MASTER_TUNE').min).toBe(-50);
    expect(PARAM_MAP.get('MASTER_TUNE').max).toBe(50);
  });

  it('should expose the newly migrated parameters in the registry', () => {
    expect(PARAM_MAP.has('OPERATION_MODE')).toBe(true);
    expect(PARAM_MAP.has('PROTECT_SWITCH')).toBe(true);
    expect(PARAM_MAP.has('HARDWARE_NOISE')).toBe(true);
    expect(PARAM_MAP.has('DRIVE_MIX')).toBe(true);
    expect(PARAM_MAP.has('KEY_FOLLOW_DCO')).toBe(true);
    expect(PARAM_MAP.has('KEY_FOLLOW_DCW')).toBe(true);
    expect(PARAM_MAP.has('KEY_FOLLOW_DCA')).toBe(true);
    expect(PARAM_MAP.has('MIDI_CH')).toBe(true);
    expect(PARAM_MAP.has('PITCH_BEND_RANGE')).toBe(true);
    expect(PARAM_MAP.has('OVERSAMPLING_QUALITY')).toBe(true);
    expect(PARAM_MAP.has('HARD_SYNC')).toBe(true);
    expect(PARAM_MAP.has('LINE_MODULATION')).toBe(true);
  });

  it('should model KEY_FOLLOW_* as 3-option choices (OFF/FIX/VAR)', () => {
    const kfd = PARAM_MAP.get('KEY_FOLLOW_DCO');
    expect(kfd.type).toBe('choice');
    expect(kfd.choices).toEqual(['OFF', 'FIX', 'VAR']);
    expect(kfd.default).toBe(2); // VAR (matches Parameters.cpp)
    // rawToNormalized: raw 2 of 0..2 -> 1.0
    expect(rawToNormalized('KEY_FOLLOW_DCO', 2)).toBe(1.0);
    // normalizedToRaw: 1.0 -> 2
    expect(normalizedToRaw('KEY_FOLLOW_DCO', 1.0)).toBe(2);
  });

  it('should model boolean defaults matching the APVTS', () => {
    expect(PARAM_MAP.get('HARD_SYNC').default).toBe(0);
    expect(PARAM_MAP.get('MOD_SPECIAL').default).toBe(0);
    expect(PARAM_MAP.get('PROTECT_SWITCH').default).toBe(1);
    expect(PARAM_MAP.get('HARDWARE_NOISE').default).toBe(1);
  });

  it('should clamp out-of-bound values safely', () => {
    expect(rawToNormalized('DCA_ATTACK', -5.0)).toBe(0.0);
    expect(rawToNormalized('DCA_ATTACK', 99.0)).toBe(1.0);
    expect(normalizedToRaw('DCA_ATTACK', -0.5)).toBe(PARAM_MAP.get('DCA_ATTACK').min);
    expect(normalizedToRaw('DCA_ATTACK', 1.5)).toBe(PARAM_MAP.get('DCA_ATTACK').max);
  });

  it('should round integer and choice parameters correctly', () => {
    // LINE_SELECT is 0..3 (4 options) in the real APVTS
    const lineSelectRaw = normalizedToRaw('LINE_SELECT', 0.5); // Maps 0..3 -> 2.0
    expect(Number.isInteger(lineSelectRaw)).toBe(true);
    expect(lineSelectRaw).toBe(2);
  });
});
