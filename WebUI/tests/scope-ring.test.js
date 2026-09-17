import { describe, it, expect } from 'vitest';
import {
  SCOPE_RING_SIZE,
  pushScopeSamples,
  ringIndexForX
} from '../src/contracts/scopeRing.js';

describe('scopeRing (native WaveformDisplay pushBuffer parity)', () => {
  it('copies a full block and wraps the write cursor', () => {
    const ring = new Float32Array(SCOPE_RING_SIZE);
    const samples = new Float32Array(128).fill(0.5);
    const r1 = pushScopeSamples(ring, samples, 0);
    expect(r1.writePos).toBe(128);
    expect(r1.ring[0]).toBe(0.5);
    expect(r1.ring[127]).toBe(0.5);
    expect(r1.ring[128]).toBe(0); // untouched tail

    // Second block: 128 + 128 = 256 -> writePos wraps to 0.
    const r2 = pushScopeSamples(r1.ring, samples, r1.writePos);
    expect(r2.writePos).toBe(0);
    expect(r2.ring[0]).toBe(0.5); // overwritten by the wrapped write
  });

  it('wraps past the end (parity: writePos = (pos + 1) % size)', () => {
    const ring = new Float32Array(SCOPE_RING_SIZE);
    // Fill to the last slot: after 255 samples writePos = 255.
    const block = new Float32Array(255).fill(1);
    const r1 = pushScopeSamples(ring, block, 0);
    expect(r1.writePos).toBe(255);
    // One more sample is written at index 255, then writePos wraps to 0.
    const r2 = pushScopeSamples(r1.ring, new Float32Array([0.75]), r1.writePos);
    expect(r2.writePos).toBe(0);
    expect(r2.ring[255]).toBe(0.75);
    expect(r2.ring[0]).toBe(1); // untouched
  });

  it('copies only as much as fits (native min(numSamples, dataSize))', () => {
    const ring = new Float32Array(16);
    const big = new Float32Array(64).fill(0.25);
    const r = pushScopeSamples(ring, big, 0, 16);
    expect(r.writePos).toBe(0); // 16 writes -> 16 % 16 = 0
    expect(r.ring[15]).toBe(0.25);
    expect(Array.from(r.ring).every(v => v === 0.25)).toBe(true);
  });

  it('returns a new array (no aliasing of the input ring)', () => {
    const ring = new Float32Array(SCOPE_RING_SIZE);
    const r = pushScopeSamples(ring, new Float32Array([1]), 0);
    expect(r.ring).not.toBe(ring);
    expect(r.ring[0]).toBe(1);
  });

  it('does not mutate the input samples', () => {
    const ring = new Float32Array(SCOPE_RING_SIZE);
    const samples = new Float32Array([1, 2, 3]);
    pushScopeSamples(ring, samples, 0);
    expect(Array.from(samples)).toEqual([1, 2, 3]);
  });
});

describe('ringIndexForX (native paint x mapping)', () => {
  it('maps 0..width to ring indices and clamps', () => {
    expect(ringIndexForX(0, 200)).toBe(0);
    expect(ringIndexForX(199, 200)).toBe(254); // floor(199/200*256)
    expect(ringIndexForX(200, 200)).toBe(255); // clamped
    expect(ringIndexForX(-5, 200)).toBe(0);
    expect(ringIndexForX(50, 100, 100)).toBe(50);
  });
});
