import { describe, it, expect } from 'vitest';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';

const __dirname = dirname(fileURLToPath(import.meta.url));
const appJs = readFileSync(join(__dirname, '..', 'src', 'app.js'), 'utf8');
const mainCss = readFileSync(join(__dirname, '..', 'src', 'styles', 'themes.css'), 'utf8');

describe('drag & drop file loading (native PluginEditor::filesDropped parity)', () => {
  it('accepts .syx/.sysex/.json drops and ignores other extensions', () => {
    // Native isInterestedInFileDrag only accepts .syx and .json
    expect(appJs).toContain("const DROP_EXT_RE = /\\.(syx|sysex|json)$/i;");
    expect(appJs).toContain("Array.from(e.dataTransfer?.types || []).includes('Files')");
  });

  it('routes .json drops like File > Load Bank (loadBank into the engine)', () => {
    expect(appJs).toContain("audioEngine.loadBank(event.target.result, PARAMETER_REGISTRY.parameters.map(p => p.id));");
  });

  it('routes .syx drops through the Bank Manager import flow (library bank + engine parse)', () => {
    expect(appJs).toContain("pendingSyxImport = { name: file.name.replace(/\\.(syx|sysex)$/i, '') };");
    expect(appJs).toContain("audioEngine.loadSysEx(new Uint8Array(event.target.result), PARAMETER_REGISTRY.parameters.map(p => p.id));");
  });

  it('shows a drop affordance (body.drag-over) while dragging files over the window', () => {
    expect(appJs).toContain("document.body.classList.add('drag-over')");
    expect(appJs).toContain("document.body.classList.remove('drag-over')");
    expect(mainCss).toContain('body.drag-over::after');
    expect(mainCss).toContain('DROP BANK / PATCH');
  });
});

describe('last visited preset remembered across reloads (tied to the active bank)', () => {
  it('persists the preset slot on user selection (loadPresetSlot)', () => {
    expect(appJs).toContain("const PRESET_SLOT_STORAGE_KEY = 'cz101.activePresetSlot';");
    expect(appJs).toContain('const saveActivePresetSlot = (slot) => {');
    expect(appJs).toContain('localStorage.setItem(PRESET_SLOT_STORAGE_KEY, JSON.stringify({ bankId, slot }))');
    // loadPresetSlot (the funnel for select/prev/next) saves the slot
    expect(appJs).toContain('selectPresetSlot.value = index;\n  saveActivePresetSlot(index);');
  });

  it('reloads the active bank after engine init (loadSysEx race fix)', () => {
    // Regression: on reloads the click-time bank load races initialize() —
    // loadSysEx silently drops while the workletNode is not built yet (cached
    // WASM/fetch resolves first), leaving the engine bankless. The active bank
    // must be (re)loaded right after init so onBankLoaded restores the slot.
    expect(appJs).toContain('// The bank load fired during the click (listener below) can race with');
    expect(appJs).toContain('now that the engine is ready — idempotent, and onBankLoaded then restores');
    expect(appJs).toContain('enableBankUI();');
  });

  it('restores the saved slot in onBankLoaded only when the SAME bank is active', () => {
    expect(appJs).toContain('const loadSavedPresetSlot = () => {');
    expect(appJs).toContain("saved.bankId === savedBank.id");
    // non-zero restored slot reloads the preset from the engine so sound matches name
    expect(appJs).toContain('if (slotIndex > 0 && audioEngine) {');
    expect(appJs).toContain('audioEngine.loadPreset(slotIndex, PARAMETER_REGISTRY.parameters.map(p => p.id));');
    // a different bank (or no saved slot) still starts at 0
    expect(appJs).toContain('? saved.slot : 0;');
  });
});
