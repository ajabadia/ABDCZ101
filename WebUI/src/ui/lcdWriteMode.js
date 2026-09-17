// Write + Compare modes (extracted from lcdPanel.js).
//
// Compare mirrors the native PresetManager::setCompareMode: entering Compare
// snapshots the current edit buffer and loads the SAVED version of the preset
// from the bank; exiting (or the inactivity timeout) restores the edits. Needs
// the WASM engine; without it the LCD still shows the compare indication.
//
// Write mode: WRT -> pick destination slot (▲/▼) -> WRT -> edit the preset name
// (◀/▶ move the cursor, ▲/▼ cycle the character) -> WRT commits a real save into
// the WASM bank via audioEngine.savePreset (the same path the Preset Bank
// Manager uses). Both state machines live in ./contracts/patchFlow.js so they
// can be tested without a browser; here we inject the DOM/engine glue.
//
// Factory pattern: receives the shared LCD ctx (DOM refs, engine getters and
// lazy getters to the sibling modes / badge) and returns the Write+Compare API.
import { PARAMETER_REGISTRY, rawToNormalized } from '../contracts/registry.gen.js';
import {
  LCD_NAME_MAX_LEN as LCD_NAME_MAX_LEN_CONST,
  LCD_NAME_CHARS as LCD_NAME_CHARS_CONST,
  seedLcdName,
  formatLcdName,
  cycleLcdChar
} from '../contracts/lcdName.js';
import { createCompareManager, createWriteManager } from '../contracts/patchFlow.js';
import { measureLcdChars } from './lcdScroller.js';

export function createWriteMode(ctx) {
  const {
    lcdLine1,
    lcdLine2,
    selectPresetSlot,
    inputPresetName,
    getAudioEngine,
    getIsAudioStarted,
    sendParameter,
    applyParameterToUI,
    envelopeState,
    ENV_TYPES,
    updateEnvelopeControls,
    redrawEnvelopes,
    // Lazy cross-mode getters (resolved at call time).
    getExitSetupMode,
    getUpdateLCD,
    getResetInactivity,
    getStopNormalLcdScroll,
    getShowBnkBadge,
    getHideBnkBadge,
    getRenderBnkBadge,
    getLcdCompareMode,
    setLcdCompareMode,
  } = ctx;

  // Wrappers keep the original internal names so call sites read naturally.
  const exitSetupMode = () => getExitSetupMode()();
  const updateLCD = () => getUpdateLCD()();
  const resetLCDInactivity = () => getResetInactivity()();
  const stopNormalLcdScroll = () => getStopNormalLcdScroll()();
  const showBnkBadge = (corner) => getShowBnkBadge()(corner);
  const hideBnkBadge = () => getHideBnkBadge()();
  const renderBnkBadge = (slot, opts) => getRenderBnkBadge()(slot, opts);

  const envTypeMap = { dca: 2, dcw: 1, pitch: 0 };

  const pushEnvelopesToEngine = (envs) => {
    const backend = window.getJuceBackend ? window.getJuceBackend() : ((window.__JUCE__ && window.__JUCE__.backend) || (window.Juce && window.Juce.backend));
    const audioEngine = getAudioEngine();
    ['dca', 'dcw', 'pitch'].forEach(typeKey => {
      ['line1', 'line2'].forEach(lineKey => {
        const env = envs[typeKey][lineKey];
        const envType = envTypeMap[typeKey];
        const line = lineKey === 'line1' ? 1 : 2;
        if (backend) {
          for (let i = 0; i < 8; i++) {
            if (typeof backend.setEnvelopeStage === 'function') backend.setEnvelopeStage(envType, line, i, env.rates[i], env.levels[i]);
            if (typeof backend.emitEvent === 'function') backend.emitEvent('setEnvelopeStage', { envType, line, stage: i, rate: env.rates[i], level: env.levels[i] });
          }
          if (typeof backend.setEnvelopeSustain === 'function') backend.setEnvelopeSustain(envType, line, env.sustainPoint);
          if (typeof backend.emitEvent === 'function') backend.emitEvent('setEnvelopeSustain', { envType, line, point: env.sustainPoint });
          if (typeof backend.setEnvelopeEnd === 'function') backend.setEnvelopeEnd(envType, line, env.endPoint);
          if (typeof backend.emitEvent === 'function') backend.emitEvent('setEnvelopeEnd', { envType, line, point: env.endPoint });
        }
        if (audioEngine) {
          for (let i = 0; i < 8; i++) {
            audioEngine.setEnvelopeStage(envType, line, i, env.rates[i], env.levels[i]);
          }
          audioEngine.setEnvelopeSustain(envType, line, env.sustainPoint);
          audioEngine.setEnvelopeEnd(envType, line, env.endPoint);
        }
      });
    });
  };

  const restoreEnvelopesToState = (envs) => {
    ['dca', 'dcw', 'pitch'].forEach(typeKey => {
      ['line1', 'line2'].forEach(lineKey => {
        const src = envs[typeKey][lineKey];
        const dest = envelopeState[typeKey][lineKey];
        dest.rates = [...src.rates];
        dest.levels = [...src.levels];
        dest.sustainPoint = src.sustainPoint;
        dest.endPoint = src.endPoint;
      });
    });
    ENV_TYPES.forEach(typeKey => {
      updateEnvelopeControls(typeKey, 1);
      updateEnvelopeControls(typeKey, 2);
    });
    redrawEnvelopes();
  };

  const compareManager = createCompareManager({
    readSnapshot: () => {
      const audioEngine = getAudioEngine();
      const isAudioStarted = getIsAudioStarted();
      if (!audioEngine || !isAudioStarted) return null;
      const params = {};
      PARAMETER_REGISTRY.parameters.forEach(p => {
        const inputEl = document.getElementById(p.id);
        if (!inputEl) return;
        const rawVal = inputEl.tagName === 'BUTTON'
          ? (inputEl.classList.contains('active') ? 1 : 0)
          : parseFloat(inputEl.value);
        params[p.id] = rawToNormalized(p.id, rawVal);
      });
      return {
        slot: parseInt(selectPresetSlot?.value || 0),
        params,
        envelopes: JSON.parse(JSON.stringify(envelopeState))
      };
    },
    loadSaved: (slot) => {
      // Hear the SAVED version from the bank (applies engine + UI via onPresetLoaded).
      const audioEngine = getAudioEngine();
      audioEngine.loadPreset(slot, PARAMETER_REGISTRY.parameters.map(p => p.id));
    },
    restore: (snap) => {
      // Restore the edited parameters to the engine and the UI controls.
      Object.entries(snap.params).forEach(([id, norm]) => {
        sendParameter(id, norm);
        applyParameterToUI(id, norm);
      });
      restoreEnvelopesToState(snap.envelopes);
      pushEnvelopesToEngine(snap.envelopes);
    }
  });

  const WRITE_SLOT_COUNT = 64;
  const LCD_NAME_MAX_LEN = LCD_NAME_MAX_LEN_CONST;
  const LCD_NAME_CHARS = LCD_NAME_CHARS_CONST;

  const writeManager = createWriteManager({
    slotCount: WRITE_SLOT_COUNT,
    nameMaxLen: LCD_NAME_MAX_LEN,
    getCurrentSlot: () => {
      const v = parseInt(selectPresetSlot?.value || 0);
      return Number.isNaN(v) ? 0 : v;
    },
    seedName: (slot) => {
      const current = (inputPresetName?.value || '').trim() || `Slot ${slot + 1}`;
      return seedLcdName(current);
    },
    // Same sanitize as the LCD seed path: uppercase, trimmed to the LCD width.
    sanitizeName: (raw) => seedLcdName(raw),
    cycleChar: (name, cursor, isUp) => cycleLcdChar(name, cursor, isUp),
    savePreset: (index, name) => {
      const audioEngine = getAudioEngine();
      if (!audioEngine) return false; // caller shows the AUDIO OFFLINE message
      // onPresetSaved callback shows "PRESET SAVED" and updates the slot dropdown.
      audioEngine.savePreset(index, name);
      return true;
    }
  });

  function enterCompare() {
    stopNormalLcdScroll();
    exitSetupMode(); // Compare takes over the LCD: unpress SET
    return compareManager.enter();
  }

  function exitCompare() {
    compareManager.exit();
  }

  function exitWriteMode() {
    writeManager.exit();
    hideBnkBadge();
  }

  function enterWriteMode() {
    stopNormalLcdScroll();
    exitSetupMode(); // Write flow takes over the LCD: unpress SET
    writeManager.enter();
    setLcdCompareMode(false);
    updateLCD();
    resetLCDInactivity();
  }

  // Advance from slot selection to name editing, seeded with the current name.
  function advanceWriteToName() {
    writeManager.advanceToName();
    updateLCD();
    resetLCDInactivity();
  }

  function confirmWriteSave() {
    const ok = writeManager.commit();
    // commit() resets the state machine internally, so hide the badge here too
    // (the updateLCD guard covers later paints, this covers the saved echo).
    hideBnkBadge();
    if (!ok) {
      lcdLine1.innerText = "AUDIO OFFLINE";
      lcdLine2.innerText = "INIT WASM FIRST";
      return true;
    }
    return false;
  }

  return {
    compareManager,
    writeManager,
    enterCompare,
    exitCompare,
    exitWriteMode,
    enterWriteMode,
    advanceWriteToName,
    confirmWriteSave,
    pushEnvelopesToEngine,
    restoreEnvelopesToState,
  };
}
