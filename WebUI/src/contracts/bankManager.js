// Pure model for the Bank Manager context menu, parity with the native
// BankManagerOverlay (PopupMenu: Move Up / Move Down / Move to Position... /
// Rename... / Delete) and PresetManager (movePreset/deletePreset slot math).

export const BANK_MENU_ITEMS = Object.freeze([
  { id: 1, label: 'Move Up' },
  { id: 2, label: 'Move Down' },
  { id: 4, label: 'Move to Position...' },
  { id: 'sep1', separator: true },
  { id: 5, label: 'Rename...' },
  { id: 'sep2', separator: true },
  { id: 3, label: 'Delete' }
]);

// Menu entries enabled/disabled for a given row (parity: Move Up needs row > 0,
// Move Down needs row < last).
export function buildBankMenu(row, count) {
  return BANK_MENU_ITEMS.map(item => {
    if (item.separator) return { ...item };
    let disabled = false;
    if (item.id === 1) disabled = row <= 0;
    else if (item.id === 2) disabled = row >= count - 1;
    return { ...item, disabled };
  });
}

// Reorder presets, tracking the previously active index like
// PresetManager::movePreset. Returns { presets, activeIndex }.
export function movePreset(presets, fromIndex, toIndex, activeIndex = 0) {
  const arr = [...presets];
  const size = arr.length;
  if (fromIndex < 0 || fromIndex >= size || toIndex < 0 || toIndex >= size || fromIndex === toIndex) {
    return { presets: arr, activeIndex };
  }
  const p = arr.splice(fromIndex, 1)[0];
  arr.splice(toIndex, 0, p);

  let nextActive = activeIndex;
  if (activeIndex === fromIndex) nextActive = toIndex;
  else if (fromIndex < activeIndex && toIndex >= activeIndex) nextActive--;
  else if (fromIndex > activeIndex && toIndex <= activeIndex) nextActive++;

  return { presets: arr, activeIndex: nextActive };
}

// Remove a preset, keeping at least one entry (PresetManager::deletePreset).
// Returns { presets, activeIndex }.
export function deletePreset(presets, index, activeIndex = 0, emptyName = 'Init') {
  const arr = [...presets];
  if (index < 0 || index >= arr.length) return { presets: arr, activeIndex };
  arr.splice(index, 1);
  if (arr.length === 0) arr.push({ name: emptyName });
  let nextActive = activeIndex;
  if (nextActive >= arr.length) nextActive = arr.length - 1;
  return { presets: arr, activeIndex: nextActive };
}

// Rename a preset in place. Returns a new array.
export function renamePreset(presets, index, name) {
  const arr = [...presets];
  if (index < 0 || index >= arr.length) return arr;
  arr[index] = { ...arr[index], name };
  return arr;
}

// Clamp a "move to position" input (1-based) to the valid range, parity with
// the native dialog (newPos = entered - 1, must be within 0..size-1).
export function clampMoveToPosition(input, count) {
  const parsed = parseInt(input, 10);
  if (Number.isNaN(parsed)) return null;
  const zeroBased = parsed - 1;
  if (zeroBased < 0 || zeroBased >= count) return null;
  return zeroBased;
}
