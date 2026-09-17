// Polyfill performance.now() inside isolated AudioWorkletGlobalScope
if (typeof globalThis.performance === 'undefined') {
  globalThis.performance = {
    now: () => Date.now()
  };
}

import CZ101DSP from '../../wasm/cz101_dsp.js';
import { pushScopeSamples, SCOPE_RING_SIZE } from '../contracts/scopeRing.js';

class CZ101AudioWorkletProcessor extends AudioWorkletProcessor {
  constructor() {
    super();
    this.wasmInstance = null;
    this.heapL = null;
    this.heapR = null;
    this.outPtrL = 0;
    this.outPtrR = 0;
    this.isReady = false;

    // Oscilloscope: ring buffer of real mono samples (parity with the native
    // WaveformDisplay, which keeps a 256-sample waveform from the audio thread
    // and repaints every 50 ms). We accumulate every processed block and ship
    // a snapshot every SCOPE_INTERVAL_BLOCKS to the main thread.
    this.scopeBuffer = new Float32Array(SCOPE_RING_SIZE);
    this.scopeWritePos = 0;
    this.scopeBlockCount = 0;
    this.SCOPE_INTERVAL_BLOCKS = 14; // 128 samples * 14 ≈ 40 ms @ 44.1 kHz

    this.port.onmessage = (event) => {
      const { type, data } = event.data;

      const readActiveEnvelopes = () => {
        const getEnvData = (envType, line) => {
          const rates = [];
          const levels = [];
          for (let i = 0; i < 8; i++) {
            rates.push(this.wasmInstance._wasm_get_envelope_rate(envType, line, i));
            levels.push(this.wasmInstance._wasm_get_envelope_level(envType, line, i));
          }
          return {
            rates,
            levels,
            sustainPoint: this.wasmInstance._wasm_get_envelope_sustain(envType, line),
            endPoint: this.wasmInstance._wasm_get_envelope_end(envType, line)
          };
        };

        return {
          dca: {
            line1: getEnvData(2, 1),
            line2: getEnvData(2, 2)
          },
          dcw: {
            line1: getEnvData(1, 1),
            line2: getEnvData(1, 2)
          },
          pitch: {
            line1: getEnvData(0, 1),
            line2: getEnvData(0, 2)
          }
        };
      };

      if (type === 'INIT_WASM') {
        this.initWasmModule(data.sampleRate);
      } else if (type === 'NOTE_ON' && this.isReady) {
        this.wasmInstance._wasm_note_on(data.note, data.velocity);
      } else if (type === 'NOTE_OFF' && this.isReady) {
        this.wasmInstance._wasm_note_off(data.note);
      } else if (type === 'MIDI_MSG' && this.isReady) {
        const bytes = new Uint8Array(data.bytes);
        const ptr = this.wasmInstance._malloc(bytes.length);
        this.wasmInstance.HEAPU8.set(bytes, ptr);
        this.wasmInstance._wasm_midi_message(ptr, bytes.length);
        this.wasmInstance._free(ptr);
      } else if (type === 'SET_PARAM' && this.isReady) {
        const paramIdPtr = this.allocateString(data.paramId);
        this.wasmInstance._wasm_set_param(paramIdPtr, data.value);
        this.wasmInstance._free(paramIdPtr);
      } else if (type === 'SET_ENV_STAGE' && this.isReady) {
        this.wasmInstance._wasm_set_envelope_stage(data.envType, data.line, data.stage, data.rate, data.level);
      } else if (type === 'SET_ENV_SUSTAIN' && this.isReady) {
        this.wasmInstance._wasm_set_envelope_sustain(data.envType, data.line, data.point);
      } else if (type === 'SET_ENV_END' && this.isReady) {
        this.wasmInstance._wasm_set_envelope_end(data.envType, data.line, data.point);
      } else if (type === 'LOAD_SYSEX' && this.isReady) {
        const bytes = new Uint8Array(data.sysexBytes);
        const ptr = this.wasmInstance._malloc(bytes.length);
        this.wasmInstance.HEAPU8.set(bytes, ptr);
        const loadedCount = this.wasmInstance._wasm_load_sysex(ptr, bytes.length);
        this.wasmInstance._free(ptr);

        // Fetch all updated parameter values from WASM registry to sync the UI
        const updatedParams = {};
        if (data.paramIds) {
          data.paramIds.forEach(id => {
            const idPtr = this.allocateString(id);
            const val = this.wasmInstance._wasm_get_param(idPtr);
            this.wasmInstance._free(idPtr);
            updatedParams[id] = val;
          });
        }

        const activeEnvs = readActiveEnvelopes();

        if (loadedCount > 1) {
          // It's a bank! Fetch all preset names
          const names = [];
          const nameBufMax = 64;
          const nameBufPtr = this.wasmInstance._malloc(nameBufMax);
          for (let i = 0; i < loadedCount; ++i) {
            this.wasmInstance._wasm_get_preset_name(i, nameBufPtr, nameBufMax);
            let name = "";
            let offset = 0;
            while (offset < nameBufMax) {
              const byte = this.wasmInstance.HEAPU8[nameBufPtr + offset];
              if (byte === 0) break;
              name += String.fromCharCode(byte);
              offset++;
            }
            names.push(name || `Slot ${i + 1}`);
          }
          this.wasmInstance._free(nameBufPtr);

          // Fill up remaining names up to 64 if needed
          while (names.length < 64) {
            names.push(`Init User ${names.length + 1}`);
          }

          this.port.postMessage({
            type: 'BANK_LOADED',
            names: names,
            params: updatedParams,
            envelopes: activeEnvs
          });
        } else {
          // Post back to main thread
          this.port.postMessage({
            type: 'SYSEX_LOADED',
            params: updatedParams,
            envelopes: activeEnvs
          });
        }
      } else if (type === 'SAVE_SYSEX' && this.isReady) {
        const namePtr = this.allocateString(data.name || "Active Patch");
        const maxLen = 512;
        const outPtr = this.wasmInstance._malloc(maxLen);
        const bytesWritten = this.wasmInstance._wasm_save_sysex(outPtr, maxLen, namePtr);
        this.wasmInstance._free(namePtr);
        
        let sysexBytes = null;
        if (bytesWritten > 0) {
          sysexBytes = Array.from(this.wasmInstance.HEAPU8.subarray(outPtr, outPtr + bytesWritten));
        }
        this.wasmInstance._free(outPtr);
        
        this.port.postMessage({
          type: 'SYSEX_SAVED',
          sysexBytes: sysexBytes,
          name: data.name
        });
      } else if (type === 'LOAD_PRESET' && this.isReady) {
        this.wasmInstance._wasm_load_preset(data.index);
        const updatedParams = {};
        if (data.paramIds) {
          data.paramIds.forEach(id => {
            const idPtr = this.allocateString(id);
            const val = this.wasmInstance._wasm_get_param(idPtr);
            this.wasmInstance._free(idPtr);
            updatedParams[id] = val;
          });
        }
        this.port.postMessage({
          type: 'PRESET_LOADED',
          params: updatedParams,
          index: data.index,
          envelopes: readActiveEnvelopes()
        });
      } else if (type === 'SAVE_PRESET' && this.isReady) {
        const namePtr = this.allocateString(data.name);
        this.wasmInstance._wasm_save_preset(data.index, namePtr);
        this.wasmInstance._free(namePtr);
        this.port.postMessage({
          type: 'PRESET_SAVED',
          index: data.index,
          name: data.name
        });
      } else if (type === 'LOAD_BANK' && this.isReady) {
        const jsonPtr = this.allocateString(data.jsonStr);
        this.wasmInstance._wasm_load_bank(jsonPtr);
        this.wasmInstance._free(jsonPtr);
        
        const names = [];
        const nameBuf = this.wasmInstance._malloc(256);
        for (let i = 0; i < 64; i++) {
          this.wasmInstance._wasm_get_preset_name(i, nameBuf, 256);
          let name = "";
          for (let j = 0; j < 256; j++) {
            const char = this.wasmInstance.HEAPU8[nameBuf + j];
            if (char === 0) break;
            name += String.fromCharCode(char);
          }
          names.push(name);
        }
        this.wasmInstance._free(nameBuf);

        const updatedParams = {};
        if (data.paramIds) {
          data.paramIds.forEach(id => {
            const idPtr = this.allocateString(id);
            const val = this.wasmInstance._wasm_get_param(idPtr);
            this.wasmInstance._free(idPtr);
            updatedParams[id] = val;
          });
        }
          this.port.postMessage({
            type: 'BANK_LOADED',
            names,
            params: updatedParams,
            envelopes: readActiveEnvelopes()
          });
      } else if (type === 'LOAD_FACTORY_BANK' && this.isReady) {
        this.wasmInstance._wasm_load_factory_bank();
        
        const names = [];
        const nameBuf = this.wasmInstance._malloc(256);
        for (let i = 0; i < 64; i++) {
          this.wasmInstance._wasm_get_preset_name(i, nameBuf, 256);
          let name = "";
          for (let j = 0; j < 256; j++) {
            const char = this.wasmInstance.HEAPU8[nameBuf + j];
            if (char === 0) break;
            name += String.fromCharCode(char);
          }
          names.push(name);
        }
        this.wasmInstance._free(nameBuf);

        const updatedParams = {};
        if (data.paramIds) {
          data.paramIds.forEach(id => {
            const idPtr = this.allocateString(id);
            const val = this.wasmInstance._wasm_get_param(idPtr);
            this.wasmInstance._free(idPtr);
            updatedParams[id] = val;
          });
        }
        this.port.postMessage({
          type: 'BANK_LOADED',
          names,
          params: updatedParams,
          envelopes: readActiveEnvelopes()
        });
      } else if (type === 'SAVE_BANK' && this.isReady) {
        const maxLen = 1024 * 1024;
        const bufPtr = this.wasmInstance._malloc(maxLen);
        const actualLen = this.wasmInstance._wasm_save_bank(bufPtr, maxLen);
        let jsonStr = "";
        if (actualLen > 0) {
          for (let i = 0; i < actualLen; i++) {
            jsonStr += String.fromCharCode(this.wasmInstance.HEAPU8[bufPtr + i]);
          }
        }
        this.wasmInstance._free(bufPtr);
        this.port.postMessage({
          type: 'BANK_SAVED',
          jsonStr
        });
      } else if ((type === 'RENAME_PRESET' || type === 'DELETE_PRESET' || type === 'MOVE_PRESET') && this.isReady) {
        // Bank management ops, parity with the native PresetManager. The WebUI
        // Bank Manager context menu drives these; after each op we re-read the
        // bank (names + active index) so the UI stays in sync.
        if (type === 'RENAME_PRESET') {
          const namePtr = this.allocateString(data.name || 'Init');
          this.wasmInstance._wasm_rename_preset(data.index, namePtr);
          this.wasmInstance._free(namePtr);
        } else if (type === 'DELETE_PRESET') {
          this.wasmInstance._wasm_delete_preset(data.index);
        } else if (type === 'MOVE_PRESET') {
          this.wasmInstance._wasm_move_preset(data.fromIndex, data.toIndex);
        }

        // Re-read all 64 names
        const names = [];
        const nameBuf = this.wasmInstance._malloc(256);
        for (let i = 0; i < 64; i++) {
          this.wasmInstance._wasm_get_preset_name(i, nameBuf, 256);
          let name = "";
          for (let j = 0; j < 256; j++) {
            const char = this.wasmInstance.HEAPU8[nameBuf + j];
            if (char === 0) break;
            name += String.fromCharCode(char);
          }
          names.push(name);
        }
        this.wasmInstance._free(nameBuf);

        const activeIndex = this.wasmInstance._wasm_get_current_preset_index();
        this.port.postMessage({
          type: 'BANK_UPDATED',
          names,
          activeIndex
        });
      }
    };
  }

  allocateString(str) {
    const ptr = this.wasmInstance._malloc(str.length + 1);
    for (let i = 0; i < str.length; i++) {
      this.wasmInstance.HEAPU8[ptr + i] = str.charCodeAt(i);
    }
    this.wasmInstance.HEAPU8[ptr + str.length] = 0;
    return ptr;
  }

  async initWasmModule(sampleRate) {
    try {
      this.wasmInstance = await CZ101DSP();
      this.wasmInstance._wasm_init(sampleRate);
      
      const bufferSize = 128 * 4;
      this.outPtrL = this.wasmInstance._malloc(bufferSize);
      this.outPtrR = this.wasmInstance._malloc(bufferSize);

      this.heapL = this.wasmInstance.HEAPF32.subarray(this.outPtrL / 4, (this.outPtrL / 4) + 128);
      this.heapR = this.wasmInstance.HEAPF32.subarray(this.outPtrR / 4, (this.outPtrR / 4) + 128);

      this.isReady = true;
      this.port.postMessage({ type: 'WASM_READY' });
    } catch (err) {
      console.error('Failed to init WASM in Worklet:', err);
    }
  }

  process(inputs, outputs, parameters) {
    if (!this.isReady) return true;

    const output = outputs[0];
    if (!output || output.length < 2) return true;

    const outputChannelL = output[0];
    const outputChannelR = output[1];
    const numSamples = outputChannelL.length;

    this.wasmInstance._wasm_process(this.outPtrL, this.outPtrR, numSamples);

    outputChannelL.set(this.heapL);
    outputChannelR.set(this.heapR);

    // Feed the oscilloscope ring buffer from the left channel (mono), exactly
    // like WaveformDisplay::pushBuffer takes channel 0.
    const pushed = pushScopeSamples(this.scopeBuffer, this.heapL, this.scopeWritePos);
    this.scopeBuffer = pushed.ring;
    this.scopeWritePos = pushed.writePos;

    if (++this.scopeBlockCount >= this.SCOPE_INTERVAL_BLOCKS) {
      this.scopeBlockCount = 0;
      // Copy so the main thread never sees a partially-updated buffer.
      this.port.postMessage({
        type: 'AUDIO_SCOPE',
        data: { samples: Array.from(this.scopeBuffer) }
      });
    }

    return true;
  }
}

registerProcessor('cz101-audio-worklet', CZ101AudioWorkletProcessor);
