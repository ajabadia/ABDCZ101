import { describe, it, expect } from 'vitest';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';

const __dirname = dirname(fileURLToPath(import.meta.url));
const appJs = readFileSync(join(__dirname, '..', 'src', 'app.js'), 'utf8');
// The Init Bank action moved to the extracted navbar module.
const navbarJs = readFileSync(join(__dirname, '..', 'src', 'ui', 'navbar.js'), 'utf8');

// Native PresetBrowser::initBank parity: the destructive factory reset shows an
// AlertWindow ("Are you sure? This will replace all presets with factory
// defaults.", Reset / Cancel) and only resets when confirmed.

describe('Init Bank confirmation (native PresetBrowser::initBank parity)', () => {
  it('asks for confirmation before replacing all presets', () => {
    const idx = navbarJs.indexOf('initBank: () => {');
    expect(idx).toBeGreaterThan(-1);
    const block = navbarJs.slice(idx, idx + 700);
    // Native text, verbatim
    expect(block).toContain("'Are you sure? This will replace all presets with factory defaults.'");
    expect(block).toContain('if (!window.confirm(');
    // Only proceeds to the reset after confirmation
    expect(block).toContain('return;');
    expect(block).toContain('engine.loadBank(makeDefaultBankJson(),');
  });

  it('still pushes undo and resets to the factory bank when confirmed', () => {
    const idx = navbarJs.indexOf('initBank: () => {');
    const block = navbarJs.slice(idx, idx + 700);
    expect(block).toContain('pushUndo();');
    expect(block).toContain('makeDefaultBankJson()');
    expect(block).toContain('PARAMETER_REGISTRY.parameters.map(p => p.id)');
  });

  it('delete bank already had its confirmation (kept intact)', () => {
    // The Bank Manager CRUD lives in the extracted src/ui/bankManager.js module.
    const bankJs = readFileSync(join(__dirname, '..', 'src', 'ui', 'bankManager.js'), 'utf8');
    expect(bankJs).toContain('Delete bank "${b.name}"?');
  });
});
