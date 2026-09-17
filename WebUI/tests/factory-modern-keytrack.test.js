import { describe, it, expect } from 'vitest';
import { FACTORY_PRESETS } from '../src/contracts/factoryPresets.js';
import { PARAM_MAP, PARAMETER_REGISTRY } from '../src/contracts/registry.gen.js';

describe('Factory preset: Modern Key Track (Authentic Key Track source)', () => {
  it('exists in the factory bank', () => {
    const names = FACTORY_PRESETS.map(p => p.name);
    expect(names).toContain('Modern Key Track');
  });

  it('is a Modern (mode 2) preset', () => {
    const p = FACTORY_PRESETS.find(p => p.name === 'Modern Key Track');
    expect(p.params.OPERATION_MODE).toBe(2);
  });

  it('routes Authentic Key Track (11) → DCW (1) at depth +1.0', () => {
    const p = FACTORY_PRESETS.find(p => p.name === 'Modern Key Track');
    expect(p.params.MOD_SLOT_1_SRC).toBe(11);   // Authentic Key Track
    expect(p.params.MOD_SLOT_1_DEST).toBe(1);   // DCW
    expect(p.params.MOD_SLOT_1_DEPTH).toBe(1.0); // full hardware curve
  });

  it('restores the velocity→amp base via the matrix (Modern gates the fixed route)', () => {
    const p = FACTORY_PRESETS.find(p => p.name === 'Modern Key Track');
    expect(p.params.MOD_SLOT_2_SRC).toBe(1);   // Velocity
    expect(p.params.MOD_SLOT_2_DEST).toBe(2);  // DCA
    expect(p.params.MOD_SLOT_2_DEPTH).toBe(1.0);
  });

  it('all matrix params used are registered (raw values within range)', () => {
    const p = FACTORY_PRESETS.find(p => p.name === 'Modern Key Track');
    Object.entries(p.params).forEach(([id, raw]) => {
      const spec = PARAM_MAP.get(id);
      expect(spec, `unknown param "${id}"`).toBeDefined();
      expect(raw).toBeGreaterThanOrEqual(spec.min);
      expect(raw).toBeLessThanOrEqual(spec.max);
    });
  });

  it('keeps DCW sustain mid so the key-track curve is audible (not clamped)', () => {
    const p = FACTORY_PRESETS.find(p => p.name === 'Modern Key Track');
    expect(p.params.DCW_SUSTAIN).toBeLessThan(1.0);
    expect(p.params.DCW_SUSTAIN).toBeGreaterThan(0.1);
  });
});

describe('Preset load syncs the mode-driven UI (regression)', () => {
  const fs = require('node:fs');
  const path = require('node:path');
  const appJs = fs.readFileSync(path.resolve(__dirname, '../src/app.js'), 'utf8');
  // applyModernMatrixSeeds (which calls syncModSlotBadges internally) moved to
  // src/ui/modMatrix.js during the app split; app.js keeps the load path.
  const modJs = fs.readFileSync(path.resolve(__dirname, '../src/ui/modMatrix.js'), 'utf8');

  it('loads a preset carrying its own matrix without leaving badges stale', () => {
    // applyParameterToUI sets the MOD_SLOT controls but not the ON/OFF badges;
    // syncModSlotBadges must run on every preset/bank load (it used to be
    // skipped when the matrix was NOT empty, e.g. the factory Modern Key
    // Track preset → all slots showed OFF).
    expect(appJs).toContain('const syncUiAfterParamLoad = () => {');
    expect(modJs).toContain('syncModSlotBadges();');
    expect(appJs).toContain('syncModePanels();\n  syncLCDMode();\n  rebuildLcdMenu();\n  applyModernMatrixSeeds();');
    expect(appJs).toContain('syncUiAfterParamLoad();');
  });
});
