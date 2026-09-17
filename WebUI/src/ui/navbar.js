// ─── Navbar (native PluginEditor MenuBar: File / Edit / Mode / View / Help) ───
// The dropdowns are built from NAVBAR_MENUS (parity with getMenuBarNames /
// getMenuForIndex). Each action maps 1:1 to the function the native
// menuItemSelected() calls; where the web engine exposes the same capability
// (load/save bank, store patch, randomize, operation mode, themes, ...) it
// calls the SAME engine functions. Items that are technically impossible on
// the web (native file chooser on disk, Audio Settings host callback) are
// bridged to their closest web equivalent or hidden.
// Extracted from app.js so the menu bar, undo/redo, shortcuts and header
// buttons live in one focused module.

import { PARAMETER_REGISTRY, PARAM_MAP, rawToNormalized } from '../contracts/registry.gen.js';
import { applyTheme } from '../contracts/themes.js';
import { NAVBAR_MENUS, renderItemHtml } from '../contracts/navbarModel.js';
import { generateRandomPatch } from '../contracts/patchRandomizer.js';
import { matchShortcut, isEditableTarget } from '../contracts/shortcuts.js';

/**
 * createNavbar
 * @param {object} deps
 * @param {() => object} deps.getAudioEngine - live engine (null until started)
 * @param {() => string} deps.getCurrentThemeId
 * @param {(v: string) => void} deps.setCurrentThemeId
 * @param {object} deps.lcdLine1
 * @param {object} deps.lcdLine2
 * @param {object} deps.inputBankFile
 * @param {object} deps.inputSysex
 * @param {object} deps.inputPresetName
 * @param {object} deps.selectPresetSlot
 * @param {() => void} deps.updateLCD - LCD refresh (lcdPanel module)
 * @param {() => void} deps.syncLCDMode
 * @param {() => void} deps.toggleSetupMode
 * @param {object} deps.envelopeState
 * @param {string[]} deps.ENV_TYPES
 * @param {(typeKey: string, line: number) => void} deps.updateEnvelopeControls
 * @param {() => void} deps.redrawEnvelopes
 * @param {(envs: object) => void} deps.copyEnvelopesToState
 * @param {(opts: object) => void} deps.openNameEditor
 * @param {() => object} deps.makeDefaultBankJson
 * @param {() => void} deps.renderBankManager
 * @param {() => object} deps.getBankManagerModal
 * @param {(sectionId: string, title: string) => void} deps.openBlockDrawer
 * @param {(id: string, norm: number) => void} deps.sendParameter
 * @param {(id: string, norm: number) => void} deps.applyParameterToUI
 * @param {() => void} deps.triggerMidiActivity
 */
export function createNavbar(deps) {
  const {
    getAudioEngine,
    getCurrentThemeId,
    setCurrentThemeId,
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
    triggerMidiActivity
  } = deps;

  const navbarMenuBar = document.getElementById('menu-bar');
  const navbarDropdown = document.getElementById('menu-dropdown');

  // --- Mini undo/redo (native getUndoManager().undo/redo on the APVTS) ---
  // Snapshots the whole patch state (normalized params + envelopes) so Undo/
  // Redo restore exactly what the native UndoManager does for APVTS + envelopes.
  const UNDO_LIMIT = 32;
  let undoStack = [];
  let redoStack = [];
  let undoLocked = false;

  const capturePatchSnapshot = () => {
    const params = {};
    PARAMETER_REGISTRY.parameters.forEach(p => {
      const el = document.getElementById(p.id);
      if (!el) return;
      params[p.id] = el.tagName === 'BUTTON'
        ? (el.classList.contains('active') ? 1 : 0)
        : parseFloat(el.value);
    });
    return { params, envelopes: JSON.parse(JSON.stringify(envelopeState)) };
  };

  const pushUndo = () => {
    if (undoLocked) return;
    undoStack.push(capturePatchSnapshot());
    if (undoStack.length > UNDO_LIMIT) undoStack.shift();
    redoStack = [];
  };

  const restorePatchSnapshot = (snap) => {
    undoLocked = true;
    try {
      Object.entries(snap.params).forEach(([id, rawVal]) => {
        const spec = PARAM_MAP.get(id);
        if (!spec) return;
        const norm = rawToNormalized(id, rawVal);
        sendParameter(id, norm);
        applyParameterToUI(id, norm);
      });
      const envTypeMap = { dca: 2, dcw: 1, pitch: 0 };
      ['dca', 'dcw', 'pitch'].forEach(typeKey => {
        ['line1', 'line2'].forEach(lineKey => {
          const src = snap.envelopes[typeKey][lineKey];
          const dest = envelopeState[typeKey][lineKey];
          dest.rates = [...src.rates];
          dest.levels = [...src.levels];
          dest.sustainPoint = src.sustainPoint;
          dest.endPoint = src.endPoint;
          const backend = window.getJuceBackend ? window.getJuceBackend() : ((window.__JUCE__ && window.__JUCE__.backend) || (window.Juce && window.Juce.backend));
          const envType = envTypeMap[typeKey];
          const line = lineKey === 'line1' ? 1 : 2;
          if (backend) {
            for (let i = 0; i < 8; i++) {
              if (typeof backend.setEnvelopeStage === 'function') backend.setEnvelopeStage(envType, line, i, src.rates[i], src.levels[i]);
              if (typeof backend.emitEvent === 'function') backend.emitEvent('setEnvelopeStage', { envType, line, stage: i, rate: src.rates[i], level: src.levels[i] });
            }
            if (typeof backend.setEnvelopeSustain === 'function') backend.setEnvelopeSustain(envType, line, src.sustainPoint);
            if (typeof backend.emitEvent === 'function') backend.emitEvent('setEnvelopeSustain', { envType, line, point: src.sustainPoint });
            if (typeof backend.setEnvelopeEnd === 'function') backend.setEnvelopeEnd(envType, line, src.endPoint);
            if (typeof backend.emitEvent === 'function') backend.emitEvent('setEnvelopeEnd', { envType, line, point: src.endPoint });
          }
          const engine = getAudioEngine();
          if (engine) {
            for (let i = 0; i < 8; i++) {
              engine.setEnvelopeStage(envType, line, i, src.rates[i], src.levels[i]);
            }
            engine.setEnvelopeSustain(envType, line, src.sustainPoint);
            engine.setEnvelopeEnd(envType, line, src.endPoint);
          }
        });
      });
      ENV_TYPES.forEach(typeKey => {
        updateEnvelopeControls(typeKey, 1);
        updateEnvelopeControls(typeKey, 2);
      });
      redrawEnvelopes();
    } finally {
      undoLocked = false;
    }
  };

  // Snapshot BEFORE a manual parameter edit so Undo captures the previous state:
  // the interaction-start events fire while the control still holds the old value
  // (input/change fire after the value has already moved). Re-armed on the first
  // input so one drag/session = one undo step.
  document.addEventListener('mousedown', (e) => {
    if (e.target && e.target.id && PARAM_MAP.has(e.target.id) && !e.target.dataset.undoArmed) {
      if (e.target === document.getElementById('OPERATION_MODE')) return; // mode change is not undoable
      e.target.dataset.undoArmed = '1';
      pushUndo();
    }
  }, true);

  document.addEventListener('touchstart', (e) => {
    if (e.target && e.target.id && PARAM_MAP.has(e.target.id) && !e.target.dataset.undoArmed) {
      if (e.target === document.getElementById('OPERATION_MODE')) return;
      e.target.dataset.undoArmed = '1';
      pushUndo();
    }
  }, true);

  document.addEventListener('focus', (e) => {
    if (e.target && e.target.id && PARAM_MAP.has(e.target.id) && !e.target.dataset.undoArmed) {
      if (e.target === document.getElementById('OPERATION_MODE')) return;
      e.target.dataset.undoArmed = '1';
      pushUndo();
    }
  }, true);

  document.addEventListener('input', (e) => {
    if (e.target && e.target.id && PARAM_MAP.has(e.target.id)) {
      e.target.dataset.undoArmed = ''; // re-arm after the edit completes
    }
  }, true);

  const doUndo = () => {
    if (undoStack.length === 0) { flashLcd('UNDO', 'NOTHING TO UNDO'); return; }
    redoStack.push(capturePatchSnapshot());
    restorePatchSnapshot(undoStack.pop());
    flashLcd('UNDO', 'PATCH RESTORED');
  };

  const doRedo = () => {
    if (redoStack.length === 0) { flashLcd('REDO', 'NOTHING TO REDO'); return; }
    undoStack.push(capturePatchSnapshot());
    restorePatchSnapshot(redoStack.pop());
    flashLcd('REDO', 'PATCH RESTORED');
  };

  function flashLcd(line1, line2) {
    if (lcdLine1) lcdLine1.innerText = line1;
    if (lcdLine2) lcdLine2.innerText = line2;
    setTimeout(() => updateLCD(), 1200);
  }

  // --- Actions (native menuItemSelected mapping) ---
  const navbarActions = {
    loadBank: () => { inputBankFile.click(); },
    saveBank: () => { const engine = getAudioEngine(); if (engine) engine.saveBank(); },
    loadPatch: () => { inputSysex.click(); },
    savePatchAs: () => { const engine = getAudioEngine(); if (engine) { pushUndo(); engine.saveSysEx((inputPresetName.value || 'Active Patch').trim()); } },
    exportSysEx: () => { const engine = getAudioEngine(); if (engine) engine.saveSysEx((inputPresetName.value || 'Active Patch').trim()); },
    storePatch: () => { const engine = getAudioEngine(); if (engine) { pushUndo(); engine.savePreset(parseInt(selectPresetSlot.value || 0), (inputPresetName.value || '').trim() || `Slot ${parseInt(selectPresetSlot.value || 0) + 1}`); } },
    storeNewSlot: () => {
      // Native File -> Store to New Slot: nameOverlay.startRename(currentName,
      // cb -> addPreset with the new name).
      const engine = getAudioEngine();
      if (!engine) return;
      openNameEditor({
        title: 'STORE NEW SLOT',
        value: (inputPresetName.value || '').trim(),
        onSave: (name) => {
          pushUndo();
          const idx = Array.from(selectPresetSlot.options).findIndex(o => (o.text || '').includes('(Empty)'));
          const index = idx === -1 ? 63 : idx;
          engine.savePreset(index, name.trim() || `Slot ${index + 1}`);
        }
      });
    },
    renamePatch: () => {
      // Native File -> Rename Current Patch: nameOverlay.startRename(currentName,
      // cb -> renamePreset(currentPresetIndex, name)).
      openNameEditor({
        title: 'RENAME PRESET',
        value: (inputPresetName.value || '').trim(),
        onSave: (name) => {
          const trimmed = name.trim();
          if (!trimmed) return;
          const engine = getAudioEngine();
          if (engine) engine.renamePreset(parseInt(selectPresetSlot.value || 0), trimmed);
          inputPresetName.value = trimmed;
        }
      });
    },
    initBank: () => {
      // Native PresetBrowser::initBank parity: AlertWindow "Are you sure? This
      // will replace all presets with factory defaults." (Reset / Cancel). The
      // destructive action must be confirmed before wiping the engine bank.
      const engine = getAudioEngine();
      if (!engine) return;
      if (!window.confirm('Are you sure? This will replace all presets with factory defaults.')) return;
      pushUndo();
      engine.loadBank(makeDefaultBankJson(), PARAMETER_REGISTRY.parameters.map(p => p.id));
    },
    resetPatch: () => {
      pushUndo();
      PARAMETER_REGISTRY.parameters.forEach(p => {
        const inputEl = document.getElementById(p.id);
        if (!inputEl) return;
        const rawVal = p.default;
        const norm = rawToNormalized(p.id, rawVal);
        sendParameter(p.id, norm);
        applyParameterToUI(p.id, norm);
      });
      flashLcd('RESET', 'DEFAULTS APPLIED');
    },
    undo: doUndo,
    redo: doRedo,
    bankManager: () => {
      // Enhanced Bank Manager: library of model-tagged banks + slot grid with
      // right-click context menu, search, CRUD and import/export.
      const modal = getBankManagerModal();
      if (!modal) return;
      renderBankManager();
      modal.hidden = false;
    },
    randomize: () => {
      const backend = window.getJuceBackend ? window.getJuceBackend() : ((window.__JUCE__ && window.__JUCE__.backend) || (window.Juce && window.Juce.backend));
      const engine = getAudioEngine();
      if (!engine && !backend) { flashLcd('RNDM', 'INIT AUDIO FIRST'); return; }
      pushUndo();
      const patch = generateRandomPatch();
      Object.entries(patch.params).forEach(([id, rawVal]) => {
        const spec = PARAM_MAP.get(id);
        if (!spec) return;
        const norm = rawToNormalized(id, rawVal);
        sendParameter(id, norm);
        applyParameterToUI(id, norm);
      });
      copyEnvelopesToState(patch.envelopes);
      // Push envelopes to the engine (copyEnvelopesToState only updates the editor).
      const envTypeMap = { dca: 2, dcw: 1, pitch: 0 };
      ['dca', 'dcw', 'pitch'].forEach(typeKey => {
        ['line1', 'line2'].forEach(lineKey => {
          const env = patch.envelopes[typeKey][lineKey];
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
          if (engine) {
            for (let i = 0; i < 8; i++) engine.setEnvelopeStage(envType, line, i, env.rates[i], env.levels[i]);
            engine.setEnvelopeSustain(envType, line, env.sustainPoint);
            engine.setEnvelopeEnd(envType, line, env.endPoint);
          }
        });
      });
      flashLcd('RANDOMIZED', 'NEW PATCH GENERATED');
    },
    settingsSystem: () => {
      // Native (PluginEditor.cpp case 205): toggles LCDStateManager between
      // SYSTEM and EDIT and shows a checkmark on the menu item. Same toggle as
      // the SET button: while active the button stays pressed; pressing again
      // (or Ctrl+, again) exits back to Preset Program Mode.
      toggleSetupMode();
    },
    audioSettings: () => {
      // Native (PluginEditor.cpp case 206) calls the host Audio Settings dialog;
      // on the web the equivalent is the SETTINGS drawer with the OUTPUT /
      // AUDIO OUTPUT device selector (model-modes section).
      openBlockDrawer('model-modes', 'SETTINGS');
    },
    midiChannel: (v) => {
      const sel = document.getElementById('MIDI_CH');
      if (sel) { sel.value = String(v); sel.dispatchEvent(new Event('change')); }
    },
    operationMode: (v) => {
      const sel = document.getElementById('OPERATION_MODE');
      if (sel) { sel.value = String(v); sel.dispatchEvent(new Event('input')); sel.dispatchEvent(new Event('change')); }
    },
    oversampling: (v) => {
      const sel = document.getElementById('OVERSAMPLING_QUALITY');
      if (sel) { sel.value = String(v); sel.dispatchEvent(new Event('input')); sel.dispatchEvent(new Event('change')); }
    },
    zoom: (v) => {
      const container = document.querySelector('.synth-container');
      if (container) {
        container.style.transform = `scale(${v})`;
        container.style.transformOrigin = 'top center';
        document.body.style.minHeight = '100vh';
      }
    },
    theme: (v) => {
      applyTheme(document.documentElement, v);
      try { localStorage.setItem('cz101.theme', v); } catch (err) { /* ignore */ }
      setCurrentThemeId(v);
      syncLCDMode();
    },
    manual: () => {
      // Native menu item 900 (Manual / Wiki) is a no-op stub in the C++ editor;
      // the web version points at the real project repository (this monorepo
      // houses the CZ-101 emulator; DOCS/ holds the full spec documentation).
      window.open('https://github.com/ajabadia/ABDOmegaUnified', '_blank');
    },
    about: () => {
      const el = document.getElementById('about-modal');
      if (el) { el.hidden = false; }
    }
  };

  // --- Dropdown rendering ---
  function renderNavbarDropdown(menuName) {
    const menu = NAVBAR_MENUS.find(m => m.name === menuName);
    if (!menu || !navbarDropdown) return;

    const getCurrent = (action) => {
      // Theme lives only in the View menu (native parity); the active id is
      // tracked in currentThemeId since there is no theme dropdown anymore.
      if (action === 'theme') return getCurrentThemeId();
      const el = document.getElementById(
        action === 'operationMode' ? 'OPERATION_MODE'
          : action === 'oversampling' ? 'OVERSAMPLING_QUALITY'
            : action === 'midiChannel' ? 'MIDI_CH'
              : null);
      if (!el) return null;
      if (el.tagName === 'SELECT') return parseInt(el.value);
      return null;
    };

    navbarDropdown.innerHTML = menu.items.map(item => renderItemHtml(item, getCurrent)).join('');
    navbarDropdown.dataset.menu = menuName;
    navbarDropdown.hidden = false;
  }

  function hideNavbarDropdown() {
    if (navbarDropdown) navbarDropdown.hidden = true;
  }

  if (navbarMenuBar) {
    navbarMenuBar.querySelectorAll('.menu-trigger').forEach(trigger => {
      trigger.addEventListener('click', (e) => {
        e.stopPropagation();
        const menuName = trigger.dataset.menu;
        if (navbarDropdown && navbarDropdown.dataset.menu === menuName && !navbarDropdown.hidden) {
          hideNavbarDropdown();
        } else {
          renderNavbarDropdown(menuName);
        }
      });
    });

    navbarDropdown.addEventListener('click', (e) => {
      const item = e.target.closest('button.nav-item');
      if (!item) return;
      const action = item.dataset.action;
      // Values are JSON: themes are strings ('retroterminal'), numeric actions
      // (operationMode, zoom, ...) are numbers. Parse accordingly.
      const raw = item.dataset.value;
      let value = undefined;
      if (raw !== '' && raw !== undefined) {
        value = /^-?\d+(\.\d+)?$/.test(raw) ? Number(raw) : raw;
      }
      if (action && navbarActions[action]) {
        navbarActions[action](value);
      }
      hideNavbarDropdown();
    });

    document.addEventListener('click', (e) => {
      if (!navbarMenuBar.contains(e.target)) hideNavbarDropdown();
    });

    document.addEventListener('keydown', (e) => {
      if (e.key === 'Escape') hideNavbarDropdown();
    });
  }

  // Header buttons: RNDM / COMPR / ALL OFF (native randomButton / compareButton / panicButton)
  const btnRandom = document.getElementById('btn-random');
  if (btnRandom) btnRandom.addEventListener('click', () => navbarActions.randomize());

  const btnPanic = document.getElementById('btn-panic');
  if (btnPanic) {
    btnPanic.addEventListener('click', () => {
      // CC 123 = All Notes Off (the native MIDIProcessor handles it -> allNotesOff)
      const engine = getAudioEngine();
      if (engine) engine.sendMidiMessage(new Uint8Array([0xB0, 0x7B, 0x00]));
      triggerMidiActivity();
      flashLcd('ALL OFF', 'NOTES KILLED');
    });
  }

  // About dialog wiring (native AboutDialog)
  const aboutModal = document.getElementById('about-modal');
  if (aboutModal) {
    const buildEl = document.getElementById('about-build');
    if (buildEl) buildEl.textContent = 'Build: CZ-101 WASM core (shared with native) — web interface.';
    const closeAbout = () => { aboutModal.hidden = true; };
    document.getElementById('btn-about-close')?.addEventListener('click', closeAbout);
    aboutModal.addEventListener('click', (e) => { if (e.target === aboutModal) closeAbout(); });
    document.addEventListener('keydown', (e) => { if (e.key === 'Escape') aboutModal.hidden = true; });
  }

  // --- Keyboard shortcuts (native keyPressed parity + web/DAW standards) ---
  // Native C++ only maps Cmd/Ctrl+Z -> undo and Cmd/Ctrl+Y -> redo; the rest
  // follow common web/DAW conventions for the same navbarActions.
  document.addEventListener('keydown', (e) => {
    // Let the browser keep native text-editing shortcuts inside inputs.
    if (isEditableTarget(e.target)) return;
    const action = matchShortcut(e);
    if (!action || typeof navbarActions[action] !== 'function') return;
    e.preventDefault();
    navbarActions[action]();
  });

  // --- Physical arrow keys -> LCD keypad cursors ---
  // The native JUCE TextButtons respond to arrow keys when focused; the WebUI
  // keypad is click-only, so map the physical arrows to the same buttons the
  // LCD menu uses (native LCDStateManager cursorUp/Down/Left/Right parity).
  // Guarded by isEditableTarget so typing in inputs/selects is never hijacked.
  document.addEventListener('keydown', (e) => {
    if (isEditableTarget(e.target)) return;
    if (e.altKey || e.ctrlKey || e.metaKey) return; // let navbar shortcuts win
    const arrowToKeypad = {
      ArrowUp: 'btn-lcd-val-up',
      ArrowDown: 'btn-lcd-val-down',
      ArrowLeft: 'btn-lcd-cur-left',
      ArrowRight: 'btn-lcd-cur-right'
    };
    const id = arrowToKeypad[e.key];
    if (!id) return;
    const btn = document.getElementById(id);
    if (!btn) return;
    e.preventDefault();
    btn.click();
  });

  return {
    pushUndo,
    flashLcd,
    navbarActions,
    doUndo,
    doRedo,
    renderNavbarDropdown,
    hideNavbarDropdown
  };
}
