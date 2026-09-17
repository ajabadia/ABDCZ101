import { describe, it, expect } from 'vitest';
import {
  BANK_MENU_ITEMS,
  buildBankMenu,
  movePreset,
  deletePreset,
  renamePreset,
  clampMoveToPosition
} from '../src/contracts/bankManager.js';

const bank = () => [
  { name: 'A' }, { name: 'B' }, { name: 'C' }, { name: 'D' }
];

describe('bankManager context menu (native BankManagerOverlay parity)', () => {
  it('menu has the native ids and separators in order', () => {
    const ids = BANK_MENU_ITEMS.map(i => i.id);
    // 1 Move Up, 2 Move Down, 4 Move to Position, sep, 5 Rename, sep, 3 Delete
    expect(ids).toEqual([1, 2, 4, 'sep1', 5, 'sep2', 3]);
    expect(BANK_MENU_ITEMS.filter(i => i.separator)).toHaveLength(2);
  });

  it('disables Move Up on row 0 and Move Down on the last row', () => {
    const menu0 = buildBankMenu(0, 4);
    expect(menu0.find(i => i.id === 1).disabled).toBe(true);
    expect(menu0.find(i => i.id === 2).disabled).toBe(false);

    const menuLast = buildBankMenu(3, 4);
    expect(menuLast.find(i => i.id === 1).disabled).toBe(false);
    expect(menuLast.find(i => i.id === 2).disabled).toBe(true);
  });

  it('enables everything in the middle', () => {
    const menu = buildBankMenu(2, 4);
    expect(menu.find(i => i.id === 1).disabled).toBe(false);
    expect(menu.find(i => i.id === 2).disabled).toBe(false);
    expect(menu.find(i => i.id === 5).disabled).toBe(false);
    expect(menu.find(i => i.id === 3).disabled).toBe(false);
  });
});

describe('bankManager slot math (native PresetManager parity)', () => {
  it('movePreset moves the slot and tracks the active index', () => {
    // Move 0 -> 2, active was 0 (the moved one) -> follows to 2.
    const r1 = movePreset(bank(), 0, 2, 0);
    expect(r1.presets.map(p => p.name)).toEqual(['B', 'C', 'A', 'D']);
    expect(r1.activeIndex).toBe(2);

    // Move 2 -> 0, active was 1 (B): removing index 2 leaves [A,B,D], then
    // inserting C at 0 gives [C,A,B,D] -> B (active) ends at index 2.
    const r2 = movePreset(bank(), 2, 0, 1);
    expect(r2.presets.map(p => p.name)).toEqual(['C', 'A', 'B', 'D']);
    expect(r2.activeIndex).toBe(2);
  });

  it('movePreset clamps out-of-range and same-index moves', () => {
    const b = bank();
    expect(movePreset(b, 0, 0, 0).presets.map(p => p.name)).toEqual(['A', 'B', 'C', 'D']);
    expect(movePreset(b, -1, 2, 0).presets.map(p => p.name)).toEqual(['A', 'B', 'C', 'D']);
    expect(movePreset(b, 0, 9, 0).presets.map(p => p.name)).toEqual(['A', 'B', 'C', 'D']);
  });

  it('deletePreset removes the slot and adjusts the active index', () => {
    const r = deletePreset(bank(), 1, 1);
    expect(r.presets.map(p => p.name)).toEqual(['A', 'C', 'D']);
    expect(r.activeIndex).toBe(1); // was 1, now points at C
  });

  it('deletePreset keeps at least one preset when the bank empties', () => {
    const one = [{ name: 'X' }];
    const r = deletePreset(one, 0, 0);
    expect(r.presets).toHaveLength(1);
    expect(r.presets[0].name).toBe('Init');
    expect(r.activeIndex).toBe(0);
  });

  it('deletePreset clamps active index when the last slot is removed', () => {
    const r = deletePreset(bank(), 3, 3);
    expect(r.presets.map(p => p.name)).toEqual(['A', 'B', 'C']);
    expect(r.activeIndex).toBe(2);
  });

  it('renamePreset renames in place without mutating the input', () => {
    const b = bank();
    const r = renamePreset(b, 2, 'New C');
    expect(r[2].name).toBe('New C');
    expect(b[2].name).toBe('C'); // input untouched
    expect(renamePreset(b, 9, 'X')).toEqual(b);
  });

  it('clampMoveToPosition converts 1-based input and rejects out of range', () => {
    expect(clampMoveToPosition('1', 4)).toBe(0);
    expect(clampMoveToPosition('4', 4)).toBe(3);
    expect(clampMoveToPosition('5', 4)).toBe(null);
    expect(clampMoveToPosition('0', 4)).toBe(null);
    expect(clampMoveToPosition('abc', 4)).toBe(null);
    expect(clampMoveToPosition('', 4)).toBe(null);
  });
});
