import { describe, it, expect } from 'vitest';
import fs from 'fs';
import path from 'path';
import { PARAM_MAP, PARAMETER_REGISTRY } from '../src/contracts/registry.gen.js';
import { LCD_MENU_ITEMS, buildLcdMenu } from '../src/contracts/lcdMenu.js';

// The LCD panel internals were split into the core + focused mode modules
// (scroller, BNK, MDL, Write, Setup); tests read them all concatenated.
const readLcdModules = () => [
  '../src/ui/lcdPanel.js',
  '../src/ui/lcdScroller.js',
  '../src/ui/lcdBnkMode.js',
  '../src/ui/lcdMdlMode.js',
  '../src/ui/lcdWriteMode.js',
  '../src/ui/lcdSetupMode.js'
].map(p => fs.readFileSync(path.resolve(__dirname, p), 'utf8')).join('\n');

describe('LCD Menu Parity Tests', () => {
  it('should only reference parameters that exist in the registry', () => {
    LCD_MENU_ITEMS.forEach(item => {
      // uiSetting entries are WebUI-only UI settings (scroll timing) — they
      // deliberately live OUTSIDE the parameter registry (no sysex, no engine).
      if (item.uiSetting) return;
      expect(PARAM_MAP.has(item.id), `LCD menu references unknown parameter "${item.id}"`).toBe(true);
    });
  });

  it('should only reference parameters that have a DOM control or are LCD-editable', () => {
    const htmlPath = path.resolve(__dirname, '../index.html');
    const htmlContent = fs.readFileSync(htmlPath, 'utf8');
    const domIdRegex = /<(?:input|select|button)\s+[^>]*id=["']([^"']+)["']/gi;
    const foundDomIds = [];
    let match;
    while ((match = domIdRegex.exec(htmlContent)) !== null) {
      foundDomIds.push(match[1]);
    }
    const domIdsSet = new Set(foundDomIds);
    // The Key Follow modes have no drawer control anymore (the hardware ROUTES
    // were removed — replicable via the free matrix) but the LCD edits them
    // directly via sendParameter, so they are not "NOT BOUND".
    const lcdEditableWithoutDom = new Set(['KEY_FOLLOW_DCO', 'KEY_FOLLOW_DCW', 'KEY_FOLLOW_DCA', 'BYPASS', 'SYSTEM_PRG']);
    LCD_MENU_ITEMS.forEach(item => {
      // uiSetting entries are edited by app.js directly (no DOM control, no
      // registry param — the LCD is their only surface, like the LCD-only ones).
      if (item.uiSetting) return;
      const ok = domIdsSet.has(item.id) || lcdEditableWithoutDom.has(item.id);
      expect(ok, `LCD menu parameter "${item.id}" has no DOM control and is not LCD-editable`).toBe(true);
    });
  });

  it('should not use the stale LFO_WAVEFORM id (real id is LFO_WAVE)', () => {
    const ids = LCD_MENU_ITEMS.map(i => i.id);
    expect(ids).not.toContain('LFO_WAVEFORM');
    expect(ids).toContain('LFO_WAVE');
    expect(PARAM_MAP.has('LFO_WAVE')).toBe(true);
  });

  it('should flag exactly the SYSTEM parameters as system-only (SET / System Mode)', () => {
    // Parity with LCDStateManager::buildParameterList Mode::SYSTEM: only the
    // system page (MIDI, BEND RANGE, TRANSPOSE, MASTER TUNE, PROTECT, MODEL) is
    // reachable while the SET button is pressed.
    const systemIds = LCD_MENU_ITEMS.filter(i => i.system).map(i => i.id);
    expect(systemIds).toEqual([
      'MIDI_CH',
      'PITCH_BEND_RANGE',
      'KEY_TRANSPOSE', // native LCDStateManager Mode::SYSTEM lists "TRANSPOSE"
      'MASTER_VOLUME',
      'MASTER_TUNE',
      'OPERATION_MODE',
      'PROTECT_SWITCH',
      'HARDWARE_NOISE',
      // WebUI-only UI settings (no registry param): the LCD scroll timing,
      // adjustable from SET > SYSTEM like a native UI preference.
      'LCD_SCROLL_SPEED',
      'LCD_SCROLL_PAUSE'
    ]);
    // Nothing else is flagged
    expect(LCD_MENU_ITEMS.filter(i => !i.system).some(i => i.id === 'LINE_SELECT')).toBe(true);
    expect(LCD_MENU_ITEMS.filter(i => !i.system).some(i => i.id === 'LFO_RATE')).toBe(true);
  });

  it('exposes OPERATION_MODE in the SYSTEM page as MODEL (MDL parity)', () => {
    const item = LCD_MENU_ITEMS.find(i => i.id === 'OPERATION_MODE');
    expect(item).toBeTruthy();
    expect(item.system).toBe(true);
    // The LCD label matches the MDL button (the model can be changed from the
    // menu as well as from the new button).
    expect(item.name).toBe('MODEL');
    // The edit path reuses the generic choice-param handling (select element).
    // MDL_NAMES lives in the extracted MDL mode module.
    const lcdJs = readLcdModules();
    expect(lcdJs).toContain("const MDL_NAMES = ['CLASSIC 101', 'CLASSIC CZ-1', 'CLASSIC 5000', 'MODERN'];");
  });

  it('should keep LFO_RATE / LFO_DELAY ranges consistent with the registry', () => {
    const lfoRate = PARAM_MAP.get('LFO_RATE');
    const lfoDelay = PARAM_MAP.get('LFO_DELAY');
    // Regression: the old LCD menu used 0..1 for both, but the APVTS/registry
    // defines LFO_RATE 0.1..30 Hz and LFO_DELAY 0..2 s.
    expect(lfoRate.min).toBeCloseTo(0.1);
    expect(lfoRate.max).toBeCloseTo(30);
    expect(lfoDelay.min).toBeCloseTo(0);
    expect(lfoDelay.max).toBeCloseTo(2);
  });

  it('renders the AUTH K-TRACK readout from the free matrix (module + wiring)', () => {
    const matrixJs = fs.readFileSync(path.resolve(__dirname, '../src/ui/modMatrix.js'), 'utf8');
    const lcdJs = readLcdModules();
    const appJs = fs.readFileSync(path.resolve(__dirname, '../src/app.js'), 'utf8');
    // The LCD value comes from the matrix slots (source 11 = Authentic Key
    // Track), is read-only, and refreshes when a slot changes while displayed.
    // The readout itself lives in the mod-matrix module (post-refactor); the
    // LCD panel wires it into the menu readout (scrolls like BNK/normal).
    expect(matrixJs).toContain("const getAuthKeyTrackLcdValue = () => {");
    expect(matrixJs).toContain('parseInt(src.value) !== 11'); // 11 = Authentic Key Track
    expect(lcdJs).toContain("if (param.computed) {")
    expect(lcdJs).toContain('menuLine2.set(getAuthKeyTrackLcdValue());');
    expect(lcdJs).toContain("// AUTH K-TRACK is a read-only status readout");
    expect(lcdJs).toContain('const param = activeMenuParameters()[lcdMenuIndex];');
    // app.js passes the module's readout into the LCD panel factory wiring.
    expect(appJs).toContain('getAuthKeyTrackLcdValue,');
  });

  it('adds the computed AUTH K-TRACK readout only in Modern mode', () => {
    // Parity with the native "K-TRACK DCW/PIT" LCD rows (LCDStateManager
    // Section::MOD): the free matrix routes the Authentic Key Track curve
    // (source 11), and the LCD shows the active route. It is a computed,
    // read-only entry (no registry param) and only reachable in Modern (the
    // free matrix is Modern-only).
    [0, 1, 2].forEach(mode => {
      const ids = buildLcdMenu(mode).map(m => m.id);
      expect(ids, `AUTH_KTRACK must not exist in mode ${mode}`).not.toContain('AUTH_KTRACK');
    });
    const modern = buildLcdMenu(3);
    const entry = modern.find(m => m.id === 'AUTH_KTRACK');
    expect(entry).toBeDefined();
    expect(entry.name).toBe('AUTH K-TRACK');
    expect(entry.computed).toBe(true);
    expect(entry.type).toBe('computed');
    // It must not collide with a real registry param (computed, not editable).
    expect(PARAM_MAP.has('AUTH_KTRACK')).toBe(false);
  });

  it('keeps only the Key Follow modes in the LCD (routes live in the free matrix)', () => {
    // The fixed hardware ROUTES were removed from the LCD menu: every one of
    // them (Velo->DCW/DCA, Wheel->Vib, KeyTrack->Pitch, ...) is replicable with
    // the free 8-slot matrix in the MOD drawer. Only the Key Follow MODES
    // (OFF/FIX/VAR) stay — they are mode switches, not routes, and mirror the
    // native LCDStateManager Section::MOD. They are available in every mode.
    const removedRoutes = ['MOD_VELO_DCW', 'MOD_VELO_DCA', 'MOD_WHEEL_DCW', 'MOD_WHEEL_LFORATE',
      'MOD_WHEEL_VIB', 'MOD_AT_DCW', 'MOD_AT_VIB', 'KEY_TRACK_DCW', 'KEY_TRACK_PITCH'];
    const kfIds = ['KEY_FOLLOW_DCO', 'KEY_FOLLOW_DCW', 'KEY_FOLLOW_DCA'];

    [0, 1, 2].forEach(mode => {
      const ids = buildLcdMenu(mode).map(m => m.id);
      removedRoutes.forEach(id => expect(ids, `"${id}" must not be in the LCD menu (mode ${mode})`).not.toContain(id));
      kfIds.forEach(id => expect(ids, `"${id}" must stay in the LCD menu (mode ${mode})`).toContain(id));
    });
  });

  it('setLCDParam positions the LCD index within the ACTIVE (SET-locked) list', () => {
    // Regression (browser crash): pressing ▲ on TRANSPOSE while SET (System
    // Mode) threw "Cannot read properties of undefined (reading 'computed')".
    // setLCDParam set lcdMenuIndex to the FULL MENU_PARAMETERS index, but
    // updateLCD reads activeMenuParameters() — in SET mode that is the short
    // SYSTEM-only list, so the index pointed past its end (undefined param).
    const appJs = fs.readFileSync(path.resolve(__dirname, '../src/app.js'), 'utf8');
    const lcdJs = readLcdModules();
    // setLCDParam must resolve the index against the currently navigable list
    // (it lives in app.js and writes through the panel's setLcdMenuIndex).
    expect(appJs).toContain('const activeIdx = activeMenuParameters().findIndex(p => p.id === id);');
    expect(appJs).toContain('if (activeIdx !== -1) setLcdMenuIndex(activeIdx);');
    // updateLCD defensively clamps a stale index back into the active list.
    expect(lcdJs).toContain('if (lcdMenuIndex < 0 || lcdMenuIndex >= menuList.length) {');
  });

  it('LCD keypad edits (▲/▼) push undo snapshots (custom controls, not covered by global mousedown)', () => {
    const lcd = readLcdModules();
    // Regression: modifyMenuValue (▲/▼) drives params via sendParameter/click
    // but the global undo-arm listeners only fire for real pointer events on
    // PARAM_MAP ids, so keypad edits were never undoable. Each effective press
    // must push one undo step; no-op presses (clamped/unchanged) must not.
    // (The keypad state machine lives in the extracted LCD panel module.)
    expect(lcd).toContain("if (wasActive !== isActive) pushUndo();");
    expect(lcd).toContain("if (val !== parseFloat(inputEl.value || 0)) {").length > 0;
    expect(lcd).toContain('pushUndo();\n      inputEl.value = val;');
    // The write-mode branches (slot/name cycling) must stay outside undo
    expect(lcd).toContain("if (writeManager.isActive) {");
    expect(lcd).toContain("if (param.computed) {").length > 0;
  });

  it('should have a registry fully covered by the DOM (dynamic matrix slots allowed)', () => {
    const htmlPath = path.resolve(__dirname, '../index.html');
    const htmlContent = fs.readFileSync(htmlPath, 'utf8');
    const domIdRegex = /<(?:input|select|button)\s+[^>]*id=["']([^"']+)["']/gi;
    const foundDomIds = [];
    let match;
    while ((match = domIdRegex.exec(htmlContent)) !== null) {
      foundDomIds.push(match[1]);
    }
    const domIdsSet = new Set(foundDomIds);
    // The free mod-matrix slots (MOD_SLOT_<n>_SRC/DEST/DEPTH) are created
    // dynamically at runtime (ABDEEP-style 8-slot matrix in the drawer), and
    // the fixed HARDWARE ROUTES (MOD_VELO_*, MOD_WHEEL_*, MOD_AT_*, KEY_*)
    // had their drawer controls removed (replicable via the free matrix).
    // Everything else must exist in the static HTML.
    const dynamic = new Set([
      ...Array.from({ length: 8 }, (_, i) => [`MOD_SLOT_${i + 1}_SRC`, `MOD_SLOT_${i + 1}_DEST`, `MOD_SLOT_${i + 1}_DEPTH`]).flat(),
      'MOD_VELO_DCW', 'MOD_VELO_DCA',
      'MOD_WHEEL_VIB', 'MOD_WHEEL_DCW', 'MOD_WHEEL_LFORATE',
      'KEY_TRACK_DCW', 'KEY_TRACK_PITCH',
      'MOD_AT_DCW', 'MOD_AT_VIB',
      'KEY_FOLLOW_DCO', 'KEY_FOLLOW_DCW', 'KEY_FOLLOW_DCA',
      'DCA_ATTACK', 'DCA_DECAY', 'DCA_SUSTAIN', 'DCA_RELEASE',
      'DCW_ATTACK', 'DCW_DECAY', 'DCW_SUSTAIN', 'DCW_RELEASE',
      'ARP_OCTAVE', 'BYPASS', 'SYSTEM_PRG'
    ]);
    PARAMETER_REGISTRY.parameters.forEach(p => {
      if (dynamic.has(p.id)) return;
      expect(domIdsSet.has(p.id), `Registry parameter "${p.id}" is missing from the index.html DOM`).toBe(true);
    });
  });

  it('LCD menu readout scrolls long parameter names and values (generic scroller)', () => {
    const lcd = readLcdModules();
    // The menu readout uses the same generic scroller as BNK / normal mode,
    // for BOTH lines (param name on line 1, value / choice label on line 2).
    expect(lcd).toContain('const menuLine1 = createLcdScroller(lcdLine1);');
    expect(lcd).toContain('const menuLine2 = createLcdScroller(lcdLine2);');
    // Every menu value render path goes through the scroller (computed entry,
    // LCD-only parameter, choice, int and float).
    expect(lcd).toContain('menuLine1.set(param.name);');
    expect(lcd).toContain('menuLine2.set(getAuthKeyTrackLcdValue());');
    expect(lcd).toContain('menuLine2.set(String(param.choices[raw]).toUpperCase());');
    expect(lcd).toContain('menuLine2.set(((param.choices && param.choices[val]) || val).toString().toUpperCase());');
    expect(lcd).toContain('menuLine2.set(`${val} ${param.unit || \'\'}`.trim().toUpperCase());');
    // Leaving the menu readout stops its scrollers (other LCD modes take over).
    expect(lcd).toContain('const renderingMenuReadout = lcdMenuIndex !== -1 && !lcdCompareMode && !lcdBnkMode && !lcdMdlMode && !writeManager.isActive;');
    expect(lcd).toContain('menuLine1.stop();\n    menuLine2.stop();');
  });

  it('normal program mode scrolls long preset names on LCD line 2', () => {
    const lcd = readLcdModules();
    const appJs = fs.readFileSync(path.resolve(__dirname, '../src/app.js'), 'utf8');
    // The normal (PRG) readout uses the same LCD scroll as Bank Browse for the
    // preset name, instead of a static innerText that would overflow.
    expect(lcd).toContain('const normalLine2 = createLcdScroller(lcdLine2);');
    expect(lcd).toContain('normalLine2.set(activeName.toUpperCase());');
    // Line 1 keeps the short PRG readout (only line 2 can scroll in normal mode).
    expect(lcd).toContain('lcdLine1.innerText = `PRG: ${activeSlot}`;');
    // Another LCD mode must cancel the normal scroll so its timer never writes
    // over the active readout (compare/write/setup/bnk all stop it).
    expect(lcd).toContain('const stopNormalLcdScroll = () => {');
    expect(lcd).toContain('if (!renderingNormalReadout) stopNormalLcdScroll();');
    expect(lcd).toContain('stopNormalLcdScroll();\n    exitMdlMode(); // Browse modes are mutually exclusive\n    setLcdCompareMode(false);\n    exitSetupMode(); // unpress SET / leave System Mode\n    exitWriteMode();\n    lcdBnkMode = true;');
    // The engine preset-load echo also starts the scroll for long names (the
    // module exposes normalLine2 to app.js for the engine callbacks).
    expect(appJs).toContain('normalLine2.set(name.toUpperCase());');
    expect(appJs).toContain('normalLine2,');
  });

  it('exposes SCROLL SPEED / SCROLL PAUSE as SYSTEM uiSetting entries with specs', () => {
    // WebUI-only UI settings (native has a Scroll UI preference): reachable
    // from SET > SYSTEM, carry their own spec (no registry param), and are
    // flagged uiSetting so the registry/DOM parity checks skip them.
    const speed = LCD_MENU_ITEMS.find(i => i.id === 'LCD_SCROLL_SPEED');
    const pause = LCD_MENU_ITEMS.find(i => i.id === 'LCD_SCROLL_PAUSE');
    expect(speed).toBeTruthy();
    expect(pause).toBeTruthy();
    expect(speed.system).toBe(true);
    expect(pause.system).toBe(true);
    expect(speed.uiSetting).toBe(true);
    expect(pause.uiSetting).toBe(true);
    expect(speed.name).toBe('SCROLL SPEED');
    expect(pause.name).toBe('SCROLL PAUSE');
    // The built menu carries the spec (type/min/max/step/unit) so the LCD
    // keypad can step them without a DOM control.
    const built = buildLcdMenu(0);
    const bSpeed = built.find(i => i.id === 'LCD_SCROLL_SPEED');
    const bPause = built.find(i => i.id === 'LCD_SCROLL_PAUSE');
    expect(bSpeed.type).toBe('int');
    expect(bSpeed.min).toBe(40);
    expect(bSpeed.max).toBe(400);
    expect(bSpeed.step).toBe(10);
    expect(bSpeed.unit).toBe('MS');
    expect(bPause.min).toBe(0);
    expect(bPause.max).toBe(5000);
    expect(bPause.step).toBe(100);
  });

  it('the LCD panel persists and applies the scroll settings (speed + pause)', () => {
    const lcd = readLcdModules();
    // Settings live in localStorage (UI preference, no engine / sysex) and are
    // clamped to the menu ranges on load.
    expect(lcd).toContain("const LCD_SCROLL_SETTINGS_KEY = 'cz101.lcdScroll';");
    expect(lcd).toContain('speed: clampInt(raw.speed, 40, 400, LCD_SCROLL_STEP_MS),');
    expect(lcd).toContain('pause: clampInt(raw.pause, 0, 5000, LCD_SCROLL_DELAY_MS),');
    expect(lcd).toContain('const saveLcdScrollSettings = () => {');
    // startLcdScroll reads the LIVE settings each scroll, not the constants.
    expect(lcd).toContain('}, getScrollSpeed());');
    expect(lcd).toContain('}, getScrollPause());');
    // The scroller can restart with the new timing after a settings change.
    expect(lcd).toContain('restart() {');
    // The menu keypad edits uiSetting entries directly (no engine round-trip):
    // steps the store, persists it, and restarts the running readouts.
    expect(lcd).toContain("const key = param.id === 'LCD_SCROLL_SPEED' ? 'speed' : 'pause';");
    expect(lcd).toContain('saveLcdScrollSettings();');
    expect(lcd).toContain('forEach(s => s.restart());');
    // updateLCD renders them from the store (value + unit on line 2).
    expect(lcd).toContain("const val = param.id === 'LCD_SCROLL_SPEED' ? getScrollSpeed() : getScrollPause();");
  });

  it('keypad cursor/value buttons use hold-to-repeat (native setRepeatSpeed 400/60)', () => {
    const lcd = readLcdModules();
    // The four cursor/value buttons are wired through the shared hold-repeat
    // helper instead of plain click listeners (native LCDKeypad
    // cursorUp/Down/Left/Right.setRepeatSpeed(400, 60)).
    expect(lcd).toContain("import { attachHoldRepeat } from '../contracts/holdRepeat.js';");
    expect(lcd).toContain('attachHoldRepeat(btnCurLeft, () => {');
    expect(lcd).toContain('attachHoldRepeat(btnCurRight, () => {');
    expect(lcd).toContain('attachHoldRepeat(btnValUp, () => {');
    expect(lcd).toContain('attachHoldRepeat(btnValDown, () => {');
    // The helper itself carries the native timings (used only in the module now).
    expect(lcd).not.toContain("addEventListener('click', () => {\n      modifyMenuValue(true);");
  });

  it('physical arrow keys map to the LCD keypad cursors (native keyPressed parity)', () => {
    // The arrow-key mapping lives in the extracted navbar module.
    const app = fs.readFileSync(path.resolve(__dirname, '../src/ui/navbar.js'), 'utf8');
    // ArrowUp/Down/Left/Right drive the same keypad buttons the LCD menu uses;
    // guarded so typing in inputs/selects is never hijacked.
    expect(app).toContain('ArrowUp: \'btn-lcd-val-up\'');
    expect(app).toContain('ArrowDown: \'btn-lcd-val-down\'');
    expect(app).toContain('ArrowLeft: \'btn-lcd-cur-left\'');
    expect(app).toContain('ArrowRight: \'btn-lcd-cur-right\'');
    expect(app).toContain('if (isEditableTarget(e.target)) return;');
  });
});
