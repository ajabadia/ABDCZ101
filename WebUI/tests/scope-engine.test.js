import { describe, it, expect, vi, beforeEach } from 'vitest';

// Mock the Web Audio API so the engine can be initialized in Node and the
// worklet -> main-thread AUDIO_SCOPE pipeline verified deterministically.
function installMocks() {
  const messageHandlers = {};

  class FakeAudioContext {
    constructor() {
      this.state = 'running';
      this.sampleRate = 44100;
      this.destination = { connect: () => {} };
      this.audioWorklet = { addModule: vi.fn(async () => {}) };
    }
    resume() { this.state = 'running'; return Promise.resolve(); }
  }

  class FakeAudioWorkletNode {
    constructor(ctx) {
      this.port = {
        postMessage: vi.fn((msg) => {
          // Simulate the worklet answering back after INIT_WASM
          if (msg.type === 'INIT_WASM' && this.port.onmessage) {
            queueMicrotask(() => this.port.onmessage({ data: { type: 'WASM_READY' } }));
          }
          // Let tests trigger worklet->main messages via the captured handler.
          messageHandlers.workletPort = this.port;
        }),
        onmessage: null
      };
      this.connect = vi.fn();
    }
  }

  vi.stubGlobal('window', { AudioContext: FakeAudioContext, webkitAudioContext: FakeAudioContext });
  vi.stubGlobal('AudioContext', FakeAudioContext);
  vi.stubGlobal('webkitAudioContext', FakeAudioContext);
  vi.stubGlobal('AudioWorkletNode', FakeAudioWorkletNode);

  return messageHandlers;
}

describe('audio engine scope pipeline (worklet -> scopeSamples)', () => {
  let engine;
  let handlers;

  beforeEach(async () => {
    vi.resetModules();
    handlers = installMocks();
    const { CZ101AudioEngine } = await import('../src/engine/cz101AudioEngine.js');
    engine = new CZ101AudioEngine();
    await engine.initialize();
    expect(engine.isReady).toBe(true);
    vi.clearAllMocks(); // keep the postMessage spies clean for each test
  });

  it('exposes no samples before any AUDIO_SCOPE arrives', () => {
    expect(engine.scopeSamples).toBeNull();
  });

  it('captures AUDIO_SCOPE samples from the worklet', () => {
    const samples = new Array(256).fill(0.25);
    handlers.workletPort.onmessage({ data: { type: 'AUDIO_SCOPE', data: { samples } } });
    expect(engine.scopeSamples).toEqual(samples);
  });

  it('fires onScopeUpdated when a snapshot arrives', () => {
    const onScopeUpdated = vi.fn();
    engine.onScopeUpdated = onScopeUpdated;
    handlers.workletPort.onmessage({ data: { type: 'AUDIO_SCOPE', data: { samples: [0.1, 0.2] } } });
    expect(onScopeUpdated).toHaveBeenCalledTimes(1);
    expect(onScopeUpdated).toHaveBeenCalledWith([0.1, 0.2]);
  });

  it('keeps the last snapshot (newer snapshots replace older)', () => {
    handlers.workletPort.onmessage({ data: { type: 'AUDIO_SCOPE', data: { samples: [1, 2, 3] } } });
    handlers.workletPort.onmessage({ data: { type: 'AUDIO_SCOPE', data: { samples: [9, 8] } } });
    expect(engine.scopeSamples).toEqual([9, 8]);
  });

  it('ignores unrelated worklet messages without touching scopeSamples', () => {
    handlers.workletPort.onmessage({ data: { type: 'PRESET_SAVED', data: { index: 3, name: 'X' } } });
    expect(engine.scopeSamples).toBeNull();
  });
});
