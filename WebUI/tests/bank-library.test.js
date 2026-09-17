import { describe, it, expect } from 'vitest';
import {
  BANK_MODELS,
  BANK_SLOTS,
  STORAGE_KEY,
  createBank,
  findBank,
  getActiveBank,
  setActiveBankId,
  addBank,
  removeBank,
  renameBank,
  setBankPresets,
  setBankSnapshot,
  loadLibrary,
  saveLibrary,
  loadActiveBankId,
  saveActiveBankId,
  detectBankModel,
  describeModel,
  searchBank,
  occupiedCount,
  suggestBankName,
  emptyPresets
} from '../src/contracts/bankLibrary.js';

// Minimal in-memory storage mimicking localStorage
const makeStorage = () => {
  const map = new Map();
  return {
    getItem: (k) => (map.has(k) ? map.get(k) : null),
    setItem: (k, v) => { map.set(k, String(v)); },
    removeItem: (k) => { map.delete(k); }
  };
};

const lib = () => ({
  banks: [
    createBank({ name: 'Factory CZ-101', model: 'cz101', readOnly: true, presets: [{ name: 'A' }, { name: 'B' }] }),
    createBank({ name: 'User Bank 1', model: 'user', readOnly: false })
  ],
  activeBankId: null
});

describe('bankLibrary createBank', () => {
  it('creates a bank with an id, model and 64 slots', () => {
    const b = createBank({ name: 'My Bank' });
    expect(b.id).toBeTruthy();
    expect(b.name).toBe('My Bank');
    expect(b.model).toBe('user');
    expect(b.readOnly).toBe(false);
    expect(b.presets).toHaveLength(BANK_SLOTS);
    expect(b.presets[0].name).toBe('Init 1');
    expect(b.presets[63].name).toBe('Init 64');
  });

  it('accepts custom presets and readOnly flag', () => {
    const b = createBank({ name: 'X', model: 'cz5000', readOnly: true, presets: [{ name: 'Solo' }, { name: 'Pad' }] });
    expect(b.readOnly).toBe(true);
    expect(b.model).toBe('cz5000');
    expect(b.presets.map(p => p.name)).toEqual(['Solo', 'Pad']);
  });

  it('generates unique ids', () => {
    expect(createBank({}).id).not.toBe(createBank({}).id);
  });

  it('accepts a stable explicit id (factory banks)', () => {
    const b = createBank({ name: 'Factory CZ-101', model: 'cz101', readOnly: true, id: 'factory_cz101' });
    expect(b.id).toBe('factory_cz101');
    // Same id across calls -> stable across restarts
    const b2 = createBank({ name: 'Factory CZ-101', model: 'cz101', readOnly: true, id: 'factory_cz101' });
    expect(b2.id).toBe(b.id);
  });
});

describe('bankLibrary CRUD', () => {
  it('addBank appends without mutating', () => {
    const l = lib();
    const added = addBank(l, createBank({ name: 'New' }));
    expect(added.banks).toHaveLength(3);
    expect(l.banks).toHaveLength(2); // input untouched
  });

  it('removeBank removes and fixes the active id', () => {
    const l = lib();
    const target = l.banks[1];
    l.activeBankId = target.id;
    const r = removeBank(l, target.id);
    expect(r.banks).toHaveLength(1);
    expect(r.activeBankId).toBe(l.banks[0].id);
  });

  it('removeBank keeps active null when the library empties', () => {
    const l = lib();
    l.activeBankId = l.banks[0].id;
    let r = removeBank(l, l.banks[0].id);
    r = removeBank(r, r.banks[0].id);
    expect(r.banks).toHaveLength(0);
    expect(r.activeBankId).toBe(null);
  });

  it('renameBank renames in place without mutating', () => {
    const l = lib();
    const r = renameBank(l, l.banks[0].id, 'Renamed');
    expect(r.banks[0].name).toBe('Renamed');
    expect(l.banks[0].name).toBe('Factory CZ-101');
  });

  it('setBankPresets replaces names and pads to 64', () => {
    const l = lib();
    const r = setBankPresets(l, l.banks[0].id, ['X', 'Y']);
    expect(r.banks[0].presets[0].name).toBe('X');
    expect(r.banks[0].presets[1].name).toBe('Y');
    expect(r.banks[0].presets).toHaveLength(BANK_SLOTS);
    expect(r.banks[0].presets[2].name).toBe('Init 3');
  });

  it('setBankSnapshot stores the engine JSON', () => {
    const l = lib();
    const r = setBankSnapshot(l, l.banks[0].id, '{"presets":[]}');
    expect(r.banks[0].snapshot).toBe('{"presets":[]}');
  });
});

describe('bankLibrary active bank helpers', () => {
  it('getActiveBank falls back to the first bank', () => {
    const l = lib();
    l.activeBankId = null;
    expect(getActiveBank(l).name).toBe('Factory CZ-101');
  });

  it('setActiveBankId updates the active id', () => {
    const l = lib();
    const r = setActiveBankId(l, l.banks[1].id);
    expect(r.activeBankId).toBe(l.banks[1].id);
  });

  it('findBank returns undefined for missing ids', () => {
    const l = lib();
    expect(findBank(l, 'nope')).toBeUndefined();
    expect(findBank(l, l.banks[0].id).name).toBe('Factory CZ-101');
  });
});

describe('bankLibrary persistence', () => {
  it('round-trips a library through storage', () => {
    const storage = makeStorage();
    const l = lib();
    l.activeBankId = l.banks[1].id;
    expect(saveLibrary(l, storage)).toBe(true);
    expect(storage.getItem(STORAGE_KEY)).toBeTruthy();

    const loaded = loadLibrary(storage);
    expect(loaded.banks).toHaveLength(2);
    expect(loaded.banks[0].name).toBe('Factory CZ-101');
    expect(loaded.banks[0].model).toBe('cz101');
    expect(loaded.activeBankId).toBe(l.banks[1].id);
  });

  it('returns null when storage is empty or broken', () => {
    const storage = makeStorage();
    expect(loadLibrary(storage)).toBeNull();
    storage.setItem(STORAGE_KEY, '{broken');
    expect(loadLibrary(storage)).toBeNull();
  });

  it('persists the active bank id separately', () => {
    const storage = makeStorage();
    saveActiveBankId('bank_abc', storage);
    expect(loadActiveBankId(storage)).toBe('bank_abc');
  });
});

describe('bankLibrary model detection', () => {
  it('maps patch counts to the right model', () => {
    expect(detectBankModel(1)).toBe('user');          // single patch import
    expect(detectBankModel(16)).toBe('cz101');        // CZ-101 bank
    expect(detectBankModel(20)).toBe('cz5000');       // 16 < 20 <= 32 -> CZ-5000 bank
    expect(detectBankModel(32)).toBe('cz5000');       // CZ-5000 bank
    expect(detectBankModel(64)).toBe('user');         // unknown/user
  });

  it('respects explicit source hints', () => {
    expect(detectBankModel(16, 'cz230s')).toBe('cz230s');
    expect(detectBankModel(20, 'czpack1')).toBe('czpack1');
    expect(detectBankModel(16, 'factory')).toBe('cz101');
  });

  it('describeModel labels known and unknown models', () => {
    expect(describeModel('cz101')).toBe('CZ-101');
    expect(describeModel('cz5000')).toBe('CZ-5000');
    expect(describeModel('cz230s')).toBe('CZ-230S');
    expect(describeModel('czpack1')).toBe('CZ Pack 1');
    expect(describeModel('user')).toBe('User');
    expect(describeModel('whatever')).toBe('User');
  });
});

describe('bankLibrary search and stats', () => {
  it('searchBank returns all indexes for an empty term', () => {
    const b = createBank({ name: 'X', presets: [{ name: 'Bass 1' }, { name: 'Lead 2' }, { name: 'Pad' }] });
    expect(searchBank(b, '')).toEqual([0, 1, 2]);
  });

  it('searchBank filters case-insensitively', () => {
    const b = createBank({ name: 'X', presets: [{ name: 'Bass 1' }, { name: 'Lead 2' }, { name: 'MyBassPad' }] });
    expect(searchBank(b, 'bass')).toEqual([0, 2]);
    expect(searchBank(b, 'LEAD')).toEqual([1]);
  });

  it('occupiedCount counts non-Init slots', () => {
    const b = createBank({ name: 'X', presets: [{ name: 'Bass 1' }, { name: 'Lead 2' }] });
    expect(occupiedCount(b)).toBe(2);
    const fresh = createBank({ name: 'Y' });
    expect(occupiedCount(fresh)).toBe(0);
  });

  it('suggestBankName avoids collisions', () => {
    const l = lib();
    const name = suggestBankName(l, 'User Bank');
    expect(name).toBe('User Bank 3'); // 1 and 2 taken
    const l2 = lib();
    l2.banks[1].name = 'User Bank 3';
    expect(suggestBankName(l2, 'User Bank')).toBe('User Bank 4');
  });

  it('emptyPresets generates the requested number of slots', () => {
    const p = emptyPresets(4, 'Pad');
    expect(p).toHaveLength(4);
    expect(p[0].name).toBe('Pad 1');
  });
});
