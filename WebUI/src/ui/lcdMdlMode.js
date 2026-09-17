// MDL (Model Browse) mode (extracted from lcdPanel.js).
// The MDL button sits next to BNK. While active, ▲/▶ advance and ▼/◀ step back
// through the three synth models (Classic 101 / Classic 5000 / Modern — the
// OPERATION_MODE choices), wrapping. Changing the model drives the same select
// as the drawer, so the engine, the LCD theme, panel gating and the LCD menu
// all follow (OPERATION_MODE input/change listeners). Line 1 shows the model
// name (scrolling like BNK), line 2 the position M<mode>/<total>.
//
// Factory pattern: receives the shared LCD ctx (DOM refs + lazy getters to the
// sibling modes) and returns the MDL API the core wires.
export function createMdlMode(ctx) {
  const {
    lcdLine1,
    lcdLine2,
    mdlLine1,
    // Lazy cross-mode getters (resolved at call time).
    getExitBnkMode,
    getExitSetupMode,
    getExitWriteMode,
    getUpdateLCD,
    getResetInactivity,
    getStopNormalLcdScroll,
    getLcdCompareMode,
    setLcdCompareMode,
    setLcdMenuIndex,
  } = ctx;

  let lcdMdlMode = false;

  // Wrappers keep the original internal names so call sites read naturally.
  const stopNormalLcdScroll = () => getStopNormalLcdScroll()();
  const exitBnkMode = () => getExitBnkMode()();
  const exitSetupMode = () => getExitSetupMode()();
  const exitWriteMode = () => getExitWriteMode()();
  const updateLCD = () => getUpdateLCD()();
  const resetLCDInactivity = () => getResetInactivity()();

  const MDL_NAMES = ['CLASSIC 101', 'CLASSIC CZ-1', 'CLASSIC 5000', 'MODERN']; // OPERATION_MODE choices

  const mdlNames = () => {
    const opModeEl = document.getElementById('OPERATION_MODE');
    const mode = opModeEl ? Math.min(2, Math.max(0, parseInt(opModeEl.value || 0) || 0)) : 0;
    return { name: MDL_NAMES[mode] || 'CLASSIC 101', mode, total: MDL_NAMES.length };
  };

  const renderMdlLcd = () => {
    if (!lcdLine1 || !lcdLine2) return;
    const { name, mode, total } = mdlNames();
    mdlLine1.set(name);
    lcdLine2.innerText = `M${mode + 1}/${total}`;
  };

  function enterMdlMode() {
    stopNormalLcdScroll();
    exitBnkMode(); // Browse modes are mutually exclusive
    setLcdCompareMode(false);
    exitSetupMode(); // unpress SET / leave System Mode
    exitWriteMode();
    setLcdMenuIndex(-1);
    lcdMdlMode = true;
    const btnMdl = document.getElementById('btn-lcd-mdl');
    if (btnMdl) btnMdl.classList.add('active');
    renderMdlLcd();
    resetLCDInactivity();
  }

  function exitMdlMode() {
    if (!lcdMdlMode) return;
    lcdMdlMode = false;
    mdlLine1.stop();
    const btnMdl = document.getElementById('btn-lcd-mdl');
    if (btnMdl) btnMdl.classList.remove('active');
    setLcdMenuIndex(-1);
    updateLCD();
  }

  function toggleMdlMode() {
    if (lcdMdlMode) exitMdlMode(); else enterMdlMode();
  }

  // ▲/▶ advance, ▼/◀ step back (wrapping). Model changes go through the
  // OPERATION_MODE control so the engine and mode-driven UI stay in sync.
  const cycleModel = (dir) => {
    const { mode, total } = mdlNames();
    const next = (mode + dir + total) % total;
    if (next === mode) return;
    const inputEl = document.getElementById('OPERATION_MODE');
    if (!inputEl) return;
    inputEl.value = next;
    inputEl.dispatchEvent(new Event('input'));
    inputEl.dispatchEvent(new Event('change'));
    renderMdlLcd();
    resetLCDInactivity();
  };

  return {
    mdlNames,
    renderMdlLcd,
    enterMdlMode,
    exitMdlMode,
    toggleMdlMode,
    cycleModel,
    getLcdMdlMode: () => lcdMdlMode,
    setLcdMdlMode: (v) => { lcdMdlMode = v; },
  };
}
