import { describe, it, expect } from 'vitest';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';
import { MOD_MATRIX_EXAMPLE_SEEDS, isMatrixEmpty } from '../src/contracts/modMatrixDefaults.js';

const __dirname = dirname(fileURLToPath(import.meta.url));
const appJs = readFileSync(join(__dirname, '..', 'src', 'app.js'), 'utf8');
// The seeding logic lives in src/ui/modMatrix.js (app split); app.js still
// owns the import of the seed contract + the load/mode-change call sites.
const modJs = readFileSync(join(__dirname, '..', 'src', 'ui', 'modMatrix.js'), 'utf8');

// The source/dest option lists in the matrix (parity with the registry choices)
const SOURCES = ['None', 'Velocity', 'Mod Wheel', 'Aftertouch', 'Key Track', 'LFO', 'Env DCW', 'Env DCA', 'Env Pitch', 'Pitch Bend', 'Noise'];
const DESTS = ['None', 'DCW', 'DCA', 'Pitch', 'Vibrato', 'LFO Rate', 'Osc2 Detune', 'Pan'];

describe('mod matrix example seeds (new presets in Modern)', () => {
  it('seeds Velocity→DCA and KeyTrack→Pitch example slots', () => {
    expect(MOD_MATRIX_EXAMPLE_SEEDS).toEqual([
      { slot: 1, src: 1, dest: 2, depth: 1.0 }, // Velocity → DCA
      { slot: 2, src: 4, dest: 3, depth: 0.3 }  // Key Track → Pitch
    ]);
  });

  it('references valid source/dest indices', () => {
    MOD_MATRIX_EXAMPLE_SEEDS.forEach(({ slot, src, dest, depth }) => {
      expect(slot).toBeGreaterThanOrEqual(1);
      expect(slot).toBeLessThanOrEqual(8);
      expect(SOURCES[src], `slot ${slot} source`).toBeTruthy();
      expect(DESTS[dest], `slot ${slot} dest`).toBeTruthy();
      expect(depth).toBeGreaterThanOrEqual(-1);
      expect(depth).toBeLessThanOrEqual(1);
    });
  });

  it('isMatrixEmpty only when every source is None (0)', () => {
    expect(isMatrixEmpty([0, 0, 0, 0, 0, 0, 0, 0])).toBe(true);
    expect(isMatrixEmpty(['0', '0', '0', '0', '0', '0', '0', '0'])).toBe(true);
    expect(isMatrixEmpty([0, 1, 0, 0, 0, 0, 0, 0])).toBe(false);
    expect(isMatrixEmpty([0, 0, 0, 0, 0, 0, 0, 5])).toBe(false);
    expect(isMatrixEmpty([])).toBe(false);
    expect(isMatrixEmpty(null)).toBe(false);
  });

  it('app.js seeds only in Modern with an empty matrix, never clobbering a built routing', () => {
    // The seed contract is imported by the matrix module and by app.js (the
    // destructured helper is what the load paths call).
    expect(modJs).toContain("import { MOD_MATRIX_EXAMPLE_SEEDS, isMatrixEmpty } from '../contracts/modMatrixDefaults.js';");
    // The matrix module reads the LCD mode lazily (the LCD panel factory is
    // instantiated after it in app.js — mutually dependent modules).
    expect(modJs).toMatch(/if \(getCurrentOperationMode\(\) !== 2\) return;/);
    expect(modJs).toMatch(/if \(!isMatrixEmpty\(srcs\.map\(el => el\.value\)\)\) return;/);
    // Seeds write the DOM controls and dispatch events (engine gets the params)
    expect(modJs).toContain('srcEl.value = String(src);');
    expect(modJs).toContain('el.dispatchEvent(new Event(\'input\', { bubbles: true }));');
    expect(appJs).toContain('applyModernMatrixSeeds');
  });

  it('seeds are applied on preset/bank/sysex load and on mode change', () => {
    // onSysExLoaded + syncUiAfterParamLoad live in app.js; the opMode input /
    // change listeners moved to the LCD panel module with the mode sync.
    const appCalls = (appJs.match(/applyModernMatrixSeeds\(\);/g) || []).length;
    const lcdJs = readFileSync(join(__dirname, '..', 'src', 'ui', 'lcdPanel.js'), 'utf8');
    const lcdCalls = (lcdJs.match(/applyModernMatrixSeeds\(\);/g) || []).length;
    // onSysExLoaded + syncUiAfterParamLoad (app) + opMode input + opMode change (LCD)
    expect(appCalls + lcdCalls).toBeGreaterThanOrEqual(4);
    expect(appJs).toContain('const syncUiAfterParamLoad = () => {');
    expect(appJs).toContain('syncModePanels();\n  syncLCDMode();\n  rebuildLcdMenu();\n  applyModernMatrixSeeds();');
  });
});
