import { describe, it, expect } from 'vitest';
import fs from 'fs';
import path from 'path';
import { parseSyxFile, humanizePatchNames } from '../src/contracts/syxNames.js';

const readSyx = (rel) => new Uint8Array(fs.readFileSync(path.resolve(__dirname, rel)));

describe('syxNames parseSyxFile (real WASM, no audio)', () => {
  it('parses the CZ-230S 00-15 bank (16 patches)', async () => {
    const { count, names } = await parseSyxFile(readSyx('../presets/cz230s/CZ230S-00-15.syx'));
    expect(count).toBe(16);
    expect(names).toHaveLength(16);
  });

  it('parses the CZ Pack 1 all bank (20 patches)', async () => {
    const { count, names } = await parseSyxFile(readSyx('../presets/czpack1/CZPack1-All.syx'));
    expect(count).toBe(20);
    expect(names).toHaveLength(20);
  });

  it('returns zero for garbage input', async () => {
    const { count, names } = await parseSyxFile(new Uint8Array([0x00, 0x01, 0x02]));
    expect(count).toBe(0);
    expect(names).toHaveLength(0);
  });

  it('rejects too-short input gracefully', async () => {
    const { count } = await parseSyxFile(new Uint8Array(10));
    expect(count).toBe(0);
  });
});

describe('syxNames humanizePatchNames', () => {
  it('turns bare Imported Preset into Patch NN', () => {
    const out = humanizePatchNames(['Imported Preset', 'Imported Preset', 'MY PATCH']);
    expect(out).toEqual(['Patch 01', 'Patch 02', 'MY PATCH']);
  });

  it('keeps real names untouched', () => {
    expect(humanizePatchNames(['INIT USER', 'FUNKY BASS'])).toEqual(['INIT USER', 'FUNKY BASS']);
  });

  it('honors a custom base label', () => {
    const out = humanizePatchNames(['Imported Preset'], 'Bass');
    expect(out).toEqual(['Bass 01']);
  });
});
