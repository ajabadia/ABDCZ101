// LCD name editor logic — shared by app.js (the Write mode name phase) and the
// test suite. The native CZ-101 lets you type a preset name character by
// character on the LCD; here the same is driven from the keypad:
//   - ▲/▼ cycle the character under the cursor through the LCD alphabet
//   - ◀/▶ move the cursor
// The alphabet mirrors what fits the hardware LCD (uppercase A-Z, digits,
// space and a few symbols).

export const LCD_NAME_MAX_LEN = 10;
export const LCD_NAME_CHARS = ' ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.';

/** Seed a name for editing: uppercase, truncated to the LCD width. */
export function seedLcdName(raw) {
  const text = (raw || '').trim() || '';
  return text.toUpperCase().slice(0, LCD_NAME_MAX_LEN);
}

/** Render with the cursor marker around the active character. */
export function formatLcdName(name, cursor) {
  const padded = (name || '').padEnd(LCD_NAME_MAX_LEN, ' ');
  const idx = Math.max(0, Math.min(LCD_NAME_MAX_LEN - 1, cursor));
  return `${padded.slice(0, idx)}[${padded[idx]}]${padded.slice(idx + 1)}`;
}

/** Cycle the character under the cursor by ±1 through the alphabet. */
export function cycleLcdChar(name, cursor, isUp) {
  const cur = Math.max(0, Math.min(LCD_NAME_MAX_LEN - 1, cursor));
  const currentChar = ((name || '')[cur] || ' ').toUpperCase();
  let idx = LCD_NAME_CHARS.indexOf(currentChar);
  if (idx === -1) idx = 0;
  idx = (idx + (isUp ? 1 : -1) + LCD_NAME_CHARS.length) % LCD_NAME_CHARS.length;
  const arr = (name || '').padEnd(LCD_NAME_MAX_LEN, ' ').split('');
  arr[cur] = LCD_NAME_CHARS[idx];
  return arr.join('').slice(0, LCD_NAME_MAX_LEN);
}

/** Clamp a cursor position into the valid range. */
export function clampCursor(cursor) {
  return Math.max(0, Math.min(LCD_NAME_MAX_LEN - 1, cursor));
}

/** Final trimmed name to store (empty falls back to the caller's default). */
export function finalLcdName(name) {
  return (name || '').trim();
}
