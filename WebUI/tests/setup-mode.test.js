import { describe, it, expect } from 'vitest';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';

const __dirname = dirname(fileURLToPath(import.meta.url));
const appJs = readFileSync(join(__dirname, '..', 'src', 'app.js'), 'utf8');
// The SET keypad state machine lives in the extracted LCD modules (core +
// the Setup mode factory; Write mode also exits SET).
const lcdJs = [
  join(__dirname, '..', 'src', 'ui', 'lcdPanel.js'),
  join(__dirname, '..', 'src', 'ui', 'lcdScroller.js'),
  join(__dirname, '..', 'src', 'ui', 'lcdBnkMode.js'),
  join(__dirname, '..', 'src', 'ui', 'lcdMdlMode.js'),
  join(__dirname, '..', 'src', 'ui', 'lcdWriteMode.js'),
  join(__dirname, '..', 'src', 'ui', 'lcdSetupMode.js')
].map(p => readFileSync(p, 'utf8')).join('\n');
const mainCss = readFileSync(join(__dirname, '..', 'src', 'styles', 'lcd.css'), 'utf8');

// SET (Settings / System Mode) parity with the native PluginEditor.cpp
// menuItemSelected 205: pressing SET toggles LCDStateManager between SYSTEM and
// EDIT and the menu item shows a checkmark. In the web UI the SET button must
// stay pressed while System Mode is active and unpress when toggled off.

describe('SET button toggle (System Mode)', () => {
  it('keeps an lcdSetupMode state variable', () => {
    expect(lcdJs).toContain('let lcdSetupMode = false;');
  });

  it('has enter/exit/toggle helpers that (un)press the SET button', () => {
    expect(lcdJs).toContain('function enterSetupMode()');
    expect(lcdJs).toContain('function exitSetupMode()');
    expect(lcdJs).toContain('function toggleSetupMode()');
    expect(lcdJs).toContain("btnSet.classList.add('active')");
    expect(lcdJs).toContain("btnSet.classList.remove('active')");
  });

  it('SET button listener toggles Setup Mode (does not one-shot jump)', () => {
    expect(lcdJs).toContain('btnSet.addEventListener(\'click\', () => {\n      toggleSetupMode();');
    expect(lcdJs).not.toContain('btnSet.addEventListener(\'click\', () => {\n      navbarActions.settingsSystem();');
  });

  it('Edit > Settings / Ctrl+, toggles the same mode (native case 205 parity)', () => {
    // The navbar actions moved to the extracted navbar module.
    const navbarJs = readFileSync(join(__dirname, '..', 'src', 'ui', 'navbar.js'), 'utf8');
    expect(navbarJs).toContain('settingsSystem: () => {\n      // Native (PluginEditor.cpp case 205): toggles LCDStateManager between');
    expect(navbarJs).toContain('toggleSetupMode();');
  });

  it('locks LCD navigation to SYSTEM parameters while in Setup Mode', () => {
    // activeMenuParameters() filters the keypad list to p.system entries and is
    // used by updateLCD, the cursor handlers, value editing and rebuildLcdMenu.
    expect(lcdJs).toContain('function activeMenuParameters()');
    expect(lcdJs).toContain('MENU_PARAMETERS.filter(p => p.system)');
    expect(lcdJs).toContain('const param = activeMenuParameters()[lcdMenuIndex]');
    expect(lcdJs).toContain('const menuList = activeMenuParameters()');
    expect(lcdJs).toContain('const menuList = activeMenuParameters();');
  });

  it('exits Setup Mode on inactivity, Compare and Write (LCD is taken over)', () => {
    expect(lcdJs).toContain('exitSetupMode(); // unpress SET and leave System Mode');
    expect(lcdJs).toContain('exitSetupMode(); // Compare takes over the LCD: unpress SET');
    expect(lcdJs).toContain('exitSetupMode(); // Write flow takes over the LCD: unpress SET');
  });

  it('has a pressed visual state for the SET button in the CSS', () => {
    expect(mainCss).toContain('.lcd-btn-set.active');
    expect(mainCss).toContain('box-shadow: 0 0 12px var(--color-accent-dim);');
  });
});
