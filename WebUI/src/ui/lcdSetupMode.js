// SET (System Mode) mode (extracted from lcdPanel.js).
// While SET is pressed the LCD locks to the SYSTEM parameter page (MIDI
// CHANNEL, BEND RANGE, TRANSPOSE, MASTER TUNE, PROTECT, AUTH MODE), mirroring
// the native LCDStateManager::Mode::SYSTEM. The button stays pressed while the
// mode is active; pressing again exits back to Preset Program Mode. SET also
// opens the fast text editor when Write mode is on its name phase (the hybrid
// naming flow).
//
// Factory pattern: receives the shared LCD ctx (menu/state getters + lazy
// getters to the sibling modes) and returns the SET API the core wires.
export function createSetupMode(ctx) {
  const {
    openNameEditor,
    // Lazy cross-mode getters (resolved at call time).
    getWriteManager,
    getExitBnkMode,
    getExitMdlMode,
    getUpdateLCD,
    getResetInactivity,
    getStopNormalLcdScroll,
    getActiveMenuParameters,
    getLcdSetupMode,
    setLcdSetupMode,
    setLcdMenuIndex,
    getLcdBnkMode,
    getLcdMdlMode,
  } = ctx;

  // Wrappers keep the original internal names so call sites read naturally.
  const getWriteManagerRef = () => getWriteManager();
  const exitBnkMode = () => getExitBnkMode()();
  const exitMdlMode = () => getExitMdlMode()();
  const updateLCD = () => getUpdateLCD()();
  const resetLCDInactivity = () => getResetInactivity()();
  const stopNormalLcdScroll = () => getStopNormalLcdScroll()();
  const activeMenuParameters = () => getActiveMenuParameters()();

  function enterSetupMode() {
    stopNormalLcdScroll();
    setLcdSetupMode(true);
    const btnSet = document.getElementById('btn-lcd-set');
    if (btnSet) btnSet.classList.add('active');
    // Jump to the first SYSTEM parameter (native: setMode(SYSTEM) shows the
    // system page; the menu item also gets its checkmark).
    const list = activeMenuParameters();
    setLcdMenuIndex(list.length > 0 ? 0 : -1);
    updateLCD();
    resetLCDInactivity();
  }

  function exitSetupMode() {
    setLcdSetupMode(false);
    const btnSet = document.getElementById('btn-lcd-set');
    if (btnSet) btnSet.classList.remove('active');
    setLcdMenuIndex(-1); // back to Preset Program Mode
    updateLCD();
  }

  function toggleSetupMode() {
    const writeManager = getWriteManagerRef();
    if (writeManager && writeManager.isActive && writeManager.state && writeManager.state.phase === 'name') {
      openNameEditor({
        title: 'PATCH NAME',
        value: (writeManager.state.name || '').trim(),
        onSave: (name) => {
          writeManager.setName(name);
          updateLCD();
          resetLCDInactivity();
        }
      });
      return;
    }
    if (getLcdBnkMode()) exitBnkMode(); // Setup takes over the LCD (any entry path)
    if (getLcdMdlMode()) exitMdlMode();
    if (getLcdSetupMode()) exitSetupMode(); else enterSetupMode();
  }

  return {
    enterSetupMode,
    exitSetupMode,
    toggleSetupMode,
  };
}
