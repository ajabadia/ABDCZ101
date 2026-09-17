// Main thread controller for ABD CZ-101 WebAssembly AudioEngine
export class CZ101AudioEngine {
  constructor() {
    this.audioCtx = null;
    this.workletNode = null;
    this.isReady = false;
    // Latest oscilloscope snapshot (mono samples) from the worklet, parity with
    // the native WaveformDisplay ring buffer consumed by the editor timer.
    this.scopeSamples = null;
  }

  async initialize() {
    if (this.audioCtx) return;

    this.audioCtx = new (window.AudioContext || window.webkitAudioContext)();
    await this.audioCtx.audioWorklet.addModule('./src/engine/cz101Worklet.js', {
      type: 'module'
    });

    this.workletNode = new AudioWorkletNode(this.audioCtx, 'cz101-audio-worklet', {
      numberOfInputs: 0,
      numberOfOutputs: 1,
      outputChannelCount: [2]
    });

    return new Promise((resolve) => {
      this.workletNode.port.onmessage = (event) => {
        if (event.data.type === 'WASM_READY') {
          this.isReady = true;
          resolve(true);
        } else if (event.data.type === 'SYSEX_LOADED') {
          if (this.onSysExLoaded) {
            this.onSysExLoaded(event.data.params, event.data.envelopes);
          }
        } else if (event.data.type === 'PRESET_LOADED') {
          if (this.onPresetLoaded) {
            this.onPresetLoaded(event.data.params, event.data.index, event.data.envelopes);
          }
        } else if (event.data.type === 'PRESET_SAVED') {
          if (this.onPresetSaved) {
            this.onPresetSaved(event.data.index, event.data.name);
          }
        } else if (event.data.type === 'BANK_LOADED') {
          if (this.onBankLoaded) {
            this.onBankLoaded(event.data.names, event.data.params, event.data.envelopes);
          }
        } else if (event.data.type === 'BANK_SAVED') {
          if (this._bankJsonCapture) {
            const cb = this._bankJsonCapture;
            this._bankJsonCapture = null;
            cb(event.data.jsonStr);
          } else if (this.onBankSaved) {
            this.onBankSaved(event.data.jsonStr);
          }
        } else if (event.data.type === 'SYSEX_SAVED') {
          if (this.onSysExSaved) {
            this.onSysExSaved(event.data.sysexBytes, event.data.name);
          }
        } else if (event.data.type === 'AUDIO_SCOPE') {
          this.scopeSamples = event.data.data ? event.data.data.samples : event.data.samples;
          if (this.onScopeUpdated) this.onScopeUpdated(this.scopeSamples);
        }
      };

      this.workletNode.connect(this.audioCtx.destination);
      this.workletNode.port.postMessage({
        type: 'INIT_WASM',
        data: { sampleRate: this.audioCtx.sampleRate }
      });
    });
  }

  async resume() {
    if (this.audioCtx && this.audioCtx.state === 'suspended') {
      await this.audioCtx.resume();
    }
  }

  noteOn(note, velocity = 0.8) {
    if (!this.workletNode) return;
    this.resume();
    this.workletNode.port.postMessage({
      type: 'NOTE_ON',
      data: { note, velocity }
    });
  }

  noteOff(note) {
    if (!this.workletNode) return;
    this.workletNode.port.postMessage({
      type: 'NOTE_OFF',
      data: { note }
    });
  }

  /**
   * Sends a raw MIDI byte array (e.g. from the Web MIDI API) to the WASM MIDI
   * processor: `sendMidiMessage(new Uint8Array([0x90, 60, 100]))`.
   */
  sendMidiMessage(bytes) {
    if (!this.workletNode) return;
    this.workletNode.port.postMessage({
      type: 'MIDI_MSG',
      data: { bytes }
    });
  }

  setParameter(paramId, normalizedValue) {
    if (!this.workletNode) return;
    this.workletNode.port.postMessage({
      type: 'SET_PARAM',
      data: { paramId, value: normalizedValue }
    });
  }

  loadSysEx(sysexBytes, paramIds) {
    if (!this.workletNode) return;
    this.workletNode.port.postMessage({
      type: 'LOAD_SYSEX',
      data: { sysexBytes, paramIds }
    });
  }

  loadPreset(index, paramIds) {
    if (!this.workletNode) return;
    this.workletNode.port.postMessage({
      type: 'LOAD_PRESET',
      data: { index, paramIds }
    });
  }

  savePreset(index, name) {
    if (!this.workletNode) return;
    this.workletNode.port.postMessage({
      type: 'SAVE_PRESET',
      data: { index, name }
    });
  }

  loadBank(jsonStr, paramIds) {
    if (!this.workletNode) return;
    this.workletNode.port.postMessage({
      type: 'LOAD_BANK',
      data: { jsonStr, paramIds }
    });
  }

  loadFactoryBank(paramIds) {
    if (!this.workletNode) return;
    this.workletNode.port.postMessage({
      type: 'LOAD_FACTORY_BANK',
      data: { paramIds }
    });
  }

  saveBank() {
    if (!this.workletNode) return;
    this.workletNode.port.postMessage({
      type: 'SAVE_BANK'
    });
  }

  // Capture the engine's current bank as JSON WITHOUT triggering the download
  // flow (used by the Bank Manager library to persist user-bank snapshots).
  captureBankJson(callback) {
    if (!this.workletNode || typeof callback !== 'function') return;
    this._bankJsonCapture = callback;
    this.saveBank();
  }

  renamePreset(index, name) {
    if (!this.workletNode) return;
    this.workletNode.port.postMessage({
      type: 'RENAME_PRESET',
      data: { index, name }
    });
  }

  deletePreset(index) {
    if (!this.workletNode) return;
    this.workletNode.port.postMessage({
      type: 'DELETE_PRESET',
      data: { index }
    });
  }

  movePreset(fromIndex, toIndex) {
    if (!this.workletNode) return;
    this.workletNode.port.postMessage({
      type: 'MOVE_PRESET',
      data: { fromIndex, toIndex }
    });
  }

  saveSysEx(name) {
    if (!this.workletNode) return;
    this.workletNode.port.postMessage({
      type: 'SAVE_SYSEX',
      data: { name }
    });
  }

  setEnvelopeStage(envType, line, stage, rate, level) {
    if (!this.workletNode) return;
    this.workletNode.port.postMessage({
      type: 'SET_ENV_STAGE',
      data: { envType, line, stage, rate, level }
    });
  }

  setEnvelopeSustain(envType, line, point) {
    if (!this.workletNode) return;
    this.workletNode.port.postMessage({
      type: 'SET_ENV_SUSTAIN',
      data: { envType, line, point }
    });
  }

  setEnvelopeEnd(envType, line, point) {
    if (!this.workletNode) return;
    this.workletNode.port.postMessage({
      type: 'SET_ENV_END',
      data: { envType, line, point }
    });
  }
}
