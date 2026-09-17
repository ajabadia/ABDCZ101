import { describe, it, expect, beforeEach } from 'vitest';
import { createMidiLearnStore } from '../src/contracts/midiLearn.js';

// Tests for the MIDI Learn manager — parity with the native
// MIDIProcessor::learnNextCC / unmapCC / getCCForParam flow:
//   - learn(): next CC is captured and mapped; the teaching CC is consumed
//     (not applied to any parameter).
//   - A mapped CC drives the parameter via the callback and is NOT forwarded
//     (native: "Override default behavior if mapped").
//   - Unmapped CCs / non-CC messages pass through untouched.
//   - unmapCC / getCCForParam mirror the native map semantics.

describe('MIDI Learn (native MIDIProcessor parity)', () => {
  let learn;
  let routed;

  beforeEach(() => {
    learn = createMidiLearnStore({ persist: false });
    routed = [];
  });

  const cc = (num, value = 64) => new Uint8Array([0xB0, num, value]);
  const onParam = (paramId, value) => routed.push([paramId, value]);

  it('learnNextCC: the next CC is captured and mapped, and the teaching CC is consumed', () => {
    learn.learn('LFO_RATE');
    const result = learn.handleMidiMessage(cc(23, 100), onParam);

    expect(result.consumed).toBe(true);
    expect(result.learned).toEqual({ cc: 23, paramId: 'LFO_RATE' });
    expect(learn.isLearning).toBe(false);
    expect(learn.getCCForParam('LFO_RATE')).toBe(23);
    expect(routed).toEqual([]); // teaching CC not applied to any param
  });

  it('a mapped CC drives the parameter and is not forwarded to the engine', () => {
    learn.learn('LFO_RATE');
    learn.handleMidiMessage(cc(23, 64), onParam);

    const result = learn.handleMidiMessage(cc(23, 127), onParam);
    expect(result.consumed).toBe(true);
    expect(result.mapped).toMatchObject({ cc: 23, paramId: 'LFO_RATE' });
    expect(result.mapped.value).toBeCloseTo(1.0, 5);
    expect(routed).toEqual([['LFO_RATE', 1]]);
  });

  it('a mapped CC value is normalized 0..1 (value / 127)', () => {
    learn.learn('MASTER_VOLUME');
    learn.handleMidiMessage(cc(50, 64), onParam);

    learn.handleMidiMessage(cc(50, 0), onParam);
    learn.handleMidiMessage(cc(50, 64), onParam);
    learn.handleMidiMessage(cc(50, 127), onParam);
    expect(routed).toEqual([
      ['MASTER_VOLUME', 0],
      ['MASTER_VOLUME', 64 / 127],
      ['MASTER_VOLUME', 1]
    ]);
  });

  it('an unmapped CC passes through untouched (default hardcoded behavior)', () => {
    const result = learn.handleMidiMessage(cc(1, 64), onParam); // mod wheel
    expect(result.consumed).toBe(false);
    expect(routed).toEqual([]);
  });

  it('a non-CC message (note on) passes through untouched', () => {
    const result = learn.handleMidiMessage(new Uint8Array([0x90, 60, 100]), onParam);
    expect(result.consumed).toBe(false);
    expect(routed).toEqual([]);
  });

  it('unmapCC removes a mapping; the CC reverts to pass-through', () => {
    learn.learn('LFO_RATE');
    learn.handleMidiMessage(cc(23, 64), onParam);

    expect(learn.unmapCC(23)).toBe(true);
    expect(learn.getCCForParam('LFO_RATE')).toBe(-1);
    expect(learn.unmapCC(23)).toBe(false); // already gone

    const result = learn.handleMidiMessage(cc(23, 64), onParam);
    expect(result.consumed).toBe(false);
    expect(routed).toEqual([]);
  });

  it('getCCForParam returns -1 when nothing is mapped', () => {
    expect(learn.getCCForParam('LFO_RATE')).toBe(-1);
  });

  it('getMappings lists mappings sorted by CC', () => {
    learn.learn('LFO_RATE');
    learn.handleMidiMessage(cc(23, 64), onParam);
    learn.learn('MASTER_VOLUME');
    learn.handleMidiMessage(cc(7, 64), onParam);

    expect(learn.getMappings()).toEqual([
      [7, 'MASTER_VOLUME'],
      [23, 'LFO_RATE']
    ]);
  });

  it('clear removes every mapping', () => {
    learn.learn('LFO_RATE');
    learn.handleMidiMessage(cc(23, 64), onParam);
    learn.clear();
    expect(learn.getMappings()).toEqual([]);
    expect(learn.getCCForParam('LFO_RATE')).toBe(-1);
  });

  it('cancelLearn aborts a pending learn without consuming the next CC', () => {
    learn.learn('LFO_RATE');
    learn.cancelLearn();
    expect(learn.isLearning).toBe(false);

    const result = learn.handleMidiMessage(cc(23, 64), onParam);
    expect(result.consumed).toBe(false);
    expect(learn.getCCForParam('LFO_RATE')).toBe(-1);
  });

  it('a parameter can be re-learned onto a new CC while keeping the old mapping (native map semantics)', () => {
    learn.learn('LFO_RATE');
    learn.handleMidiMessage(cc(23, 64), onParam);
    learn.learn('LFO_RATE');
    learn.handleMidiMessage(cc(24, 64), onParam);

    expect(learn.getCCForParam('LFO_RATE')).toBe(23); // first CC wins (native loop)
    expect(learn.getMappings()).toEqual([
      [23, 'LFO_RATE'],
      [24, 'LFO_RATE']
    ]);
  });

  it('persists mappings to localStorage when enabled', () => {
    // The vitest suite runs with environment: 'node' (no localStorage); provide
    // a minimal in-memory polyfill so the persistence path is exercised.
    const memory = new Map();
    globalThis.localStorage = {
      getItem: (k) => (memory.has(k) ? memory.get(k) : null),
      setItem: (k, v) => memory.set(k, String(v)),
      removeItem: (k) => memory.delete(k)
    };

    const storeKey = 'cz101.midiLearn.test';
    const store = createMidiLearnStore({ persist: true, storageKey: storeKey });
    store.learn('LFO_RATE');
    store.handleMidiMessage(cc(23, 64), onParam);

    const reloaded = createMidiLearnStore({ persist: true, storageKey: storeKey });
    expect(reloaded.getCCForParam('LFO_RATE')).toBe(23);
    store.clear();
  });
});
