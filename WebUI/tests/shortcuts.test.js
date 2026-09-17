import { describe, it, expect } from 'vitest';
import { SHORTCUTS, matchShortcut, isEditableTarget } from '../src/contracts/shortcuts.js';
import { NAVBAR_MENUS, renderItemHtml } from '../src/contracts/navbarModel.js';

// Native parity (PluginEditor.cpp keyPressed): Cmd/Ctrl+Z undo, Cmd/Ctrl+Y
// redo. Plus standard web/DAW conventions mapped to the same navbarActions.

const ev = (opts = {}) => ({
  key: opts.key || '',
  ctrlKey: !!opts.ctrl,
  metaKey: !!opts.meta,
  shiftKey: !!opts.shift,
  altKey: !!opts.alt
});

describe('matchShortcut (native keyPressed parity + web standards)', () => {
  it('Ctrl+Z -> undo, Ctrl+Y -> redo (native parity)', () => {
    expect(matchShortcut(ev({ key: 'z', ctrl: true }))).toBe('undo');
    expect(matchShortcut(ev({ key: 'y', ctrl: true }))).toBe('redo');
  });

  it('Cmd (macOS) works like Ctrl', () => {
    expect(matchShortcut(ev({ key: 'z', meta: true }))).toBe('undo');
    expect(matchShortcut(ev({ key: 's', meta: true }))).toBe('savePatchAs');
  });

  it('Ctrl+Shift+Z -> redo (web/macOS convention)', () => {
    expect(matchShortcut(ev({ key: 'z', ctrl: true, shift: true }))).toBe('redo');
    expect(matchShortcut(ev({ key: 'z', ctrl: true }))).toBe('undo');
  });

  it('standard file actions', () => {
    expect(matchShortcut(ev({ key: 's', ctrl: true }))).toBe('savePatchAs');
    expect(matchShortcut(ev({ key: 's', ctrl: true, shift: true }))).toBe('saveBank');
    expect(matchShortcut(ev({ key: 'o', ctrl: true }))).toBe('loadPatch');
    expect(matchShortcut(ev({ key: 'e', ctrl: true }))).toBe('exportSysEx');
    expect(matchShortcut(ev({ key: 'n', ctrl: true }))).toBe('resetPatch');
    expect(matchShortcut(ev({ key: ',', ctrl: true }))).toBe('settingsSystem');
    expect(matchShortcut(ev({ key: 'f1' }))).toBe('manual');
  });

  it('rejects unbound keys and modifier mismatches', () => {
    expect(matchShortcut(ev({ key: 'x', ctrl: true }))).toBeNull();
    expect(matchShortcut(ev({ key: 'z' }))).toBeNull();            // no modifier
    expect(matchShortcut(ev({ key: 'z', ctrl: true, alt: true }))).toBeNull();
    expect(matchShortcut(ev({ key: 's', shift: true }))).toBeNull();
    expect(matchShortcut(ev({ key: 'S', ctrl: true }))).toBe('savePatchAs'); // case-insensitive
    expect(matchShortcut(ev({}))).toBeNull();
    expect(matchShortcut(null)).toBeNull();
  });

  it('F1 has no modifier requirement but rejects modified F1', () => {
    expect(matchShortcut(ev({ key: 'f1' }))).toBe('manual');
    expect(matchShortcut(ev({ key: 'f1', ctrl: true }))).toBeNull();
    expect(matchShortcut(ev({ key: 'f1', shift: true }))).toBeNull();
  });
});

describe('isEditableTarget (never hijack text inputs)', () => {
  it('detects input/textarea/select/contenteditable', () => {
    expect(isEditableTarget({ tagName: 'INPUT' })).toBe(true);
    expect(isEditableTarget({ tagName: 'TEXTAREA' })).toBe(true);
    expect(isEditableTarget({ tagName: 'SELECT' })).toBe(true);
    expect(isEditableTarget({ tagName: 'DIV', isContentEditable: true })).toBe(true);
    expect(isEditableTarget({ tagName: 'BUTTON' })).toBe(false);
    expect(isEditableTarget(null)).toBe(false);
    expect(isEditableTarget({})).toBe(false);
  });
});

describe('shortcut hints in the navbar model (kept in sync)', () => {
  const flatItems = [];
  NAVBAR_MENUS.forEach(m => m.items.forEach(i => {
    if (i.separator) return;
    if (i.submenu) i.submenu.forEach(s => flatItems.push(s));
    else flatItems.push(i);
  }));

  const findByAction = (action) => flatItems.find(i => i.action === action);

  it('every shortcut in shortcuts.js is attached to its menu item', () => {
    for (const sc of SHORTCUTS) {
      const item = findByAction(sc.action);
      expect(item, `menu item for action "${sc.action}"`).toBeTruthy();
      expect(item.shortcut, `shortcut hint for "${sc.action}"`).toBeTruthy();
    }
  });

  it('renders the kbd hint inside the menu item', () => {
    const undo = findByAction('undo');
    const html = renderItemHtml(undo, null);
    expect(html).toContain('>Undo<kbd class="nav-kbd">Ctrl+Z</kbd></button>');
    expect(html).toContain('Undo');
    expect(html).toContain('Ctrl+Z');
  });

  it('items without a shortcut render no kbd', () => {
    const storeNew = findByAction('storeNewSlot');
    expect(storeNew.shortcut).toBeFalsy();
    expect(renderItemHtml(storeNew, null)).not.toContain('nav-kbd');
  });
});
