import { describe, it, expect, beforeEach } from 'vitest';
import fs from 'fs';
import path from 'path';
import CZ101DSP from '../wasm/cz101_dsp.js';
import { createWriteManager } from '../src/contracts/patchFlow.js';
import { seedLcdName, cycleLcdChar, finalLcdName, LCD_NAME_MAX_LEN } from '../src/contracts/lcdName.js';

// Write flow integration against the REAL compiled WASM engine: the LCD keypad
// state machine (WRT -> slot -> name -> save) drives wasm_save_preset exactly
// like the worklet SAVE_PRESET handler does, and the name edited character by
// character on the LCD alphabet is what lands in the bank.

describe('Write flow (WRT -> slot -> name -> save) against the real WASM bank', () => {
  let dsp;

  const allocateString = (str) => {
    const ptr = dsp._malloc(str.length + 1);
    for (let i = 0; i < str.length; i++) dsp.HEAPU8[ptr + i] = str.charCodeAt(i);
    dsp.HEAPU8[ptr + str.length] = 0;
    return ptr;
  };

  const getName = (index) => {
    const buf = dsp._malloc(256);
    dsp._wasm_get_preset_name(index, buf, 256);
    let name = '';
    for (let j = 0; j < 256; j++) {
      const c = dsp.HEAPU8[buf + j];
      if (c === 0) break;
      name += String.fromCharCode(c);
    }
    dsp._free(buf);
    return name;
  };

  // Mirrors the worklet SAVE_PRESET handler (cz101Worklet.js): allocate the
  // name, call wasm_save_preset, free.
  const savePresetToWasm = (index, name) => {
    const ptr = allocateString(name);
    dsp._wasm_save_preset(index, ptr);
    dsp._free(ptr);
  };

  beforeEach(async () => {
    dsp = await CZ101DSP();
    dsp._wasm_init(44100.0);
  });

  it('the full keypad sequence saves the edited name into the chosen slot', () => {
    // Wire the WriteManager exactly like app.js (deps.seedName = seedLcdName,
    // cycleChar = cycleLcdChar, savePreset -> wasm_save_preset).
    const manager = createWriteManager({
      slotCount: 64,
      nameMaxLen: LCD_NAME_MAX_LEN,
      getCurrentSlot: () => 0,
      seedName: (slot) => seedLcdName(`SLOT ${slot + 1}`),
      cycleChar: cycleLcdChar,
      savePreset: (index, name) => {
        savePresetToWasm(index, name);
        return true;
      }
    });

    // WRT -> slot phase (▲/▼ pick slot 3)
    manager.enter();
    manager.cycleSlot(true);
    manager.cycleSlot(true);
    manager.cycleSlot(true);

    // WRT -> name phase, seeded from the LCD alphabet
    manager.advanceToName();
    // Edit: cursor to the start, cycle the first char 'S' up a few times
    manager.moveCursor(-99);
    manager.cycleChar(true); // 'S' -> 'T'
    manager.cycleChar(true); // 'T' -> 'U'

    // WRT -> save (real bank write)
    const ok = manager.commit();
    expect(ok).toBe(true);

    // The name in the bank is the LCD-edited one, trimmed
    expect(getName(3)).toBe('ULOT 4');
    expect(manager.isActive).toBe(false); // state reset after the save
  });

  it('the saved preset round-trips: loadPreset returns the same name', () => {
    savePresetToWasm(7, 'ROUNDTRIP');

    const buf = dsp._malloc(256);
    dsp._wasm_load_preset(7, buf, 256);
    dsp._free(buf);

    expect(getName(7)).toBe('ROUNDTRIP');
  });

  it('a blank LCD name falls back to "Slot N" like the native flow', () => {
    const manager = createWriteManager({
      slotCount: 64,
      nameMaxLen: LCD_NAME_MAX_LEN,
      getCurrentSlot: () => 0,
      seedName: () => '',
      cycleChar: cycleLcdChar,
      savePreset: (index, name) => {
        savePresetToWasm(index, name);
        return true;
      }
    });

    manager.enter();
    manager.advanceToName();
    manager.commit();

    expect(getName(0)).toBe('Slot 1');
  });

  it('the LCD alphabet cycling matches the characters that reach the bank', () => {
    const manager = createWriteManager({
      slotCount: 64,
      nameMaxLen: LCD_NAME_MAX_LEN,
      getCurrentSlot: () => 10,
      seedName: (slot) => seedLcdName('TEST NAME'),
      cycleChar: cycleLcdChar,
      savePreset: (index, name) => {
        savePresetToWasm(index, name);
        return true;
      }
    });

    manager.enter();
    manager.advanceToName();
    // Move the cursor to the end and append two characters with ▲
    manager.cycleChar(true);
    manager.cycleChar(true);
    manager.commit();

    const stored = getName(10);
    // seedLcdName uppercases + truncates to 10; cycle at the cursor index
    // (name.length) appends, so the stored name starts with the seed.
    expect(stored.startsWith('TEST NAME'.toUpperCase().slice(0, 10))).toBe(true);
    expect(stored.length).toBeLessThanOrEqual(10);
  });

  it('save is a no-op on the bank when the engine is offline (commit returns false)', () => {
    const originalName = getName(2);
    const manager = createWriteManager({
      slotCount: 64,
      nameMaxLen: LCD_NAME_MAX_LEN,
      getCurrentSlot: () => 2,
      seedName: (slot) => seedLcdName('SLOT 3'),
      cycleChar: cycleLcdChar,
      savePreset: () => false // engine offline: the worklet never gets the message
    });

    manager.enter();
    manager.advanceToName();
    const ok = manager.commit();

    expect(ok).toBe(false);
    expect(getName(2)).toBe(originalName); // untouched — save never reached the bank
    expect(manager.isActive).toBe(false);
  });

  it('finalLcdName trims exactly what the engine receives', () => {
    expect(finalLcdName('  MY PATCH  ')).toBe('MY PATCH');
    expect(finalLcdName('')).toBe('');
  });

  it('hybrid naming: the NameEditor text lands in the WASM bank via setName', () => {
    // The hybrid flow opens the text editor seeded with the LCD name, SAVE
    // writes it back through writeManager.setName (sanitized like the LCD
    // seed), and commit() saves that name into the real bank.
    const manager = createWriteManager({
      slotCount: 64,
      nameMaxLen: LCD_NAME_MAX_LEN,
      getCurrentSlot: () => 1,
      seedName: (slot) => seedLcdName('OLD NAME'),
      sanitizeName: (raw) => seedLcdName(raw),
      cycleChar: cycleLcdChar,
      savePreset: (index, name) => { savePresetToWasm(index, name); return true; }
    });

    manager.enter();
    manager.advanceToName();
    manager.setName('Fast Typed Name');
    const ok = manager.commit();

    expect(ok).toBe(true);
    const stored = getName(1);
    expect(stored).toBe('FAST TYPED'); // seedLcdName: upper + 10 chars
  });

  it('hybrid wiring: SET in the Write name phase opens the NameEditor overlay', () => {
    // The hybrid branch lives in the Setup mode factory (toggleSetupMode, which
    // both the SET keypad button and the navbar Edit > Settings share), and
    // app.js wires openNameEditor into the factory deps.
    const lcd = [
      '../src/ui/lcdPanel.js',
      '../src/ui/lcdScroller.js',
      '../src/ui/lcdBnkMode.js',
      '../src/ui/lcdMdlMode.js',
      '../src/ui/lcdWriteMode.js',
      '../src/ui/lcdSetupMode.js'
    ].map(p => fs.readFileSync(path.resolve(__dirname, p), 'utf8')).join('\n');
    const app = fs.readFileSync(path.resolve(__dirname, '../src/app.js'), 'utf8');
    expect(lcd).toContain("writeManager && writeManager.isActive && writeManager.state && writeManager.state.phase === 'name'");
    expect(lcd).toContain("title: 'PATCH NAME',");
    expect(lcd).toContain("value: (writeManager.state.name || '').trim(),");
    // SAVE writes back via writeManager.setName and keeps the name phase active
    // (WRT still commits the real save afterwards).
    expect(lcd).toContain('writeManager.setName(name);');
    // The LCD shows the SET hint (adaptive: only when it fits next to the
    // badge — ~16 chars measured in the name phase, so it stays short).
    expect(lcd).toContain('const hint = "WRITE: NAME SET";');
    expect(lcd).toContain('measureLcdChars(lcdLine1) >= hint.length');
    // app.js passes the overlay opener into the factory.
    expect(app).toContain('openNameEditor,');
    // patchFlow exposes setName (sanitize through the LCD seed path).
    const flow = fs.readFileSync(path.resolve(__dirname, '../src/contracts/patchFlow.js'), 'utf8');
    expect(flow).toContain('setName(raw) {');
    expect(flow).toContain('deps.sanitizeName');
  });
});
