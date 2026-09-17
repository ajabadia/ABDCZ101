import CZ101DSP from '../wasm/cz101_dsp.js';
import { initFilmstrips } from './ui/filmstrips.js';
import { CZ101AudioEngine } from './engine/cz101AudioEngine.js';
import { PARAMETER_REGISTRY, PARAM_MAP, rawToNormalized, normalizedToRaw } from './contracts/registry.gen.js';
import { FACTORY_PRESETS } from './contracts/factoryPresets.js';
import { buildLcdMenu } from './contracts/lcdMenu.js';
import { applyTheme, getTheme } from './contracts/themes.js';
import { NAVBAR_MENUS, renderItemHtml } from './contracts/navbarModel.js';
import { generateRandomPatch } from './contracts/patchRandomizer.js';
import { matchShortcut, isEditableTarget } from './contracts/shortcuts.js';
import {
  LCD_NAME_MAX_LEN as LCD_NAME_MAX_LEN_CONST,
  LCD_NAME_CHARS as LCD_NAME_CHARS_CONST,
  seedLcdName,
  formatLcdName,
  cycleLcdChar
} from './contracts/lcdName.js';
import { createCompareManager, createWriteManager } from './contracts/patchFlow.js';
import { MOD_MATRIX_EXAMPLE_SEEDS, isMatrixEmpty } from './contracts/modMatrixDefaults.js';
import {
  buildBankMenu,
  movePreset as movePresetInBank,
  deletePreset as deletePresetInBank,
  renamePreset as renamePresetInBank,
  clampMoveToPosition
} from './contracts/bankManager.js';
import {
  BANK_MODELS,
  BANK_SLOTS as BANK_SLOTS_CONST,
  createBank,
  findBank,
  getActiveBank,
  setActiveBankId,
  addBank,
  removeBank,
  renameBank,
  setBankPresets,
  setBankSnapshot,
  loadLibrary as loadBankLibrary,
  saveLibrary as saveBankLibrary,
  loadActiveBankId,
  saveActiveBankId,
  detectBankModel,
  describeModel,
  searchBank as searchBankInLibrary,
  occupiedCount,
  suggestBankName,
  emptyPresets
} from './contracts/bankLibrary.js';
import { parseSyxFile, humanizePatchNames } from './contracts/syxNames.js';
import { CZ230S_BANK_NAMES, CZ_PACK1_NAMES, factoryBankNames } from './contracts/factoryBankNames.js';
import { attachHoldRepeat } from './contracts/holdRepeat.js';
import { createEnvelopeEditor } from './ui/envelopeEditor.js';
import { createKeyboard } from './ui/keyboard.js';
import { createOscilloscope } from './ui/oscilloscope.js';
import { createModMatrix } from './ui/modMatrix.js';
import { createLcdPanel } from './ui/lcdPanel.js';
import { createBankManager } from './ui/bankManager.js';
import { createNameEditor } from './ui/nameEditor.js';
import { createBlockDrawer } from './ui/blockDrawer.js';
import { createNavbar } from './ui/navbar.js';
import { createMidiSystem } from './ui/midiSystem.js';
import { drawLcdGraphics } from './ui/lcdGraphics.js';

let audioEngine = null;
let dspModule = null;
let isAudioStarted = false;

// If running in JUCE (Standalone/VST), immediately flag for styling and auto-start
if (window.logToCpp) {
  window.logToCpp("app.js evaluation: hasJuce=" + (typeof window.__JUCE__ !== 'undefined') + " hasBackend=" + !!(window.__JUCE__ && window.__JUCE__.backend));
}
if (window.__JUCE__) {
  document.body.classList.add('is-plugin');
  setTimeout(() => {
    const btnInit = document.getElementById('btn-init-audio');
    if (btnInit) btnInit.click();
  }, 100);
}

// UI elements
const btnInit = document.getElementById('btn-init-audio');
const lcdLine1 = document.getElementById('lcd-line1');
const lcdLine2 = document.getElementById('lcd-line2');
const canvas = document.getElementById('oscilloscope');
const ctx = canvas.getContext('2d');

// Initialize LCD static graphics
drawLcdGraphics();

// --- Theme (parity with the native SkinManager: 9 themes via View menu) ---
// Apply the saved (or default Dark) theme immediately to avoid a flash of the
// wrong skin. The active theme id is tracked here so the View -> theme
// submenu can show the checkmark (the native keeps it in the menu only).
let currentThemeId = 'dark';
let navbarApi = null; // assigned at the end (navbar module); getters are lazy
let midiApi = null; // assigned later (midiSystem module); consumers use lazy getters
(() => {
  const savedTheme = (() => {
    try { return localStorage.getItem('cz101.theme'); } catch (err) { return null; }
  })();
  const initialTheme = getTheme(savedTheme);
  currentThemeId = initialTheme.id;
  applyTheme(document.documentElement, initialTheme.id);
})();

// Populate waveform choices (main + second waveform selects + windows, from the registry contract)
const waveSelects = {
  OSC1_WAVEFORM: 'OSC1_WAVEFORM',
  OSC2_WAVEFORM: 'OSC2_WAVEFORM',
  OSC1_WAVEFORM2: 'OSC1_WAVEFORM2',
  OSC2_WAVEFORM2: 'OSC2_WAVEFORM2',
  OSC1_WINDOW: 'OSC1_WINDOW',
  OSC2_WINDOW: 'OSC2_WINDOW',
  LINE_MODULATION: 'LINE_MODULATION'
};

Object.entries(waveSelects).forEach(([selectId, specId]) => {
  const el = document.getElementById(selectId);
  const spec = PARAM_MAP.get(specId);
  if (el && spec && spec.choices) {
    el.innerHTML = '';
    spec.choices.forEach((choice, idx) => {
      el.appendChild(new Option(choice, idx));
    });
  }
});

// --- Envelope Editor (extracted module: src/ui/envelopeEditor.js) ---
// The envelope editors (canvas drawing, sliders, SUS/END, COPY/PASTE, mini
// graphics) live in their own module; here we instantiate it with the live
// app references (engine getter, undo, LCD lines) and keep the same names in
// app scope so the rest of the file is untouched.
const {
  ENV_TYPES,
  ENV_TYPE_INDEX,
  ENV_ACCENTS,
  envelopeState,
  redrawEnvelopes,
  updateEnvelopeControls,
  copyEnvelopesToState,
  updateMiniGraphics,
  updateEnvelopeGraphs,
  setupEnvelopeEditors
} = createEnvelopeEditor({
  getAudioEngine: () => audioEngine,
  getPushUndo: () => navbarApi && navbarApi.pushUndo,
  lcdLine1,
  lcdLine2
});

// Initial draw at startup (before the engine is started) so the envelope
// graphs are visible inside the drawers.
setupEnvelopeEditors();
// --- Dual gateway: official JUCE 8 bridge or standalone WASM engine ---
function sendParameter(paramId, normalizedValue) {
  const backend = window.getJuceBackend ? window.getJuceBackend() : ((window.__JUCE__ && window.__JUCE__.backend) || (window.Juce && window.Juce.backend));
  if (backend) {
    if (typeof backend.setParameterValue === 'function') backend.setParameterValue(paramId, normalizedValue);
    if (typeof backend.emitEvent === 'function') backend.emitEvent('setParameterValue', { id: paramId, value: normalizedValue });
  } else if (audioEngine) {
    audioEngine.setParameter(paramId, normalizedValue);
  }
  // Keep the LCD value store in sync for parameters without a drawer control.
  if (!document.getElementById(paramId)) {
    lcdParamRawValues[paramId] = normalizedToRaw(paramId, normalizedValue);
  }
  requestSysexDisplayUpdate();
}
let sysexDisplayTimeout = null;
function requestSysexDisplayUpdate() {
  const backend = window.getJuceBackend ? window.getJuceBackend() : ((window.__JUCE__ && window.__JUCE__.backend) || (window.Juce && window.Juce.backend));
  if (backend && typeof backend.emitEvent === 'function') {
    clearTimeout(sysexDisplayTimeout);
    sysexDisplayTimeout = setTimeout(() => {
      backend.emitEvent('requestSysex', {});
    }, 150);
  } else if (typeof audioEngine !== 'undefined' && audioEngine) {
    clearTimeout(sysexDisplayTimeout);
    sysexDisplayTimeout = setTimeout(() => {
      audioEngine.saveSysEx('__UI_SYSEX_DISPLAY__');
    }, 150);
  }
}

// Apply a normalized value coming from the host/engine to the visual control (inverse sync)
function applyParameterToUI(paramId, normalizedValue) {
  const spec = PARAM_MAP.get(paramId);
  if (!spec) return;
  const inputEl = document.getElementById(paramId);
  if (!inputEl) {
    // No drawer control (e.g. Key Follow modes): keep the raw value for the
    // LCD so preset loads stay in sync with what the LCD displays.
    lcdParamRawValues[paramId] = normalizedToRaw(paramId, normalizedValue);
    return;
  }

  const normVal = Math.min(1.0, Math.max(0.0, normalizedValue));

  // Toggle buttons
  if (inputEl.tagName === 'BUTTON') {
    const isActive = normVal >= 0.5;
    inputEl.classList.toggle('active', isActive);
    return;
  }

  // Use the registry conversion (skew-aware for the APVTS cutoffs) instead of
  // inline linear math, so host automation / preset loads place the controls
  // exactly like the native plugin. normalizedToRaw rounds int/choice/bool.
  const rawVal = normalizedToRaw(paramId, normVal);

  if (spec.skew && spec.skew !== 1.0) {
    inputEl.value = normVal;
  } else {
    inputEl.value = rawVal;
  }

  // Update value display span
  const valEl = document.getElementById(`val-${paramId}`);
  if (valEl) {
    let textVal = spec.type === 'choice' ? (spec.choices[rawVal] ?? rawVal) : rawVal;
    if (typeof textVal === 'number' && spec.unit === 'Hz') {
      textVal = Math.round(textVal);
    }
    valEl.innerText = `${textVal} ${spec.unit || ''}`.trim();
  }

  // Notify custom UI wrappers (like filmstrips) that the value changed programmatically
  inputEl.dispatchEvent(new CustomEvent('filmstrip-update'));

  if (paramId.startsWith('DCA_') || paramId.startsWith('DCW_')) {
    updateEnvelopeGraphs();
  }

  if (paramId === 'OSC1_WAVEFORM' || paramId === 'OSC1_WAVEFORM2' || paramId === 'OSC1_WINDOW' ||
      paramId === 'OSC2_WAVEFORM' || paramId === 'OSC2_WAVEFORM2' || paramId === 'OSC2_WINDOW' ||
      paramId === 'LFO_WAVE') {
    updateMiniGraphics();
  }

  requestSysexDisplayUpdate();
}

// Initial button states from registry defaults (e.g. PROTECT_SWITCH / HARDWARE_NOISE default ON)
PARAMETER_REGISTRY.parameters.forEach(p => {
  const inputEl = document.getElementById(p.id);
  if (inputEl && inputEl.tagName === 'BUTTON') {
    inputEl.classList.toggle('active', p.default >= 0.5);
  }
});

// --- On-screen keyboard (extracted module: src/ui/keyboard.js) ---
// The 49-key CZ-101 keyboard (build + vintage wear + note handlers) lives in
// its own module; instantiate with the live app references. The same noteOn /
// noteOff / triggerMidiActivity wiring as before, now module-scoped.
createKeyboard({
  getAudioEngine: () => audioEngine,
  lcdLine1,
  lcdLine2,
  triggerMidiActivity: (...args) => midiApi && midiApi.triggerMidiActivity(...args)
});
// Visual feedback and parameter linkage
function setLCDParam(id, val) {
  const spec = PARAM_MAP.get(id);
  if (!spec) return;

  setLcdCompareMode(false);
  exitWriteMode();
  compareManager.clear(); // a manual edit abandons any active A/B compare

  const idx = getMenuParameters().findIndex(p => p.id === id);
  if (idx !== -1) {
    // Position within the CURRENTLY navigable list: in SET (System) mode the
    // keypad is locked to the SYSTEM page, so the full-list index can point
    // past the end of the active list (undefined param -> crash in updateLCD).
    const activeIdx = activeMenuParameters().findIndex(p => p.id === id);
    if (activeIdx !== -1) setLcdMenuIndex(activeIdx);
    // A param that is not reachable on the current page (e.g. a non-system
    // control edited while SET locks the LCD to SYSTEM) keeps the page.
  } else {
    // Show custom temp parameter
    if (lcdLine1 && lcdLine2) {
      lcdLine1.innerText = `${spec.name}`.toUpperCase();
      lcdLine2.innerText = `${val} ${spec.unit || ''}`.toUpperCase().trim();
    }
    resetLCDInactivity();
    return;
  }

  updateLCD();
  resetLCDInactivity();
}

// Hook up parameters from registry (sliders, selects and toggle buttons)
PARAMETER_REGISTRY.parameters.forEach(p => {
  const inputEl = document.getElementById(p.id);
  if (!inputEl) return;

  const updateBadge = (rawVal, textVal) => {
    const valBadge = document.getElementById(`val-${p.id}`);
    if (valBadge) {
      valBadge.innerText = `${textVal} ${p.unit || ''}`.trim();
    }
  };

  if (inputEl.tagName === 'BUTTON') {
    inputEl.addEventListener('click', () => {
      inputEl.classList.toggle('active');
      const isActive = inputEl.classList.contains('active');
      setLCDParam(p.id, isActive ? 'ON' : 'OFF');
      sendParameter(p.id, isActive ? 1.0 : 0.0);
    });
    return;
  }

  const updateVal = () => {
    let rawVal;
    let normVal;
    if (p.skew && p.skew !== 1.0) {
      normVal = parseFloat(inputEl.value);
      rawVal = normalizedToRaw(p.id, normVal);
    } else {
      rawVal = parseFloat(inputEl.value);
      normVal = rawToNormalized(p.id, rawVal);
    }
    let textVal = p.type === 'choice' ? (p.choices[rawVal] ?? rawVal) : rawVal;
    if (typeof textVal === 'number' && p.unit === 'Hz') {
      textVal = Math.round(textVal);
    }
    updateBadge(rawVal, textVal);
    setLCDParam(p.id, textVal);
    sendParameter(p.id, normVal);

    if (p.id.startsWith('DCA_') || p.id.startsWith('DCW_')) {
      updateEnvelopeGraphs();
    }
    if (p.id.startsWith('OSC1_') || p.id.startsWith('OSC2_') || p.id === 'LFO_WAVE' || p.id === 'LINE_SELECT') {
      updateMiniGraphics();
    }
  };

  inputEl.addEventListener('input', updateVal);
  inputEl.addEventListener('change', updateVal);
});

// Listen for host-side parameter automation (JUCE embedded gateway)
const setupJuceListeners = () => {
  const backend = window.getJuceBackend ? window.getJuceBackend() : ((window.__JUCE__ && window.__JUCE__.backend) || (window.Juce && window.Juce.backend));
  if (!backend || !backend.addEventListener) return;

  backend.addEventListener('parameterChanged', (e) => {
    const detail = e && e.detail ? e.detail : e;
    if (detail && detail.id !== undefined) {
      applyParameterToUI(detail.id, detail.value);
    }
  });

  backend.addEventListener('presetLoaded', (e) => {
    const detail = e && e.detail ? e.detail : e;
    if (detail && detail.index !== undefined) {
      if (selectPresetSlot && selectPresetSlot.options[detail.index]) {
        selectPresetSlot.value = detail.index;
      }
      if (inputPresetName && detail.name) {
        inputPresetName.value = detail.name;
      }
      if (detail.params) {
        // Reset unmentioned parameters to their defaults
        PARAMETER_REGISTRY.parameters.forEach(p => {
          if (detail.params[p.id] === undefined) {
            applyParameterToUI(p.id, p.default !== undefined ? (p.skew ? p.default : rawToNormalized(p.id, p.default)) : 0.0);
          }
        });
        for (const [id, val] of Object.entries(detail.params)) {
          applyParameterToUI(id, val);
        }
      }
      if (detail.envelopes) {
        copyEnvelopesToState(detail.envelopes);
      }
      if (detail.sysexHex) {
        const el = document.getElementById('sysex-hex-content');
        if (el) el.innerText = detail.sysexHex;
      }
      syncUiAfterParamLoad();
      updateMiniGraphics();
    }
  });

  backend.addEventListener('sysexDump', (e) => {
    const hex = typeof e === 'string' ? e : (e && (e.sysexHex || e.detail)) || '';
    if (hex) {
      const el = document.getElementById('sysex-hex-content');
      if (el) el.innerText = hex;
    }
  });
};
setupJuceListeners();
setTimeout(setupJuceListeners, 300); // Retry after backend handshake completes

// --- Free Modulation Matrix (extracted module: src/ui/modMatrix.js) ---
// The 8-slot Source→Dest→Depth matrix (DOM build, ON/OFF badges, AUTH K-TRACK
// LCD readout, Modern example seeds) lives in its own module; instantiate with
// the app-scope references. The returned helpers keep the same names in app
// scope so the call sites below are untouched.
const {
  syncModSlotBadges,
  getAuthKeyTrackLcdValue,
  applyModernMatrixSeeds
} = createModMatrix({
  getSendParameter: () => sendParameter,
  getRawToNormalized: () => rawToNormalized,
  getLcdMenuIndex: () => getLcdMenuIndex(),
  getActiveMenuParameters: () => activeMenuParameters,
  getUpdateLCD: () => updateLCD,
  getCurrentOperationMode: () => currentOperationMode
});

// --- Oscilloscope (extracted module: src/ui/oscilloscope.js) ---
// The real-audio scope (rAF loop, grid, flat line while offline) lives in its
// own module; instantiate with the live engine/audio-started references.
createOscilloscope({
  canvas,
  ctx,
  getAudioEngine: () => audioEngine,
  getIsAudioStarted: () => isAudioStarted
});

const inputSysex = document.getElementById('input-sysex');

// Init audio engine on button click
btnInit.addEventListener('click', async () => {
  if (isAudioStarted) return;

  if (window.__JUCE__) {
    // JUCE Native Mode (No WASM)
    isAudioStarted = true;
    btnInit.innerText = 'C++ ENGINE ONLINE';
    btnInit.disabled = true;
    btnInit.classList.add('connected');
    
    lcdLine1.innerText = 'CZ-101 C++ ONLINE';
    lcdLine2.innerText = 'READY TO PLAY';
    
    enableBankUI();
    return; // Skip WASM init
  }

  lcdLine1.innerText = 'INITIALIZING WASM...';
  
  try {
    audioEngine = new CZ101AudioEngine();
    await audioEngine.initialize();

    isAudioStarted = true;
    btnInit.innerText = 'AUDIO ONLINE';
    btnInit.disabled = true;
    btnInit.classList.add('connected');

    // Audio output device selector: (re)scan now that we have a user gesture
    // (device labels), restore the last chosen output from storage, and route
    // the AudioContext to it — parity with the native Audio Settings window.
    if (midiApi) midiApi.populateAudioOutputDevices();
    const savedOutput = localStorage.getItem(midiApi ? midiApi.AUDIO_OUTPUT_STORAGE_KEY : 'cz101.audioOutputDevice');
    const audioOutputSelectEl = document.getElementById('audio-output-device');
    if (savedOutput && audioOutputSelectEl) audioOutputSelectEl.value = savedOutput;
    if (midiApi) midiApi.applyAudioOutputDevice(audioOutputSelectEl ? audioOutputSelectEl.value : 'default');
    
    // Enable controls
    lcdLine1.innerText = 'CZ-101 WASM ONLINE';
    lcdLine2.innerText = 'READY TO PLAY';

    // The bank load fired during the click (listener below) can race with
    // initialize(): loadSysEx silently drops when the workletNode is not built
    // yet (cached WASM/fetch resolves first on reloads). Reload the active bank
    // now that the engine is ready — idempotent, and onBankLoaded then restores
    // the last visited preset.
    enableBankUI();
    
    // Set initial values for parameters from controls
    PARAMETER_REGISTRY.parameters.forEach(p => {
      const inputEl = document.getElementById(p.id);
      if (!inputEl) return;
      const rawVal = inputEl.tagName === 'BUTTON'
        ? (inputEl.classList.contains('active') ? 1 : 0)
        : parseFloat(inputEl.value);
      sendParameter(p.id, rawToNormalized(p.id, rawVal));
    });
    updateEnvelopeGraphs();

    // Wire up SysEx loaded feedback to update sliders
    audioEngine.onSysExLoaded = (params, envelopes) => {
      lcdLine1.innerText = "PRESET LOADED";
      lcdLine2.innerText = "SYSEX IMPORT SUCCESS";
      
      Object.keys(params).forEach(id => {
        applyParameterToUI(id, params[id]);
      });

      if (envelopes) {
        copyEnvelopesToState(envelopes);
      }
      applyModernMatrixSeeds();
      requestSysexDisplayUpdate();
    };

    // The envelope editors were wired at startup (setupEnvelopeEditors);
    // just refresh the graphs now that the engine is live.
    redrawEnvelopes();
    requestSysexDisplayUpdate();

    // Optional: connect the Web MIDI API to the standalone WASM MIDI processor
    // so an external controller (notes, pitch bend, sustain, CC) drives the
    // engine. Only MIDI_CH-matching channels are heard (default Ch 1).
    try {
      if (navigator.requestMIDIAccess) {
        const midiAccess = await navigator.requestMIDIAccess();
        let midiInputs = 0;
        midiAccess.inputs.forEach(input => {
          midiInputs++;
          input.onmidimessage = (event) => {
            if (event.data && event.data.length) {
              if (midiApi) midiApi.routeMidi(event.data);
            }
          };
        });
        if (midiInputs > 0) {
          console.log(`[MIDI] ${midiInputs} input(s) connected to the WASM engine`);
        }
        // Re-connect inputs that appear later
        midiAccess.onstatechange = (e) => {
          if (e.port.type === 'input' && e.port.state === 'connected') {
            e.port.onmidimessage = (event) => {
              if (event.data && event.data.length) {
                if (midiApi) midiApi.routeMidi(event.data);
              }
            };
          }
        };
      }
    } catch (midiErr) {
      console.warn('[MIDI] Web MIDI API unavailable:', midiErr.message || midiErr);
    }

  } catch (err) {
    console.error('Audio initialization failed:', err);
    lcdLine1.innerText = 'INIT FAILED!';
    lcdLine2.innerText = 'CHECK CONSOLE ERROR';
  }
});

// Hook up load buttons
inputSysex.addEventListener('change', (e) => {
  const file = e.target.files[0];
  if (!file) return;

  const reader = new FileReader();
  reader.onload = (event) => {
    const arrayBuffer = event.target.result;
    const bytes = new Uint8Array(arrayBuffer);
    
    lcdLine1.innerText = "PARSING SYSEX...";
    lcdLine2.innerText = file.name;
    
    if (audioEngine) {
      audioEngine.loadSysEx(bytes, PARAMETER_REGISTRY.parameters.map(p => p.id));
    }
  };
  reader.readAsArrayBuffer(file);
});

// Preset Bank Manager Setup
const selectPresetSlot = document.getElementById('select-preset-slot');
const btnPresetPrev = document.getElementById('btn-preset-prev');
const btnPresetNext = document.getElementById('btn-preset-next');
const inputPresetName = document.getElementById('input-preset-name');
const inputBankFile = document.getElementById('input-bank-file');

// Populate 64 slots
for (let i = 0; i < 64; i++) {
  const opt = new Option(`Slot ${i + 1}: (Empty)`, i);
  selectPresetSlot.appendChild(opt);
}

// Function to format factory presets array as bank JSON for C++ loadBank
const makeDefaultBankJson = () => {
  const presets = FACTORY_PRESETS.map(p => {
    const makeDcaEnv = () => (p.dcaEnv ? p.dcaEnv : {
      rates: [9900, 5000, 5000, 5000, 5000, 5000, 5000, 5000],
      levels: [10000, 8000, 8000, 0, 0, 0, 0, 0],
      sustainPoint: 2,
      endPoint: 3
    });
    const makeDcwEnv = () => (p.dcwEnv ? p.dcwEnv : {
      rates: [9900, 5000, 5000, 5000, 5000, 5000, 5000, 5000],
      levels: [10000, 8000, 8000, 0, 0, 0, 0, 0],
      sustainPoint: 2,
      endPoint: 3
    });
    const makePitchEnv = () => (p.pitchEnv ? p.pitchEnv : {
      rates: [9900, 9900, 9900, 9900, 9900, 9900, 9900, 9900],
      levels: [5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000],
      sustainPoint: 0,
      endPoint: 0
    });
    return {
      name: p.name,
      params: p.params,
      dcwEnv: makeDcwEnv(),
      dcaEnv: makeDcaEnv(),
      pitchEnv: makePitchEnv(),
      dcwEnv2: makeDcwEnv(),
      dcaEnv2: makeDcaEnv(),
      pitchEnv2: makePitchEnv()
    };
  });
  return JSON.stringify({ version: 1, presets });
};

// Enable Bank UI after initialization
const enableBankUI = () => {
  selectPresetSlot.disabled = false;
  inputPresetName.disabled = false;

  // Load the bank that is active in the library (defaults to the factory bank)
  // so the Bank Manager / header stay in sync with what the engine holds.
  if (audioEngine) {
    loadBankIntoEngine(activeBank());
  }
};

// A preset/bank load applies OPERATION_MODE through applyParameterToUI (which
// sets the select value WITHOUT dispatching events), so the mode-driven UI
// (LCD menu rebuild, panel gating, LCD theme) would stay on the previous mode.
// Sync it here after applying params, parity with the native
// LCDStateManager::onPageChanged + PluginEditor per-mode rebuild.
const syncUiAfterParamLoad = () => {
  syncModePanels();
  syncLCDMode();
  rebuildLcdMenu();
  applyModernMatrixSeeds();
};

// Wire up bank management callbacks to AudioEngine
btnInit.addEventListener('click', () => {
  if (!audioEngine) return;
  enableBankUI();

  audioEngine.onPresetLoaded = (params, index, envelopes) => {
    // In Bank Browse mode the LCD stays on bank/patch names (the engine echo
    // would fight the scrolling readout).
    if (!getLcdBnkMode()) {
      lcdLine1.innerText = "SLOT LOADED";
      const name = selectPresetSlot.options[index].text.split(': ')[1] || "Init User";
      lcdLine2.innerText = name;
      // Long preset names scroll on the LCD in normal program mode too.
      if (!getLcdCompareMode() && !getLcdSetupMode() && !getLcdMdlMode() && !writeManager.isActive) {
        normalLine2.set(name.toUpperCase());
      }
    }
    const name = selectPresetSlot.options[index].text.split(': ')[1] || "Init User";
    inputPresetName.value = name;
    selectPresetSlot.value = index;

    // Update UI controls
    Object.keys(params).forEach(id => {
      applyParameterToUI(id, params[id]);
    });

    if (envelopes) {
      copyEnvelopesToState(envelopes);
    }
    syncUiAfterParamLoad();
    requestSysexDisplayUpdate();
    // Bank Browse readout stays current after an engine preset load.
    if (getLcdBnkMode()) renderBnkLcd();
  };

  audioEngine.onPresetSaved = (index, name) => {
    lcdLine1.innerText = "PRESET SAVED";
    lcdLine2.innerText = name;
    selectPresetSlot.options[index].text = `Slot ${index + 1}: ${name}`;
  };

  audioEngine.onBankLoaded = (names, params, envelopes) => {
    // In Bank Browse mode the LCD stays on bank/patch names.
    if (!getLcdBnkMode()) {
      lcdLine1.innerText = "BANK IMPORTED";
      lcdLine2.innerText = "SUCCESS";
    }

    // The engine labels every CZ-101 patch "Imported Preset" (the format stores
    // no names). For factory banks we prefer the library's human names; for user
    // banks the engine names are the real ones.
    const b = activeBank();
    const useLibraryNames = b && b.readOnly && occupiedCount(b) > 0;
    const displayNames = useLibraryNames ? b.presets.map(p => p.name) : names;

    // Update dropdown titles
    displayNames.forEach((name, idx) => {
      if (idx < 64) {
        selectPresetSlot.options[idx].text = `Slot ${idx + 1}: ${name}`;
      }
    });
    
    // Restore the last visited preset when the SAME bank is active again;
    // a different bank starts at slot 0. Selecting a non-zero slot reloads the
    // preset from the engine so the sound matches the name shown.
    const saved = loadSavedPresetSlot();
    const savedBank = activeBank();
    const slotIndex = (saved && savedBank && saved.bankId === savedBank.id && saved.slot >= 0 && saved.slot < selectPresetSlot.options.length)
      ? saved.slot : 0;
    selectPresetSlot.value = slotIndex;
    inputPresetName.value = displayNames[slotIndex] || displayNames[0] || "Init User";
    if (slotIndex > 0 && audioEngine) {
      audioEngine.loadPreset(slotIndex, PARAMETER_REGISTRY.parameters.map(p => p.id));
    }

    // Update UI controls
    Object.keys(params).forEach(id => {
      applyParameterToUI(id, params[id]);
    });

    if (envelopes) {
      copyEnvelopesToState(envelopes);
    }

    // Sync the loaded bank into the library (names + user-bank snapshot).
    syncBankNamesToLibrary(displayNames);
    requestSysexDisplayUpdate();
    syncUiAfterParamLoad();
    // The slot is resolved above (remembered or 0): refresh the BNK readout.
    if (getLcdBnkMode()) renderBnkLcd();
  };

  // Bank Manager edits (rename/delete/move) come back with the full name list
  // and the engine's active index; keep the dropdown + Bank Manager list in sync.
  audioEngine.onBankUpdated = (names, activeIndex) => {
    names.forEach((name, idx) => {
      if (idx < selectPresetSlot.options.length) {
        selectPresetSlot.options[idx].text = `Slot ${idx + 1}: ${name}`;
      }
    });
    if (activeIndex !== undefined && activeIndex < selectPresetSlot.options.length) {
      selectPresetSlot.value = activeIndex;
    }
    syncBankNamesToLibrary(names);
  };

  audioEngine.onBankSaved = (jsonStr) => {
    const blob = new Blob([jsonStr], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'cz101_user_bank.json';
    a.click();
    URL.revokeObjectURL(url);
    lcdLine1.innerText = "BANK EXPORTED";
    lcdLine2.innerText = "DOWNLOAD STARTED";
  };

  audioEngine.onSysExSaved = (sysexBytes, name) => {
    if (name === '__UI_SYSEX_DISPLAY__') {
      if (sysexBytes && sysexBytes.length > 0) {
        const hex = Array.from(sysexBytes).map(b => b.toString(16).padStart(2, '0').toUpperCase()).join(' ');
        const el = document.getElementById('sysex-hex-content');
        if (el) el.innerText = hex;
      }
      return;
    }

    if (!sysexBytes || sysexBytes.length === 0) {
      lcdLine1.innerText = "EXPORT FAILED";
      lcdLine2.innerText = "NO SYSEX DATA";
      return;
    }
    const cleanName = name.replace(/[^a-zA-Z0-9_\-]/g, '_');
    const blob = new Blob([new Uint8Array(sysexBytes)], { type: 'application/octet-stream' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `${cleanName}.syx`;
    a.click();
    URL.revokeObjectURL(url);
    lcdLine1.innerText = "SYSEX EXPORTED";
    lcdLine2.innerText = `${name}.syx`;
  };

  const btnCopySysex = document.getElementById('btn-copy-sysex');
  if (btnCopySysex) {
    btnCopySysex.addEventListener('click', () => {
      const el = document.getElementById('sysex-hex-content');
      if (el && el.innerText) {
        navigator.clipboard.writeText(el.innerText).then(() => {
          const originalText = btnCopySysex.innerText;
          btnCopySysex.innerText = "COPIED!";
          setTimeout(() => { btnCopySysex.innerText = originalText; }, 1500);
        }).catch(err => {
          console.error("Failed to copy sysex:", err);
        });
      }
    });
  }
});

// Shared preset-loading path: the slot select, the prev/next buttons and the
// LCD slot navigation all go through here (parity with the native
// PresetBrowser::selectPreset, which loadPreset + notifies the editor).
// ── Last visited preset (native-like memory, tied to the active bank) ──
// The last bank is already restored across reloads; remember the preset slot
// too so the session picks up exactly where it left off. Stored per bank so a
// different bank still starts at slot 0.
const PRESET_SLOT_STORAGE_KEY = 'cz101.activePresetSlot';

const loadSavedPresetSlot = () => {
  try {
    const raw = localStorage.getItem(PRESET_SLOT_STORAGE_KEY);
    if (!raw) return null;
    const parsed = JSON.parse(raw);
    return (parsed && typeof parsed.slot === 'number') ? parsed : null;
  } catch (err) { return null; }
};

const saveActivePresetSlot = (slot) => {
  const lib = getBankLib();
  const bankId = (lib && lib.activeBankId) || '';
  try { localStorage.setItem(PRESET_SLOT_STORAGE_KEY, JSON.stringify({ bankId, slot })); } catch (err) { /* ignore */ }
};

const loadPresetSlot = (index) => {
  if (window.logToCpp) window.logToCpp(`loadPresetSlot: ${index}`);
  if (!selectPresetSlot.options[index]) return;
  selectPresetSlot.value = index;
  saveActivePresetSlot(index);
  const backend = window.getJuceBackend ? window.getJuceBackend() : ((window.__JUCE__ && window.__JUCE__.backend) || (window.Juce && window.Juce.backend));
  if (backend) {
    if (typeof backend.loadPreset === 'function') backend.loadPreset(index);
    if (typeof backend.emitEvent === 'function') backend.emitEvent('loadPreset', index);
  } else if (audioEngine) {
    audioEngine.loadPreset(index, PARAMETER_REGISTRY.parameters.map(p => p.id));
  }
};

// Bind UI actions
selectPresetSlot.addEventListener('change', (e) => {
  loadPresetSlot(parseInt(e.target.value));
});

// Prev/Next buttons, same as the native PresetBrowser: step one slot and clamp
// at the bank edges (current > 0 / current < numItems - 1).
btnPresetPrev.addEventListener('click', () => {
  const current = parseInt(selectPresetSlot.value);
  if (current > 0) loadPresetSlot(current - 1);
});

btnPresetNext.addEventListener('click', () => {
  const current = parseInt(selectPresetSlot.value);
  if (current < selectPresetSlot.options.length - 1) loadPresetSlot(current + 1);
});

inputBankFile.addEventListener('change', (e) => {
  const file = e.target.files[0];
  if (!file) return;

  const reader = new FileReader();
  reader.onload = (event) => {
    const jsonStr = event.target.result;
    lcdLine1.innerText = "IMPORTING BANK...";
    lcdLine2.innerText = file.name;
    if (audioEngine) {
      audioEngine.loadBank(jsonStr, PARAMETER_REGISTRY.parameters.map(p => p.id));
    }
  };
  reader.readAsText(file);
});

// ── Drag & drop (native PluginEditor::filesDropped parity) ──
// The native accepts .syx (routed to handleSysEx) and .json (routed to
// loadBank). Drops behave exactly like the file pickers: .json loads the bank
// into the engine; .syx/.sysex goes through the Bank Manager import flow so a
// bank also lands in the library tagged with its model (single patches still
// just load into the engine).
const DROP_EXT_RE = /\.(syx|sysex|json)$/i;

window.addEventListener('dragover', (e) => {
  const hasFiles = Array.from(e.dataTransfer?.types || []).includes('Files');
  if (!hasFiles) return;
  e.preventDefault();
  document.body.classList.add('drag-over');
});

window.addEventListener('dragleave', (e) => {
  if (!e.relatedTarget) document.body.classList.remove('drag-over');
});

window.addEventListener('drop', (e) => {
  document.body.classList.remove('drag-over');
  const files = Array.from(e.dataTransfer?.files || []);
  const file = files.find(f => DROP_EXT_RE.test(f.name));
  if (!file) return;
  e.preventDefault();

  if (/\.(syx|sysex)$/i.test(file.name)) {
    // Bank Manager import flow: parse names in the engine, then create a user
    // bank tagged with the detected model (syncBankNamesToLibrary does it).
    if (!audioEngine) { if (navbarApi) navbarApi.flashLcd('INIT AUDIO', 'FIRST'); return; }
    const reader = new FileReader();
    reader.onload = (event) => {
      pendingSyxImport = { name: file.name.replace(/\.(syx|sysex)$/i, '') };
      lcdLine1.innerText = 'PARSING SYSEX...';
      lcdLine2.innerText = file.name;
      audioEngine.loadSysEx(new Uint8Array(event.target.result), PARAMETER_REGISTRY.parameters.map(p => p.id));
    };
    reader.readAsArrayBuffer(file);
  } else {
    const reader = new FileReader();
    reader.onload = (event) => {
      lcdLine1.innerText = 'IMPORTING BANK...';
      lcdLine2.innerText = file.name;
      if (audioEngine) {
        audioEngine.loadBank(event.target.result, PARAMETER_REGISTRY.parameters.map(p => p.id));
      }
    };
    reader.readAsText(file);
  }
});

// --- Enhanced Bank Manager (extracted module: src/ui/bankManager.js) ---
// The multi-bank library (model-tagged banks, chips, slot grid, context menu,
// import/export, factory warm-up) lives in its own module; here we instantiate
// it with the live app references. bankLib is owned by the module (getBankLib
// getter); the load/sync helpers used by the engine callbacks come back
// directly.
const {
  getBankLib,
  activeBank,
  activateBank,
  loadBankIntoEngine,
  syncBankNamesToLibrary,
  renderBankManager,
  getBankManagerModal
} = createBankManager({
  getAudioEngine: () => audioEngine,
  getFlashLcd: () => navbarApi && navbarApi.flashLcd,
  getOpenNameEditor: () => openNameEditor,
  selectPresetSlot,
  lcdLine1,
  lcdLine2,
  makeDefaultBankJson,
  loadPresetSlot
});


// --- Name Editor overlay (extracted module: src/ui/nameEditor.js) ---
const { openNameEditor, closeNameEditor } = createNameEditor();

// ─── MIDI subsystem (extracted module: src/ui/midiSystem.js) ---
// The activity LED, Web MIDI connection badge, audio-output device chooser,
// virtual pitch/mod wheels and the MIDI Learn menu live in their own module.
// Instantiated before the LCD/navbar consumers that need triggerMidiActivity /
// routeMidi via the lazy midiApi getters above.
midiApi = createMidiSystem({
  getAudioEngine: () => audioEngine,
  getIsAudioStarted: () => isAudioStarted,
  sendParameter,
  applyParameterToUI,
  lcdLine1,
  lcdLine2
});

// --- LCD panel (extracted module: src/ui/lcdPanel.js) ---
// The keypad state machine (Normal / Parameter Edit / Compare / Write / BNK /
// MDL), the char-by-char LCD scroller, the position badge and the SET System
// Mode live in their own module; here we instantiate it with the live app
// references. Mutable LCD state that this file reads/writes (setLCDParam,
// onPresetLoaded, the mod-matrix wiring) comes back through get/set accessors.
const {
  updateLCD,
  resetLCDInactivity,
  renderBnkLcd,
  syncLCDMode,
  syncModePanels,
  rebuildLcdMenu,
  toggleSetupMode,
  exitWriteMode,
  activeMenuParameters,
  currentOperationMode,
  compareManager,
  writeManager,
  normalLine2,
  lcdParamRawValues,
  getLcdMenuIndex,
  setLcdMenuIndex,
  getLcdCompareMode,
  setLcdCompareMode,
  getLcdSetupMode,
  getLcdBnkMode,
  getLcdMdlMode,
  getMenuParameters
} = createLcdPanel({
  lcdLine1,
  lcdLine2,
  selectPresetSlot,
  inputPresetName,
  getAudioEngine: () => audioEngine,
  getIsAudioStarted: () => isAudioStarted,
  getPushUndo: () => navbarApi && navbarApi.pushUndo,
  getBankLib: () => getBankLib(),
  sendParameter,
  applyParameterToUI,
  openNameEditor,
  activeBank,
  activateBank,
  loadSavedPresetSlot,
  loadPresetSlot,
  getAuthKeyTrackLcdValue,
  applyModernMatrixSeeds,
  envelopeState,
  ENV_TYPES,
  updateEnvelopeControls,
  redrawEnvelopes
});


// ─── Slide-out settings drawer (extracted module: src/ui/blockDrawer.js) ---
const { openBlockDrawer, closeBlockDrawer, toggleBlockDrawer } = createBlockDrawer({
  redrawEnvelopes
});

// ─── Navbar (extracted module: src/ui/navbar.js) ---
// The menu bar, undo/redo, keyboard shortcuts and header buttons live in their
// own module. Instantiated LAST: earlier factories receive pushUndo/flashLcd
// via lazy navbarApi getters, and the navbar needs their return values here.
navbarApi = createNavbar({
  getAudioEngine: () => audioEngine,
  getCurrentThemeId: () => currentThemeId,
  setCurrentThemeId: (v) => { currentThemeId = v; },
  lcdLine1,
  lcdLine2,
  inputBankFile,
  inputSysex,
  inputPresetName,
  selectPresetSlot,
  updateLCD,
  syncLCDMode,
  toggleSetupMode,
  envelopeState,
  ENV_TYPES,
  updateEnvelopeControls,
  redrawEnvelopes,
  copyEnvelopesToState,
  openNameEditor,
  makeDefaultBankJson,
  renderBankManager,
  getBankManagerModal,
  openBlockDrawer,
  sendParameter,
  applyParameterToUI,
  triggerMidiActivity: (...args) => midiApi && midiApi.triggerMidiActivity(...args)
});

// Upgrade sliders to filmstrip faders
initFilmstrips();

// Setup Auto-Scaling for the UI
function scaleUI() {
  const container = document.querySelector('.synth-container');
  if (!container) return;
  const windowW = window.innerWidth;
  const windowH = window.innerHeight;
  const targetW = 1409;
  const targetH = 768; // 1409 / (2818/1536) ≈ 768

  // Calculate scale to fit within the window, maintaining aspect ratio
  const scaleW = windowW / targetW;
  const scaleH = windowH / targetH;
  const scale = Math.min(scaleW, scaleH);

  container.style.transform = `scale(${scale})`;
}
window.addEventListener('resize', scaleUI);
scaleUI();
