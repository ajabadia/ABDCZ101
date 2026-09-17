import { describe, it, expect } from 'vitest';
import {
  CZ230S_BANK_NAMES,
  CZ_PACK1_NAMES,
  factoryBankNames
} from '../src/contracts/factoryBankNames.js';

describe('factoryBankNames — CZ-230S official factory index', () => {
  it('covers all 7 bundled banks with real names', () => {
    const files = Object.keys(CZ230S_BANK_NAMES);
    expect(files).toHaveLength(7);
    expect(files).toContain('CZ230S-00-15.syx');
    expect(files).toContain('CZ230S-96-99.syx');
  });

  it('has 16 patches per bank and 4 in the last bank (100 total)', () => {
    const total = Object.values(CZ230S_BANK_NAMES).reduce((sum, arr) => sum + arr.length, 0);
    expect(total).toBe(100);
    Object.entries(CZ230S_BANK_NAMES).forEach(([file, names]) => {
      if (file === 'CZ230S-96-99.syx') expect(names).toHaveLength(4);
      else expect(names).toHaveLength(16);
    });
  });

  it('starts with the factory brass ensemble (bank 00-15)', () => {
    expect(CZ230S_BANK_NAMES['CZ230S-00-15.syx'][0]).toBe('Brass Ens. 1');
    expect(CZ230S_BANK_NAMES['CZ230S-00-15.syx'][3]).toBe('Symphonic Ens. 1');
    expect(CZ230S_BANK_NAMES['CZ230S-00-15.syx'][15]).toBe('Slap Horn');
  });

  it('ends with the user-replaceable slots (96-99)', () => {
    expect(CZ230S_BANK_NAMES['CZ230S-96-99.syx']).toEqual([
      'Computer Game', 'Laser Gun', 'Miracle', 'Sweep'
    ]);
  });

  it('maps a bundled source path to its real names', () => {
    expect(factoryBankNames('presets/cz230s/CZ230S-16-31.syx')?.[0]).toBe('Sweet Strings');
    expect(factoryBankNames('presets/cz230s/CZ230S-80-95.syx')?.[14]).toBe('Cavernous Sound');
  });
});

describe('factoryBankNames — CZ Pack 1', () => {
  it('has 20 patches in the CZPack1-All.syx order', () => {
    expect(CZ_PACK1_NAMES).toHaveLength(20);
    expect(CZ_PACK1_NAMES[0]).toBe('Afterxylo');
    expect(CZ_PACK1_NAMES[19]).toBe('Yay Afterhit');
  });

  it('maps the All bank source to the 20 real names', () => {
    expect(factoryBankNames('presets/czpack1/CZPack1-All.syx')).toEqual(CZ_PACK1_NAMES);
  });

  it('no name table for unknown sources / factory preset bank', () => {
    expect(factoryBankNames('factory')).toBeNull();
    expect(factoryBankNames('presets/unknown.syx')).toBeNull();
    expect(factoryBankNames(null)).toBeNull();
  });
});
