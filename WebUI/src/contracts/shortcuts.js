// Keyboard shortcuts — native parity + standard web/DAW conventions.
//
// Native parity (PluginEditor.cpp keyPressed):
//   Cmd/Ctrl+Z -> undo   |   Cmd/Ctrl+Y -> redo
// Web/DAW standards added on top (same actions the native menu calls):
//   Cmd/Ctrl+Shift+Z -> redo   (macOS / web convention)
//   Cmd/Ctrl+S        -> Save Patch As
//   Cmd/Ctrl+Shift+S  -> Save Bank
//   Cmd/Ctrl+O        -> Load Patch
//   Cmd/Ctrl+E        -> Export SysEx
//   Cmd/Ctrl+N        -> Reset Patch (New)
//   Cmd/Ctrl+,        -> Settings (System Mode)
//   F1                -> Manual / Wiki
//
// The map is pure and testable; app.js installs a single keydown listener
// that resolves the action via matchShortcut() and calls navbarActions.

// Each entry: { action, key, primary:bool, shift:bool, ctrl:bool, meta:bool }
// `primary` = the platform modifier (Ctrl on Win/Linux, Cmd on macOS).
export const SHORTCUTS = [
  { action: 'undo', key: 'z', primary: true },
  { action: 'redo', key: 'y', primary: true },          // native Windows convention
  { action: 'redo', key: 'z', primary: true, shift: true }, // web/macOS convention
  { action: 'savePatchAs', key: 's', primary: true },
  { action: 'saveBank', key: 's', primary: true, shift: true },
  { action: 'loadPatch', key: 'o', primary: true },
  { action: 'exportSysEx', key: 'e', primary: true },
  { action: 'resetPatch', key: 'n', primary: true },
  { action: 'settingsSystem', key: ',', primary: true },
  { action: 'manual', key: 'f1' }
];

// True when the keydown happened inside a text-editable element, so native
// text undo/redo (Ctrl+Z inside <input>/<textarea>) is never hijacked.
export function isEditableTarget(target) {
  if (!target || typeof target.tagName !== 'string') return false;
  const tag = target.tagName.toUpperCase();
  return tag === 'INPUT' || tag === 'TEXTAREA' || tag === 'SELECT'
    || target.isContentEditable === true;
}

// Resolve a KeyboardEvent to an action name, or null. `primary` matches
// Ctrl on Windows/Linux and Cmd on macOS (e.ctrlKey || e.metaKey), which is
// the standard web convention for a command modifier.
export function matchShortcut(e) {
  if (!e || typeof e.key !== 'string') return null;
  const key = e.key.toLowerCase();
  const primary = e.ctrlKey || e.metaKey;
  for (const sc of SHORTCUTS) {
    if (sc.key !== key) continue;
    if (!!sc.primary !== primary) continue;
    if (!!sc.shift !== !!e.shiftKey) continue;
    if (e.altKey) continue;
    return sc.action;
  }
  return null;
}
