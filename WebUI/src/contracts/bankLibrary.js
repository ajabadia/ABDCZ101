// Pure model for the enhanced Bank Manager library (inspired by the ABDEEP
// dual-container Bank & Preset Manager). Unlike the native CZ BankManagerOverlay
// (a single 64-slot list), the web library keeps MULTIPLE named banks, each
// tagged with the synth model its SysEx data belongs to — fixing the "sysex de
// un modelo u otro" confusion where CZ-101 (16 patches), CZ-5000 (32), CZ-230S
// banks and CZ Pack 1 patches all landed in the same bank without a label.
//
// Each bank is { id, name, model, readOnly, presets } where presets is an array
// of { name } (64 slots, the engine is the source of truth for the sound data;
// the library tracks organization + names). User banks additionally persist the
// engine JSON snapshot so they can be restored without the original .syx.

export const BANK_MODELS = Object.freeze({
  cz101: 'CZ-101',
  cz5000: 'CZ-5000',
  cz230s: 'CZ-230S',
  czpack1: 'CZ Pack 1',
  modern: 'Modern',
  user: 'User'
});

export const BANK_SLOTS = 64;
export const STORAGE_KEY = 'cz101.bankLibrary';
export const ACTIVE_KEY = 'cz101.activeBankId';

let nextId = 1;
const genId = () => `bank_${Date.now().toString(36)}_${nextId++}`;

// Empty 64-slot preset list (names only; the engine fills sound data on load).
export function emptyPresets(slots = BANK_SLOTS, prefix = 'Init') {
  const arr = [];
  for (let i = 0; i < slots; i++) {
    arr.push({ name: `${prefix} ${i + 1}` });
  }
  return arr;
}

export function createBank({ name, model = 'user', readOnly = false, presets, source = null, id = null }) {
  return {
    id: id || genId(),
    name: name || 'User Bank',
    model,
    readOnly,
    presets: presets && Array.isArray(presets)
      ? presets.map(p => (p && typeof p === 'object' ? { ...p } : { name: String(p) }))
      : emptyPresets(),
    // Optional engine snapshot (user banks) so a bank can be restored offline.
    snapshot: null,
    // Optional bundled .syx path for read-only factory banks (used to warm the
    // library with real patch counts and to load the bank into the engine).
    source
  };
}

// Normalize a bank id into the collection; returns undefined when missing.
export function findBank(library, id) {
  return (library.banks || []).find(b => b.id === id);
}

export function getActiveBank(library) {
  if (!library) return undefined;
  return findBank(library, library.activeBankId) || (library.banks || [])[0];
}

export function setActiveBankId(library, id) {
  return { ...library, activeBankId: id };
}

// Pure CRUD (no mutation).
export function addBank(library, bank) {
  return { ...library, banks: [...(library.banks || []), bank] };
}

export function removeBank(library, id) {
  const banks = (library.banks || []).filter(b => b.id !== id);
  const activeBankId = library.activeBankId === id
    ? (banks[0] ? banks[0].id : null)
    : library.activeBankId;
  return { ...library, banks, activeBankId };
}

export function renameBank(library, id, newName) {
  return {
    ...library,
    banks: (library.banks || []).map(b => b.id === id ? { ...b, name: newName } : b)
  };
}

export function setBankSnapshot(library, id, snapshot) {
  return {
    ...library,
    banks: (library.banks || []).map(b => b.id === id ? { ...b, snapshot } : b)
  };
}

export function setBankPresets(library, id, names) {
  const presets = Array.isArray(names)
    ? names.map(n => ({ name: n }))
    : [];
  while (presets.length < BANK_SLOTS) presets.push({ name: `Init ${presets.length + 1}` });
  return {
    ...library,
    banks: (library.banks || []).map(b => b.id === id ? { ...b, presets: presets.slice(0, BANK_SLOTS) } : b)
  };
}

// ── Persistence (localStorage) ──
export function loadLibrary(storage = typeof localStorage !== 'undefined' ? localStorage : null) {
  if (!storage) return null;
  try {
    const raw = storage.getItem(STORAGE_KEY);
    if (!raw) return null;
    const lib = JSON.parse(raw);
    if (!lib || !Array.isArray(lib.banks)) return null;
    // Restore the id counter so generated ids never collide.
    const maxNum = lib.banks.reduce((m, b) => {
      const n = parseInt((b.id || '').split('_').pop(), 36);
      return Number.isFinite(n) ? Math.max(m, n) : m;
    }, 0);
    nextId = Math.max(nextId, maxNum + 1);
    return lib;
  } catch (err) {
    return null;
  }
}

export function saveLibrary(library, storage = typeof localStorage !== 'undefined' ? localStorage : null) {
  if (!storage) return false;
  try {
    storage.setItem(STORAGE_KEY, JSON.stringify(library));
    return true;
  } catch (err) {
    return false;
  }
}

export function loadActiveBankId(storage = typeof localStorage !== 'undefined' ? localStorage : null) {
  if (!storage) return null;
  try { return storage.getItem(ACTIVE_KEY); } catch (err) { return null; }
}

export function saveActiveBankId(id, storage = typeof localStorage !== 'undefined' ? localStorage : null) {
  if (!storage) return;
  try { storage.setItem(ACTIVE_KEY, id); } catch (err) { /* ignore */ }
}

// ── Model detection ──
// CZ-101 factory banks are 16 patches; CZ-5000 banks are 32; a single SysEx is
// one patch. Used when importing a .syx so the created bank is tagged with the
// model its data actually belongs to.
export function detectBankModel(loadedCount, sourceHint) {
  if (sourceHint === 'cz230s') return 'cz230s';
  if (sourceHint === 'czpack1') return 'czpack1';
  if (sourceHint === 'factory') return 'cz101';
  if (loadedCount === 1) return 'user'; // single patch import -> user bank
  if (loadedCount <= 16) return 'cz101';
  if (loadedCount <= 32) return 'cz5000';
  return 'user';
}

export function describeModel(model) {
  return BANK_MODELS[model] || BANK_MODELS.user;
}

// ── Search ──
// Returns the preset indexes whose name contains the term (case-insensitive).
// Empty term returns all indexes.
export function searchBank(bank, term) {
  if (!bank || !bank.presets) return [];
  const t = (term || '').trim().toLowerCase();
  if (!t) return bank.presets.map((_, i) => i);
  const out = [];
  bank.presets.forEach((p, i) => {
    if ((p.name || '').toLowerCase().includes(t)) out.push(i);
  });
  return out;
}

// Count of non-init slots (a slot counts as "occupied" when its name is not the
// generated Init prefix).
export function occupiedCount(bank, initPrefix = 'Init') {
  if (!bank || !bank.presets) return 0;
  return bank.presets.filter(p => !(p.name || '').startsWith(initPrefix)).length;
}

// Unique name for a new user bank ("User Bank N", skipping collisions).
export function suggestBankName(library, base = 'User Bank') {
  const names = new Set((library.banks || []).map(b => b.name));
  let n = names.size + 1;
  while (names.has(`${base} ${n}`)) n++;
  return `${base} ${n}`;
}
