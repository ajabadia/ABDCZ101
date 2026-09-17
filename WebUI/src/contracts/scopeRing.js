// Ring buffer for the oscilloscope, parity with the native WaveformDisplay
// (which keeps a 256-sample waveform: pushBuffer copies channel 0 and wraps
// with writePos = (writePos + 1) % size).

export const SCOPE_RING_SIZE = 256;

// Feed `samples` into a ring buffer of length `size`. Returns a NEW Float32Array
// (the caller keeps a reference; mutation is explicit). `writePos` is the next
// write index. Parity with WaveformDisplay::pushBuffer: only the first `size`
// samples of a block are copied, then the write cursor wraps.
export function pushScopeSamples(ring, samples, writePos, size = SCOPE_RING_SIZE) {
  const out = ring.length === size ? ring.slice() : new Float32Array(size);
  const count = Math.min(samples.length, size);
  for (let i = 0; i < count; i++) {
    out[writePos] = samples[i];
    writePos = (writePos + 1) % size;
  }
  return { ring: out, writePos };
}

// Map a 0..width x position to the ring sample index, parity with the native
// paint loop (x = i/size * width). Clamps to the last sample.
export function ringIndexForX(x, width, size = SCOPE_RING_SIZE) {
  if (width <= 0) return 0;
  const idx = Math.floor((x / width) * size);
  return Math.min(Math.max(idx, 0), size - 1);
}
