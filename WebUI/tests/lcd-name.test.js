import { describe, it, expect } from 'vitest';
import {
  LCD_NAME_MAX_LEN,
  LCD_NAME_CHARS,
  seedLcdName,
  formatLcdName,
  cycleLcdChar,
  clampCursor,
  finalLcdName
} from '../src/contracts/lcdName.js';

// Pure-logic tests for the Write mode name phase (WRT -> slot -> name). The
// interactive sequence (keypad buttons driving the LCD) is verified by the
// browser harness; here we test the character editor itself.

describe('LCD name editor (Write mode name phase)', () => {
  it('seeds a name uppercased and truncated to the LCD width', () => {
    expect(seedLcdName('  my patch  ')).toBe('MY PATCH');
    expect(seedLcdName('a'.repeat(30))).toHaveLength(LCD_NAME_MAX_LEN);
    expect(seedLcdName('')).toBe('');
    expect(seedLcdName(null)).toBe('');
  });

  it('formats the name with a cursor marker around the active character', () => {
    expect(formatLcdName('HELLO', 0)).toBe('[H]ELLO     ');
    expect(formatLcdName('HELLO', 1)).toBe('H[E]LLO     ');
    expect(formatLcdName('HELLO', 4)).toBe('HELL[O]     ');
  });

  it('clamps the cursor into the valid range', () => {
    expect(clampCursor(-5)).toBe(0);
    expect(clampCursor(0)).toBe(0);
    expect(clampCursor(LCD_NAME_MAX_LEN - 1)).toBe(LCD_NAME_MAX_LEN - 1);
    expect(clampCursor(999)).toBe(LCD_NAME_MAX_LEN - 1);
  });

  it('cycles the character under the cursor forward and backward', () => {
    const name = 'HELLO';
    // 'H' is at index 8 in the alphabet (space, A..H) -> up => 'I'
    const up = cycleLcdChar(name, 0, true);
    expect(up[0]).toBe('I');
    // back down => 'H' again (the result is padded to the full LCD width)
    const back = cycleLcdChar(up, 0, false);
    expect(back.slice(0, 5)).toBe('HELLO');
    expect(back).toHaveLength(LCD_NAME_MAX_LEN);
  });

  it('wraps around the alphabet ends', () => {
    // Space is index 0: down from space wraps to the last char.
    const space = ' A';
    const wrapped = cycleLcdChar(space, 0, false);
    expect(wrapped[0]).toBe(LCD_NAME_CHARS[LCD_NAME_CHARS.length - 1]);
    // up from the last char wraps to space
    expect(cycleLcdChar(wrapped, 0, true)[0]).toBe(' ');
  });

  it('edits a blank position (name padded to full width)', () => {
    const name = 'HI';
    // cursor at the blank after 'HI' -> cycle up -> ' ' (index 0) -> 'A' (index 1)
    const edited = cycleLcdChar(name, 2, true);
    expect(edited.slice(0, 3)).toBe('HIA');
    // and down wraps back to the last alphabet char
    expect(cycleLcdChar(edited, 2, false).slice(0, 3)).toBe('HI ');
  });

  it('finalLcdName trims whitespace', () => {
    expect(finalLcdName('  MY PATCH  ')).toBe('MY PATCH');
    expect(finalLcdName('')).toBe('');
  });
});
