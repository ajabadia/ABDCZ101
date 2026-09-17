// Navbar menu model — parity with the native PluginEditor MenuBar
// (Source/PluginEditor.cpp: getMenuBarNames / getMenuForIndex). The native
// bar exposes File / Edit / Mode / View / Help; this module holds the same
// item tree so the WebUI dropdowns match the C++ menus and the test suite can
// assert coverage (no item accidentally dropped during the port).
//
// Each item: { id, label, action?, separator?, checked?, disabled?, submenu? }
//   - action: key handled by app.js (calls the same engine functions the
//     native menuItemSelected calls, when technically possible on the web).
//   - checked: function/flag evaluated at render time (radio/check marks).
//   - submenu: nested items (native "MIDI Channel", "Oversampling").
//   - shortcut: optional keyboard hint rendered as <kbd> (kept in sync with
//     ./shortcuts.js; the native keyPressed only defines Undo/Redo, the rest
//     follow common web/DAW conventions).

// Native checkmark parity (PluginEditor.cpp getMenuForIndex):
//   midiMenu.addItem(210+i, String(i), true, i == currentCh);
//   oversamplingMenu.addItem(410, "1x (Eco)", true, currentQ == 0);
// The active item of a menu (or submenu) gets the checkmark. `currentValue`
// is a function action -> current value; radio actions (operationMode,
// oversampling, midiChannel, theme, zoom) tick the item whose `value` matches.

export function isItemChecked(item, currentValue) {
  if (!item) return false;
  if (item.checked) return true;
  if (item.action && currentValue) {
    return currentValue(item.action) === item.value;
  }
  return false;
}

// Renders one item (or submenu) to the same HTML app.js uses. Pure and
// testable: the checkmark of every item — including nested submenu items —
// comes from isItemChecked with the same currentValue resolver.
export function renderItemHtml(item, currentValue) {
  if (!item) return '';
  if (item.separator) return '<div class="nav-separator"></div>';
  const check = isItemChecked(item, currentValue) ? '✓ ' : '';
  if (item.submenu) {
    return `<div class="nav-item nav-has-sub">${item.label}<span class="nav-arrow">▸</span>` +
      `<div class="nav-submenu">${item.submenu.map(sub => renderItemHtml(sub, currentValue)).join('')}</div></div>`;
  }
  const kbd = item.shortcut ? `<kbd class="nav-kbd">${item.shortcut}</kbd>` : '';
  return `<button type="button" class="nav-item" data-action="${item.action || ''}" data-value="${item.value ?? ''}">${check}${item.label}${kbd}</button>`;
}

export const NAVBAR_MENUS = [
  {
    name: 'File',
    items: [
      { id: 100, label: 'Load Bank (.json)...', action: 'loadBank' },
      { id: 101, label: 'Save Bank (.json)...', action: 'saveBank', shortcut: 'Ctrl+Shift+S' },
      { separator: true },
      { id: 104, label: 'Load Patch (.json, .syx)...', action: 'loadPatch', shortcut: 'Ctrl+O' },
      { id: 105, label: 'Save Patch As (.json)...', action: 'savePatchAs', shortcut: 'Ctrl+S' },
      { id: 106, label: 'Export Original SysEx (.syx)...', action: 'exportSysEx', shortcut: 'Ctrl+E' },
      { separator: true },
      { id: 107, label: 'Store Patch (Override Slot)', action: 'storePatch' },
      { id: 108, label: 'Store to New Slot...', action: 'storeNewSlot' },
      { id: 109, label: 'Rename Current Patch', action: 'renamePatch' },
      { separator: true },
      { id: 102, label: 'Init Bank (Factory Reset)', action: 'initBank' },
      { id: 103, label: 'Reset Current Patch', action: 'resetPatch', shortcut: 'Ctrl+N' }
    ]
  },
  {
    name: 'Edit',
    items: [
      { id: 200, label: 'Undo', action: 'undo', shortcut: 'Ctrl+Z' },
      { id: 201, label: 'Redo', action: 'redo', shortcut: 'Ctrl+Y' },
      { separator: true },
      { id: 203, label: 'Bank Manager...', action: 'bankManager' },
      { id: 204, label: 'Randomize Patch', action: 'randomize' },
      { separator: true },
      { id: 205, label: 'Settings (System Mode)', action: 'settingsSystem', shortcut: 'Ctrl+,' },
      { separator: true },
      { id: 206, label: 'Audio Settings...', action: 'audioSettings' },
      { separator: true },
      {
        id: 210,
        label: 'MIDI Channel',
        submenu: Array.from({ length: 16 }, (_, i) => ({
          id: 210 + i + 1,
          label: String(i + 1),
          action: 'midiChannel',
          value: i + 1
        }))
      }
    ]
  },
  {
    name: 'Mode',
    items: [
      { id: 400, label: 'Classic CZ-101 (Hardware Strict)', action: 'operationMode', value: 0 },
      { id: 403, label: 'Classic CZ-1 (Velocity & Chorus)', action: 'operationMode', value: 1 },
      { id: 402, label: 'Classic CZ-5000 (Extended Polyphony)', action: 'operationMode', value: 2 },
      { id: 401, label: 'Modern (Enhanced Features)', action: 'operationMode', value: 3 },
      { separator: true },
      {
        id: 410,
        label: 'Oversampling',
        submenu: [
          { id: 410, label: '1x (Eco)', action: 'oversampling', value: 0 },
          { id: 411, label: '2x (High)', action: 'oversampling', value: 1 },
          { id: 412, label: '4x (Ultra)', action: 'oversampling', value: 2 }
        ]
      }
    ]
  },
  {
    name: 'View',
    items: [
      { id: 300, label: 'Zoom 100%', action: 'zoom', value: 1.0 },
      { id: 301, label: 'Zoom 125%', action: 'zoom', value: 1.25 },
      { id: 302, label: 'Zoom 150%', action: 'zoom', value: 1.5 },
      { separator: true },
      { id: 310, label: 'Dark Theme', action: 'theme', value: 'dark' },
      { id: 312, label: 'Vintage Theme (ABD Z5001)', action: 'theme', value: 'vintage' },
      { id: 314, label: 'CyberGlow Theme', action: 'theme', value: 'cyberglow' },
      { id: 315, label: 'Neon Retro Theme', action: 'theme', value: 'neonretro' },
      { id: 316, label: 'Steampunk Theme', action: 'theme', value: 'steampunk' },
      { id: 318, label: 'Retro Terminal Theme', action: 'theme', value: 'retroterminal' }
    ]
  },
  {
    name: 'Help',
    items: [
      { id: 900, label: 'Manual / Wiki', action: 'manual', shortcut: 'F1' },
      { id: 901, label: 'About...', action: 'about' }
    ]
  }
];
