// LCD panel — keypad state machine + LCD readout (native LCDStateManager parity).
// The keypad modes were split into focused modules under ./ that share state
// through the `ctx` object below:
//   lcdScroller.js   — generic char-by-char scroller + scroll settings
//   lcdBnkMode.js    — BNK (Bank Browse) mode + position badge
//   lcdMdlMode.js    — MDL (Model Browse) mode
//   lcdWriteMode.js  — Write + Compare modes (patchFlow managers)
//   lcdSetupMode.js  — SET (System Mode)
// This file owns the shared LCD state (menu index, mode flags, scrollers, the
// LCD readout itself), the keypad listeners and the per-model UI sync.
//
// Factory pattern (same as envelopeEditor / keyboard / modMatrix): the module
// owns the LCD state and receives the live app references (engine getter, undo,
// bank library, DOM refs) through deps so app.js wires the real objects.
// Mutable state that app.js must read/write (setLCDParam, onPresetLoaded,
// modMatrix wiring) is exposed through get/set accessors.
import { PARAMETER_REGISTRY, PARAM_MAP, rawToNormalized } from '../contracts/registry.gen.js';
import { buildLcdMenu } from '../contracts/lcdMenu.js';
import { formatLcdName } from '../contracts/lcdName.js';
import { attachHoldRepeat } from '../contracts/holdRepeat.js';
import { createLcdScroller, measureLcdChars, getScrollSpeed, getScrollPause, setScrollSetting } from './lcdScroller.js';
import { createBnkMode } from './lcdBnkMode.js';
import { createMdlMode } from './lcdMdlMode.js';
import { createWriteMode } from './lcdWriteMode.js';
import { createSetupMode } from './lcdSetupMode.js';

export function createLcdPanel(deps) {
  const lcdLine1 = deps.lcdLine1;
  const lcdLine2 = deps.lcdLine2;
  const selectPresetSlot = deps.selectPresetSlot;
  const inputPresetName = deps.inputPresetName;
  const getAudioEngine = deps.getAudioEngine;
  const getIsAudioStarted = deps.getIsAudioStarted;
  const getPushUndo = deps.getPushUndo;
  const getBankLib = deps.getBankLib;
  const sendParameter = deps.sendParameter;
  const applyParameterToUI = deps.applyParameterToUI;
  const openNameEditor = deps.openNameEditor;
  const activeBank = deps.activeBank;
  const activateBank = deps.activateBank;
  const loadSavedPresetSlot = deps.loadSavedPresetSlot;
  const loadPresetSlot = deps.loadPresetSlot;
  const getAuthKeyTrackLcdValue = deps.getAuthKeyTrackLcdValue;
  const applyModernMatrixSeeds = deps.applyModernMatrixSeeds;
  const envelopeState = deps.envelopeState;
  const ENV_TYPES = deps.ENV_TYPES;
  const updateEnvelopeControls = deps.updateEnvelopeControls;
  const redrawEnvelopes = deps.redrawEnvelopes;
  const pushUndo = (...args) => getPushUndo()(...args);

// --- LCD Menu Keypad State & Manager ---
// Menu metadata lives in ./contracts/lcdMenu.js (built from the parameter
// registry) so it can be shared with the test suite. The list is rebuilt on
// operation-mode changes, mirroring LCDStateManager::buildParameterList.
const currentOperationMode = () => {
  const opModeEl = document.getElementById('OPERATION_MODE');
  return opModeEl ? parseInt(opModeEl.value || 0) : 0;
};

let MENU_PARAMETERS = buildLcdMenu(currentOperationMode());

// Raw values for LCD-editable parameters that have no drawer control (the
// Key Follow modes): the LCD is their control surface, so the value store is
// seeded from the registry defaults and kept in sync by sendParameter paths.
const lcdParamRawValues = {};
const getLcdRawValue = (paramId, fallback = 0) => {
  if (lcdParamRawValues[paramId] !== undefined) return lcdParamRawValues[paramId];
  const spec = PARAM_MAP.get(paramId);
  return (spec && spec.default !== undefined) ? spec.default : fallback;
};

let lcdMenuIndex = -1; // -1 = Preset Program Mode, >=0 = Parameter Edit Mode
let lcdCompareMode = false;
let lcdSetupMode = false; // SET pressed: LCD locked to the SYSTEM parameter page
let lcdInactivityTimeout = null;

// The menu list the keypad actually navigates. While SET (System Mode) is
// active only the SYSTEM parameters are reachable, mirroring the native
// LCDStateManager::Mode::SYSTEM which rebuilds its parameter list with just
// the system page (MIDI CHANNEL, BEND RANGE, TRANSPOSE, MASTER TUNE, PROTECT,
// AUTH MODE).
function activeMenuParameters() {
  return lcdSetupMode
    ? MENU_PARAMETERS.filter(p => p.system)
    : MENU_PARAMETERS;
}

// ── Scrollers (one per LCD readout owner) ──
// Each owner keeps its own scroller; only one mode is active at a time and
// switching modes stops the previous owner's scroller.
const bnkLine1 = createLcdScroller(lcdLine1);
const bnkLine2 = createLcdScroller(lcdLine2);
// Normal program-mode readout also scrolls line 2 when the preset name is too
// long (same LCD char-by-char ping-pong as the browse modes).
const normalLine2 = createLcdScroller(lcdLine2);
// LCD menu readout: long parameter names / values (e.g. choice labels) scroll.
const menuLine1 = createLcdScroller(lcdLine1);
const menuLine2 = createLcdScroller(lcdLine2);
const mdlLine1 = createLcdScroller(lcdLine1);

// Cancel the normal-mode line-2 scroll (used when another LCD mode takes over).
const stopNormalLcdScroll = () => {
  normalLine2.stop();
};

function syncLCDBlink() {
  const display = document.getElementById('lcd-display');
  if (!display) return;
  display.classList.toggle('lcd-blink', lcdCompareMode);
}

// ── Shared ctx handed to the extracted mode modules ──
// Mode factories are created below in an order-independent way: every cross-
// mode call (BNK -> MDL/SET/WRITE, etc.) goes through a lazy getter on `ctx`
// that resolves to the sibling API at call time, so creation order does not
// matter. `modeApis` is filled right after the four factories are created.
const modeApis = {};

const ctx = {
  lcdLine1,
  lcdLine2,
  selectPresetSlot,
  inputPresetName,
  getAudioEngine,
  getIsAudioStarted,
  getBankLib,
  sendParameter,
  applyParameterToUI,
  openNameEditor,
  activeBank,
  activateBank,
  loadSavedPresetSlot,
  loadPresetSlot,
  envelopeState,
  ENV_TYPES,
  updateEnvelopeControls,
  redrawEnvelopes,
  bnkLine1,
  bnkLine2,
  mdlLine1,
  // Shared state (get/set so the modules never hold stale copies).
  getLcdMenuIndex: () => lcdMenuIndex,
  setLcdMenuIndex: (v) => { lcdMenuIndex = v; },
  getLcdCompareMode: () => lcdCompareMode,
  setLcdCompareMode: (v) => { lcdCompareMode = v; },
  getLcdSetupMode: () => lcdSetupMode,
  setLcdSetupMode: (v) => { lcdSetupMode = v; },
  getLcdBnkMode: () => !!(modeApis.bnk && modeApis.bnk.getLcdBnkMode()),
  getLcdMdlMode: () => !!(modeApis.mdl && modeApis.mdl.getLcdMdlMode()),
  // Core functions (lazy: resolved at call time).
  getUpdateLCD: () => updateLCD,
  getResetInactivity: () => resetLCDInactivity,
  getStopNormalLcdScroll: () => stopNormalLcdScroll,
  getActiveMenuParameters: () => activeMenuParameters,
  // Cross-mode APIs (lazy: filled once modeApis is complete).
  getExitBnkMode: () => modeApis.bnk && modeApis.bnk.exitBnkMode,
  getExitMdlMode: () => modeApis.mdl && modeApis.mdl.exitMdlMode,
  getExitSetupMode: () => modeApis.setup && modeApis.setup.exitSetupMode,
  getExitWriteMode: () => modeApis.write && modeApis.write.exitWriteMode,
  getShowBnkBadge: () => modeApis.bnk && modeApis.bnk.showBnkBadge,
  getHideBnkBadge: () => modeApis.bnk && modeApis.bnk.hideBnkBadge,
  getRenderBnkBadge: () => modeApis.bnk && modeApis.bnk.renderBnkBadge,
  getWriteManager: () => modeApis.write && modeApis.write.writeManager,
  getCompareManager: () => modeApis.write && modeApis.write.compareManager,
};

modeApis.bnk = createBnkMode(ctx);
modeApis.mdl = createMdlMode(ctx);
modeApis.write = createWriteMode(ctx);
modeApis.setup = createSetupMode(ctx);

const writeManager = modeApis.write.writeManager;
const compareManager = modeApis.write.compareManager;

// Mode-active checks used by the keypad handlers (resolved at call time).
const lcdBnkModeActive = () => modeApis.bnk.getLcdBnkMode();
const lcdMdlModeActive = () => modeApis.mdl.getLcdMdlMode();
const { renderBnkLcd, hideBnkBadge, showBnkBadge, renderBnkBadge } = modeApis.bnk;
const { renderMdlLcd } = modeApis.mdl;
const { toggleSetupMode, exitSetupMode } = modeApis.setup;
const { exitBnkMode, toggleBnkMode, cycleBank, cyclePatch } = modeApis.bnk;
const { exitMdlMode, toggleMdlMode, cycleModel } = modeApis.mdl;
const { enterWriteMode, exitWriteMode, advanceWriteToName, confirmWriteSave, enterCompare, exitCompare } = modeApis.write;

function updateLCD() {
  if (!lcdLine1 || !lcdLine2) return;
  syncLCDBlink();

  // Mode flags snapshot for this paint (BNK/MDL read live state via the mode
  // factories; the paint uses one consistent view).
  const lcdBnkMode = modeApis.bnk.getLcdBnkMode();
  const lcdMdlMode = modeApis.mdl.getLcdMdlMode();

  // The normal program-mode readout (PRG + preset name) is the only place line 2
  // scrolls outside Bank Browse; cancel that scroll whenever another mode takes
  // the LCD so its timer never overwrites the active readout.
  const renderingNormalReadout = lcdMenuIndex === -1 && !lcdCompareMode && !lcdBnkMode && !lcdMdlMode && !writeManager.isActive;
  if (!renderingNormalReadout) stopNormalLcdScroll();
  // The menu readout (param name + value) scrolls long text too; stop its
  // timers whenever another LCD mode takes over.
  const renderingMenuReadout = lcdMenuIndex !== -1 && !lcdCompareMode && !lcdBnkMode && !lcdMdlMode && !writeManager.isActive;
  if (!renderingMenuReadout) {
    menuLine1.stop();
    menuLine2.stop();
  }

  // The position badge belongs to Bank Browse, Write and Compare mode only
  // (coherent readout); any other LCD mode hides it and releases the corners.
  if (!lcdBnkMode && !writeManager.isActive && !lcdCompareMode) hideBnkBadge();

  if (lcdCompareMode) {
    lcdLine1.innerText = "   COMPARING    ";
    lcdLine2.innerText = "   ..SOUND..    ";
    // Badge shows the compared slot (same bank N/total · Pxx/occupied format).
    const cmpSlot = compareManager.slot;
    if (cmpSlot != null) {
      showBnkBadge(); // top-right, like BNK
      renderBnkBadge(cmpSlot + 1);
    }
    return;
  }
  if (modeApis.bnk.getLcdBnkMode()) {
    // Bank Browse: bank name on line 1, patch name on line 2 (scrolling).
    renderBnkLcd();
    return;
  }
  if (modeApis.mdl.getLcdMdlMode()) {
    // Model Browse: model name on line 1 (scrolling), M<mode>/<total> on line 2.
    renderMdlLcd();
    return;
  }
  if (writeManager.isActive) {
    const ws = writeManager.state;
    // Same position badge as Bank Browse, tracking the selected destination
    // slot — bottom-left corner so it never collides with "WRITE: SELECT SLOT"
    // on line 1, and the slot turns amber when it already holds a patch (you
    // are about to overwrite it).
    showBnkBadge('bottom-left');
    if (ws.phase === 'name') {
      // Hybrid naming hint: SET opens the fast text editor for the name. The
      // hint is adaptive — it only shows when it fits next to the badge
      // (measured: ~16 chars available in the name phase, so it stays short).
      const hint = "WRITE: NAME SET";
      lcdLine1.innerText = measureLcdChars(lcdLine1) >= hint.length ? hint : "WRITE: NAME";
      lcdLine2.innerText = formatLcdName(ws.name, ws.cursor);
    } else {
      const slotText = (selectPresetSlot && selectPresetSlot.options[ws.targetSlot])
        ? (selectPresetSlot.options[ws.targetSlot].text.split(': ')[1] || "Empty")
        : "Empty";
      lcdLine1.innerText = "WRITE: SELECT SLOT";
      lcdLine2.innerText = `SLOT ${String(ws.targetSlot + 1).padStart(2, '0')} ${slotText.toUpperCase()}`;
    }
    renderBnkBadge(ws.targetSlot + 1, { warnOccupiedSlot: true });
    return;
  }
  if (lcdMenuIndex === -1) {
    const activeSlot = parseInt(document.getElementById('select-preset-slot')?.value || 0) + 1;
    const activeName = document.getElementById('input-preset-name')?.value || "Init User";
    lcdLine1.innerText = `PRG: ${activeSlot}`;
    // Long preset names scroll on line 2 (LCD char-by-char, same as the browse
    // modes — the generic scroller keeps the running scroll on unchanged text).
    normalLine2.set(activeName.toUpperCase());
    return;
  }

  const menuList = activeMenuParameters();
  // Defensive: never let the index drift past the navigable list (any stale
  // full-list index after a mode/setup switch would crash on param.computed).
  if (lcdMenuIndex < 0 || lcdMenuIndex >= menuList.length) {
    lcdMenuIndex = menuList.length > 0 ? 0 : -1;
  }
  const param = menuList[lcdMenuIndex];
  if (!param) return;
  if (param.computed) {
    // Computed read-only entry (AUTH K-TRACK): value derived from the free
    // matrix at display time, like the native "K-TRACK DCW/PIT" readouts.
    menuLine1.set(param.name);
    menuLine2.set(getAuthKeyTrackLcdValue());
    return;
  }
  if (param.uiSetting) {
    // WebUI-only UI setting (scroll timing): no engine param — edited directly
    // and persisted in localStorage, like the native "Scroll" UI preference.
    menuLine1.set(param.name);
    const val = param.id === 'LCD_SCROLL_SPEED' ? getScrollSpeed() : getScrollPause();
    menuLine2.set(`${val} ${param.unit || ''}`.trim().toUpperCase());
    return;
  }
  const inputEl = document.getElementById(param.id);
  if (!inputEl) {
    // LCD-only parameter (no drawer control, e.g. Key Follow modes): show the
    // stored raw value instead of "NOT BOUND" — the LCD edits it directly.
    menuLine1.set(param.name);
    const raw = getLcdRawValue(param.id);
    if (param.type === 'choice' && param.choices && param.choices[raw] != null) {
      menuLine2.set(String(param.choices[raw]).toUpperCase());
    } else {
      menuLine2.set(`${raw} ${param.unit || ''}`.trim().toUpperCase());
    }
    return;
  }

  menuLine1.set(param.name);

  if (param.type === 'choice') {
    const val = parseInt(inputEl.value || 0);
    if (inputEl.tagName === 'BUTTON') {
      const isActive = inputEl.classList.contains('active');
      menuLine2.set(isActive ? "ON" : "OFF");
    } else {
      menuLine2.set(((param.choices && param.choices[val]) || val).toString().toUpperCase());
    }
  } else if (param.type === 'int') {
    const val = parseInt(inputEl.value || 0);
    menuLine2.set(`${val} ${param.unit || ''}`.trim().toUpperCase());
  } else {
    const val = parseFloat(inputEl.value || 0);
    menuLine2.set(`${val.toFixed(2)} ${param.unit || ''}`.trim().toUpperCase());
  }
}

function resetLCDInactivity() {
  if (lcdInactivityTimeout) clearTimeout(lcdInactivityTimeout);
  lcdInactivityTimeout = setTimeout(() => {
    if (lcdCompareMode) exitCompare(); // restore edits before returning to normal
    lcdMenuIndex = -1;
    lcdCompareMode = false;
    exitSetupMode(); // unpress SET and leave System Mode
    exitWriteMode();
    exitBnkMode();  // unpress BNK and leave Bank Browse mode
    exitMdlMode();  // unpress MDL and leave Model Browse mode
    updateLCD();
  }, 10000); // Revert to program select mode after 10s of inactivity
}

function setupLCDKeypadListeners() {
  const btnCompare = document.getElementById('btn-lcd-compare');
  const btnWrite = document.getElementById('btn-lcd-write');
  const btnCurLeft = document.getElementById('btn-lcd-cur-left');
  const btnCurRight = document.getElementById('btn-lcd-cur-right');
  const btnValUp = document.getElementById('btn-lcd-val-up');
  const btnValDown = document.getElementById('btn-lcd-val-down');
  const btnSet = document.getElementById('btn-lcd-set');
  const btnBnk = document.getElementById('btn-lcd-bnk');
  const btnMdl = document.getElementById('btn-lcd-mdl');

  if (btnBnk) {
    btnBnk.addEventListener('click', () => {
      toggleBnkMode();
      resetLCDInactivity();
    });
  }

  if (btnMdl) {
    btnMdl.addEventListener('click', () => {
      toggleMdlMode();
      resetLCDInactivity();
    });
  }

  if (btnCompare) {
    btnCompare.addEventListener('click', () => {
      if (lcdBnkModeActive()) exitBnkMode(); // Compare takes over the LCD
      if (lcdMdlModeActive()) exitMdlMode();
      if (writeManager.isActive) writeManager.exit();
      if (!lcdCompareMode) {
        enterCompare(); // A/B swap (LCD-only indication without the engine)
        lcdCompareMode = true;
      } else {
        exitCompare();
        lcdCompareMode = false;
      }
      updateLCD();
      resetLCDInactivity();
    });
  }

  if (btnWrite) {
    btnWrite.addEventListener('click', () => {
      if (lcdBnkModeActive()) exitBnkMode(); // Write takes over the LCD
      if (lcdMdlModeActive()) exitMdlMode();
      lcdCompareMode = false;
      if (writeManager.isActive) {
        // WRT -> slot -> name -> save
        if (writeManager.state.phase === 'slot') {
          advanceWriteToName();
        } else if (writeManager.state.phase === 'name') {
          if (!confirmWriteSave()) updateLCD(); // saved: let onPresetSaved confirm
        }
      } else {
        enterWriteMode();
      }
      resetLCDInactivity();
    });
  }

  if (btnCurLeft) {
    attachHoldRepeat(btnCurLeft, () => {
      if (lcdMdlModeActive()) {
        cycleModel(-1); // previous model
        resetLCDInactivity();
        return;
      }
      if (lcdBnkModeActive()) {
        cyclePatch(-1); // previous patch in the active bank
        resetLCDInactivity();
        return;
      }
      if (writeManager.isActive) {
        if (writeManager.state.phase === 'name') { // move cursor left through the name
          writeManager.moveCursor(-1);
        } else { // slot phase: cursor cancels Write mode without saving
          exitWriteMode();
        }
        updateLCD();
        resetLCDInactivity();
        return;
      }
      lcdCompareMode = false;
      const menuList = activeMenuParameters();
      if (lcdMenuIndex === -1) {
        lcdMenuIndex = menuList.length - 1;
      } else {
        lcdMenuIndex = (lcdMenuIndex - 1 + menuList.length) % menuList.length;
      }
      updateLCD();
      resetLCDInactivity();
    });
  }

  if (btnCurRight) {
    attachHoldRepeat(btnCurRight, () => {
      if (lcdMdlModeActive()) {
        cycleModel(1); // next model
        resetLCDInactivity();
        return;
      }
      if (lcdBnkModeActive()) {
        cyclePatch(1); // next patch in the active bank
        resetLCDInactivity();
        return;
      }
      if (writeManager.isActive) {
        if (writeManager.state.phase === 'name') { // move cursor right through the name
          writeManager.moveCursor(1);
        } else { // slot phase: cursor cancels Write mode without saving
          exitWriteMode();
        }
        updateLCD();
        resetLCDInactivity();
        return;
      }
      lcdCompareMode = false;
      const menuList = activeMenuParameters();
      if (lcdMenuIndex === -1) {
        lcdMenuIndex = 0;
      } else {
        lcdMenuIndex = (lcdMenuIndex + 1) % menuList.length;
      }
      updateLCD();
      resetLCDInactivity();
    });
  }

  const modifyMenuValue = (isUp) => {
    // MDL mode: ▲/▼ step through the synth models (▲ forward, ▼ backward).
    if (lcdMdlModeActive()) {
      cycleModel(isUp ? 1 : -1);
      resetLCDInactivity();
      return;
    }
    // BNK mode: ▲/▼ step through the LIBRARY banks (▲ forward, ▼ backward).
    if (lcdBnkModeActive()) {
      cycleBank(isUp ? 1 : -1);
      resetLCDInactivity();
      return;
    }
    // Write mode: slot phase = pick the target slot, name phase = cycle the
    // character under the cursor through the LCD alphabet.
    if (writeManager.isActive) {
      if (writeManager.state.phase === 'name') {
        writeManager.cycleChar(isUp);
      } else {
        writeManager.cycleSlot(isUp);
      }
      updateLCD();
      resetLCDInactivity();
      return;
    }
    if (lcdMenuIndex === -1) return;
    const param = activeMenuParameters()[lcdMenuIndex];
    if (param.computed) {
      // AUTH K-TRACK is a read-only status readout (the route is edited in the
      // MOD drawer); ▲/▼ does nothing but keeps the inactivity timer alive.
      updateLCD();
      resetLCDInactivity();
      return;
    }
    if (param.uiSetting) {
      // WebUI-only UI setting (scroll timing): step the persisted value and
      // restart the running readout so the new speed/pause applies at once.
      const min = param.min !== undefined ? param.min : 0;
      const max = param.max !== undefined ? param.max : 100;
      const step = param.step || 1;
      const key = param.id === 'LCD_SCROLL_SPEED' ? 'speed' : 'pause';
      const cur = key === 'speed' ? getScrollSpeed() : getScrollPause();
      const val = isUp ? Math.min(max, cur + step) : Math.max(min, cur - step);
      setScrollSetting(key, val);
      [bnkLine1, bnkLine2, normalLine2, menuLine1, menuLine2, mdlLine1].forEach(s => s.restart());
      updateLCD();
      resetLCDInactivity();
      return;
    }
    const inputEl = document.getElementById(param.id);

    if (!inputEl) {
      // LCD-only parameter (no drawer control, e.g. Key Follow modes): step the
      // stored raw value and drive the engine directly via sendParameter. No
      // undo snapshot is pushed: capturePatchSnapshot only reads params with a
      // DOM control, so an undo step here could not restore this param anyway.
      const cur = getLcdRawValue(param.id);
      const step = param.step || 1;
      let val;
      if (param.type === 'choice') {
        const maxVal = param.choices.length - 1;
        val = isUp ? Math.min(maxVal, cur + 1) : Math.max(0, cur - 1);
      } else {
        const min = param.min !== undefined ? param.min : 0;
        const max = param.max !== undefined ? param.max : 100;
        val = isUp ? Math.min(max, cur + step) : Math.max(min, cur - step);
      }
      lcdParamRawValues[param.id] = val;
      sendParameter(param.id, rawToNormalized(param.id, val));
      updateLCD();
      resetLCDInactivity();
      return;
    }

    if (inputEl.tagName === 'BUTTON') {
      // Programmatic click() bypasses the global mousedown undo-arm listeners
      // (they only fire on real pointer events), so arm explicitly here — only
      // when the toggle actually changes state.
      const wasActive = inputEl.classList.contains('active');
      inputEl.click();
      const isActive = inputEl.classList.contains('active');
      if (wasActive !== isActive) pushUndo();
      updateLCD();
      resetLCDInactivity();
      return;
    }

    let val = parseFloat(inputEl.value || 0);
    const step = param.step || 1;
    if (param.type === 'choice') {
      const maxVal = param.choices.length - 1;
      val = isUp ? Math.min(maxVal, val + 1) : Math.max(0, val - 1);
    } else {
      const min = param.min !== undefined ? param.min : 0;
      const max = param.max !== undefined ? param.max : 100;
      val = isUp ? Math.min(max, val + step) : Math.max(min, val - step);
    }

    if (val !== parseFloat(inputEl.value || 0)) {
      pushUndo();
      inputEl.value = val;
      inputEl.dispatchEvent(new Event('input'));
      inputEl.dispatchEvent(new Event('change'));
    }

    updateLCD();
    resetLCDInactivity();
  };

  if (btnValUp) {
    attachHoldRepeat(btnValUp, () => {
      modifyMenuValue(true);
    });
  }

  if (btnValDown) {
    attachHoldRepeat(btnValDown, () => {
      modifyMenuValue(false);
    });
  }

  // SET button: same as Edit > Settings (System Mode) — toggles Setup Mode on
  // the LCD (native menuItemSelected 205 toggles SYSTEM/EDIT). While active the
  // button stays pressed; pressing again exits back to Preset Program Mode.
  if (btnSet) {
    btnSet.addEventListener('click', () => {
      toggleSetupMode();
      resetLCDInactivity();
    });
  }
}

// Initialize keypad listeners and update display on load
setupLCDKeypadListeners();
updateLCD();

// --- Per-model behavior (mirrors the native UI) ---
// The operation mode changes: which panels/controls are visible, the LCD color
// theme (LCDDisplay.cpp: Modern tints the LCD teal/cyan + glow and shows a
// "MODERN" label, CZ-5000 shows a "CZ-5000" label) and which LCD menu entries
// are reachable (LCDStateManager rebuilds its parameter list on mode change).
function syncLCDMode() {
  const display = document.getElementById('lcd-display');
  const badge = document.getElementById('lcd-mode-badge');
  const opModeEl = document.getElementById('OPERATION_MODE');
  const mode = opModeEl ? parseInt(opModeEl.value || 0) : 0;
  if (display) {
    display.classList.toggle('lcd-mode-modern', mode === 3);
    display.classList.toggle('lcd-mode-5000', mode === 2);
    display.classList.toggle('lcd-mode-cz1', mode === 1);
  }
  if (badge) {
    badge.textContent = mode === 3 ? 'MODERN' : (mode === 2 ? 'CZ-5000' : (mode === 1 ? 'CZ-1' : ''));
  }
  const brandModelNum = document.getElementById('brand-model-num');
  if (brandModelNum) {
    brandModelNum.textContent = mode === 3 ? 'Modern' : (mode === 2 ? '5000' : (mode === 1 ? '1' : '101'));
  }
}

function syncModePanels() {
  const opModeEl = document.getElementById('OPERATION_MODE');
  const mode = opModeEl ? parseInt(opModeEl.value || 0) : 0;

  // Modulation matrix section: Modern only (the CZ-101/CZ-5000 had no routing
  // matrix; it is an emulator addition).
  const modPanel = document.querySelector('.panel-modulation');
  if (modPanel) modPanel.style.display = (mode === 3) ? '' : 'none';

  // Macro modifiers: Modern only (like the native GeneralSection/macroPanel).
  const macrosPanel = document.getElementById('modern-macros-panel');
  if (macrosPanel) macrosPanel.style.display = (mode === 3) ? '' : 'none';

  // Arpeggiator: hidden in Classic 101 (like the native ArpeggiatorSection).
  const arpPanel = document.querySelector('.panel-arp');
  if (arpPanel) arpPanel.style.display = (mode === 0) ? 'none' : '';

  // Filters & Effects: two separate panels (native FiltersSection/EffectsSection).
  // FILTERS (LPF/HPF) only in Modern; EFFECTS chorus in Classic 5000 + Modern;
  // drive/delay/reverb only in Modern; nothing in Classic 101 (message instead).
  const filterPanel = document.getElementById('panel-filters');
  const effectsPanel = document.getElementById('panel-effects');
  const effectsClassicMsg = document.getElementById('effects-classic-msg');
  const modeKeys = [];
  if (mode >= 1) modeKeys.push('chorus', 'cz1');
  if (mode >= 2) modeKeys.push('cz5000');
  if (mode >= 3) modeKeys.push('modern');

  // Surface panels: FILTERS exists only in Modern; EFFECTS stays visible in
  // Classic 101 just to show the disabled message (EDIT hidden in that mode).
  if (filterPanel) filterPanel.style.display = (mode === 3) ? '' : 'none'; // Modern only
  if (effectsPanel) {
    effectsPanel.style.display = (mode === 0) ? 'none' : '';
    if (effectsClassicMsg) effectsClassicMsg.style.display = (mode === 0) ? '' : 'none';
  }
  // The controls live in the drawer sections; gate them per mode there too.
  // We scan all drawer sections and control panels so that velocity controls (in env panels), filters/effects, and surface buttons work.
  document.querySelectorAll('.drawer-section, .control-panel').forEach(sec => {
    sec.querySelectorAll('[data-mode]').forEach(p => {
      const pModes = (p.dataset.mode || '').split(' ');
      const match = pModes.some(m => modeKeys.includes(m));
      p.style.display = match ? '' : 'none';
    });
  });
  // MODULATION MATRIX (free slots) is a Modern-only feature.
  document.querySelectorAll('.drawer-section[data-drawer-section="mod"]').forEach(sec => {
    sec.style.display = (mode === 3) ? '' : 'none';
  });
}

function rebuildLcdMenu() {
  MENU_PARAMETERS = buildLcdMenu(currentOperationMode());
  const menuList = activeMenuParameters();
  if (menuList.length === 0) {
    lcdMenuIndex = -1;
  } else if (lcdMenuIndex >= menuList.length) {
    lcdMenuIndex = menuList.length - 1;
  }
  updateLCD();
}

const opModeEl = document.getElementById('OPERATION_MODE');
if (opModeEl) {
  opModeEl.addEventListener('input', () => { syncModePanels(); syncLCDMode(); rebuildLcdMenu(); applyModernMatrixSeeds(); });
  opModeEl.addEventListener('change', () => { syncModePanels(); syncLCDMode(); rebuildLcdMenu(); applyModernMatrixSeeds(); });
}
syncModePanels();
syncLCDMode();

  // Accessors for state app.js reads/writes outside this module.
  return {
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
    getLcdMenuIndex: () => lcdMenuIndex,
    setLcdMenuIndex: (v) => { lcdMenuIndex = v; },
    getLcdCompareMode: () => lcdCompareMode,
    setLcdCompareMode: (v) => { lcdCompareMode = v; },
    getLcdSetupMode: () => lcdSetupMode,
    getLcdBnkMode: () => modeApis.bnk.getLcdBnkMode(),
    getLcdMdlMode: () => modeApis.mdl.getLcdMdlMode(),
    getMenuParameters: () => MENU_PARAMETERS
  };
}
