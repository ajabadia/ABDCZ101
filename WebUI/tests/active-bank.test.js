import { describe, it, expect } from 'vitest';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';

const __dirname = dirname(fileURLToPath(import.meta.url));
// The Bank Manager library bootstrap + model filter live in the extracted
// src/ui/bankManager.js module (post-refactor); app.js wires the factory.
const appJs = readFileSync(join(__dirname, '..', 'src', 'app.js'), 'utf8');
const bankJs = readFileSync(join(__dirname, '..', 'src', 'ui', 'bankManager.js'), 'utf8');

// El Bank Manager debe restaurar el último banco visitado al arrancar. Los
// bancos de fábrica tienen IDs ESTABLES (derivados de su fuente .syx), porque
// si se generaran con genId() (Date.now()) cada arranque el activeBankId
// persistido no resolvería y siempre caería a Factory CZ-101.
describe('active bank restore on startup', () => {
  it('factory banks use stable ids in buildDefaultLibrary', () => {
    // Factory CZ-101
    expect(bankJs).toContain("id: FACTORY_BANK_IDS.cz101");
    expect(bankJs).toMatch(/cz101: 'factory_cz101'/);
    // CZ Pack 1
    expect(bankJs).toMatch(/czpack1: 'factory_czpack1'/);
    // CZ-230S derived from the .syx filename
    expect(bankJs).toMatch(/cz230s: \(file\) => `factory_cz230s_\$\{file\.replace\('\.syx', ''\)\}`/);
  });

  it('buildDefaultLibrary passes the stable ids into createBank', () => {
    expect(bankJs).toMatch(/createBank\(\{\s*id: FACTORY_BANK_IDS\.cz101,/);
    expect(bankJs).toMatch(/createBank\(\{\s*id: FACTORY_BANK_IDS\.czpack1,/);
    expect(bankJs).toMatch(/createBank\(\{\s*id: FACTORY_BANK_IDS\.cz230s\(file\),/);
  });

  it('initBankLibrary restores the last visited bank (savedActive first, then stored.activeBankId)', () => {
    const m = bankJs.match(/const candidates = \[savedActive, stored\.activeBankId\];/);
    expect(m, 'restore candidates should prefer savedActive then stored.activeBankId').toBeTruthy();
    expect(bankJs).toMatch(/bankLib\.activeBankId = resolveActiveBankId\(bankLib, stored, savedActive\) \|\| defaults\.banks\[0\]\.id;/);
  });

  it('migrates old random ids to stable factory ids by source then name', () => {
    // Old persisted libraries stored genId() ids; the resolver must match the
    // stale bank by its .syx source, falling back to the display name.
    expect(bankJs).toMatch(/const bySource = oldBank\.source && \(bankLib\.banks \|\| \[\]\)\.find\(b => b\.source === oldBank\.source\);/);
    expect(bankJs).toMatch(/const byName = \(bankLib\.banks \|\| \[\]\)\.find\(b => b\.name === oldBank\.name\);/);
  });

  it('does not always force Factory CZ-101 when a stored active id resolves', () => {
    // The old bug: `if (!(savedActive && findBank(...))) bankLib.activeBankId = defaults.banks[0].id;`
    // would only restore user banks. The new code must not contain that pattern.
    expect(bankJs).not.toMatch(/if \(!\(savedActive && findBank\(bankLib, savedActive\)\)\)/);
  });
});

describe('bank model filter (find banks by synth model)', () => {
  it('renderBankSelect filters banks by bankModelFilterValue', () => {
    expect(bankJs).toMatch(/\.filter\(b => !model \|\| b\.model === model\)/);
    expect(bankJs).toMatch(/const model = bankModelFilterValue;/);
  });

  it('applyBankModelFilter auto-activates the first matching bank when the active one is hidden', () => {
    expect(bankJs).toMatch(/if \(bankModelFilterValue && active && active\.model !== bankModelFilterValue\)/);
    expect(bankJs).toMatch(/\(bankLib\.banks \|\| \[\]\)\.find\(b => b\.model === bankModelFilterValue\)/);
    expect(bankJs).toMatch(/activateBank\(firstMatch\.id\);/);
  });

  it('populates the filter from BANK_MODELS and resets to ALL on clear', () => {
    expect(bankJs).toMatch(/Object\.keys\(BANK_MODELS\)\.forEach/);
    expect(bankJs).toMatch(/new Option\(describeModel\(key\), key\)/);
    expect(bankJs).toMatch(/bankModelFilterValue = e\.target\.value;/);
  });
});
