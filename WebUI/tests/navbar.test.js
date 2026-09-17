import { describe, it, expect } from 'vitest';
import fs from 'fs';
import path from 'path';
import { NAVBAR_MENUS, renderItemHtml, isItemChecked } from '../src/contracts/navbarModel.js';
import { generateRandomPatch } from '../src/contracts/patchRandomizer.js';

// Parity with the native PluginEditor MenuBar: exactly 5 menus in the same
// order as getMenuBarNames() ({ "File", "Edit", "Mode", "View", "Help" }), and
// the items mirror getMenuForIndex() (ids preserved from menuItemSelected).

describe('Navbar menu model (native MenuBar parity)', () => {
  it('exposes the 5 native menus in order', () => {
    expect(NAVBAR_MENUS.map(m => m.name)).toEqual(['File', 'Edit', 'Mode', 'View', 'Help']);
  });

  it('File menu matches the native items (ids 100-109)', () => {
    const ids = NAVBAR_MENUS[0].items.filter(i => !i.separator).map(i => i.id);
    expect(ids).toEqual([100, 101, 104, 105, 106, 107, 108, 109, 102, 103]);
    const labels = NAVBAR_MENUS[0].items.filter(i => !i.separator).map(i => i.label);
    expect(labels).toEqual([
      'Load Bank (.json)...',
      'Save Bank (.json)...',
      'Load Patch (.json, .syx)...',
      'Save Patch As (.json)...',
      'Export Original SysEx (.syx)...',
      'Store Patch (Override Slot)',
      'Store to New Slot...',
      'Rename Current Patch',
      'Init Bank (Factory Reset)',
      'Reset Current Patch'
    ]);
  });

  it('every File/Edit item has an action handler key', () => {
    ['File', 'Edit'].forEach(menuName => {
      const menu = NAVBAR_MENUS.find(m => m.name === menuName);
      menu.items.filter(i => !i.separator && !i.submenu).forEach(item => {
        expect(item.action, `${item.label} missing action`).toBeTruthy();
      });
    });
  });

  it('Edit menu has Undo/Redo/Bank Manager/Randomize/Settings + MIDI Channel submenu', () => {
    const edit = NAVBAR_MENUS[1];
    const actions = edit.items.filter(i => !i.separator).map(i => i.action || i.label);
    expect(actions).toContain('undo');
    expect(actions).toContain('redo');
    expect(actions).toContain('bankManager');
    expect(actions).toContain('randomize');
    expect(actions).toContain('settingsSystem');
    expect(actions).toContain('audioSettings'); // native id 206 Audio Settings...

    const midiSub = edit.items.find(i => i.submenu && i.label === 'MIDI Channel');
    expect(midiSub.submenu).toHaveLength(16);
    expect(midiSub.submenu[0]).toMatchObject({ action: 'midiChannel', value: 1 });
    expect(midiSub.submenu[15]).toMatchObject({ action: 'midiChannel', value: 16 });
  });

  it('Audio Settings item sits between Settings and MIDI Channel (native order)', () => {
    const edit = NAVBAR_MENUS[1];
    const flat = edit.items.filter(i => !i.separator);
    const settingsIdx = flat.findIndex(i => i.action === 'settingsSystem');
    const audioIdx = flat.findIndex(i => i.action === 'audioSettings');
    const midiIdx = flat.findIndex(i => i.submenu && i.label === 'MIDI Channel');
    expect(audioIdx).toBeGreaterThan(settingsIdx);
    expect(midiIdx).toBeGreaterThan(audioIdx);
    expect(flat[audioIdx].id).toBe(206); // native PluginEditor.cpp case 206
  });

  it('Mode menu has the 3 operation modes + oversampling submenu', () => {
    const mode = NAVBAR_MENUS[2];
    const opModes = mode.items.filter(i => i.action === 'operationMode');
    expect(opModes.map(i => i.value)).toEqual([0, 1, 2, 3]); // Classic 101, 5000, CZ-1, Modern

    const oversampling = mode.items.find(i => i.submenu);
    expect(oversampling.submenu.map(i => i.value)).toEqual([0, 1, 2]); // 1x, 2x, 4x
  });

  it('View menu has zoom + all 9 themes (native ids 300-318)', () => {
    const view = NAVBAR_MENUS[3];
    const zooms = view.items.filter(i => i.action === 'zoom').map(i => i.value);
    expect(zooms).toEqual([1.0, 1.25, 1.5]);

    const themes = view.items.filter(i => i.action === 'theme').map(i => i.value);
    expect(themes).toEqual([
      'dark', 'vintage', 'cyberglow',
      'neonretro', 'steampunk', 'retroterminal'
    ]);
  });

  it('Help menu has Manual + About (native 900/901)', () => {
    const help = NAVBAR_MENUS[4];
    expect(help.items.map(i => i.id)).toEqual([900, 901]);
    expect(help.items.map(i => i.action)).toEqual(['manual', 'about']);
  });
});

describe('Navbar checkmarks (native isTicked parity)', () => {
  // Mirrors app.js getCurrent: returns the active value for radio actions.
  const currentValue = (state) => (action) => {
    if (action === 'operationMode') return state.operationMode;
    if (action === 'oversampling') return state.oversampling;
    if (action === 'midiChannel') return state.midiChannel;
    if (action === 'theme') return state.theme;
    if (action === 'zoom') return state.zoom;
    return undefined;
  };

  it('MIDI Channel submenu ticks exactly the active channel', () => {
    const midiSub = NAVBAR_MENUS[1].items.find(i => i.submenu && i.label === 'MIDI Channel');
    const state = { operationMode: 0, oversampling: 0, midiChannel: 9, theme: 'dark', zoom: 1.0 };
    const html = midiSub.submenu.map(sub => renderItemHtml(sub, currentValue(state))).join('');
    expect(html.match(/✓/g) || []).toHaveLength(1);
    expect(html).toContain('✓ 9');
    expect(html).not.toContain('✓ 1');
    expect(html).not.toContain('✓ 8');
    expect(html).not.toContain('✓ 10');
  });

  it('MIDI Channel checkmark follows the active channel when it changes', () => {
    const midiSub = NAVBAR_MENUS[1].items.find(i => i.submenu && i.label === 'MIDI Channel');
    let channel = 3;
    let html = midiSub.submenu.map(sub => renderItemHtml(sub, currentValue({ operationMode: 0, oversampling: 0, midiChannel: channel, theme: 'dark', zoom: 1 }))).join('');
    expect(html).toContain('✓ 3');
    channel = 16; // wrap to the last channel
    html = midiSub.submenu.map(sub => renderItemHtml(sub, currentValue({ operationMode: 0, oversampling: 0, midiChannel: channel, theme: 'dark', zoom: 1 }))).join('');
    expect(html).toContain('✓ 16');
    expect(html).not.toContain('✓ 3');
  });

  it('Oversampling submenu ticks exactly the active quality', () => {
    const overSub = NAVBAR_MENUS[2].items.find(i => i.submenu && i.label === 'Oversampling');
    const state = { operationMode: 1, oversampling: 2, midiChannel: 1, theme: 'dark', zoom: 1 };
    const html = overSub.submenu.map(sub => renderItemHtml(sub, currentValue(state))).join('');
    expect(html.match(/✓/g) || []).toHaveLength(1);
    expect(html).toContain('✓ 4x (Ultra)');
    expect(html).not.toContain('✓ 1x');
    expect(html).not.toContain('✓ 2x');
  });

  it('Oversampling checkmark follows the active quality when it changes', () => {
    const overSub = NAVBAR_MENUS[2].items.find(i => i.submenu && i.label === 'Oversampling');
    const render = (q) => overSub.submenu.map(sub => renderItemHtml(sub, currentValue({ operationMode: 0, oversampling: q, midiChannel: 1, theme: 'dark', zoom: 1 }))).join('');
    expect(render(0)).toContain('✓ 1x (Eco)');
    expect(render(1)).toContain('✓ 2x (High)');
    expect(render(2)).toContain('✓ 4x (Ultra)');
    expect(render(1)).not.toContain('✓ 1x');
  });

  it('Mode items tick the active operation mode (same mechanism)', () => {
    const mode = NAVBAR_MENUS[2];
    const items = mode.items.filter(i => i.action === 'operationMode');
    const html = items.map(item => renderItemHtml(item, currentValue({ operationMode: 3, oversampling: 0, midiChannel: 1, theme: 'dark', zoom: 1 }))).join('');
    expect(html.match(/✓/g) || []).toHaveLength(1);
    expect(html).toContain('✓ Modern (Enhanced Features)');
  });

  it('isItemChecked: explicit checked flag wins, missing currentValue = unchecked', () => {
    expect(isItemChecked({ action: 'midiChannel', value: 5 }, () => 5)).toBe(true);
    expect(isItemChecked({ action: 'midiChannel', value: 5 }, () => 6)).toBe(false);
    expect(isItemChecked({ checked: true, action: 'midiChannel', value: 1 }, () => 9)).toBe(true);
    expect(isItemChecked({ action: 'midiChannel', value: 5 }, null)).toBe(false);
    expect(isItemChecked(null)).toBe(false);
  });

  it('renderItemHtml includes data-action/data-value and separators', () => {
    expect(renderItemHtml({ separator: true }, null)).toBe('<div class="nav-separator"></div>');
    expect(renderItemHtml({ action: 'zoom', value: 1.25, label: 'Zoom 125%' }, currentValue({ operationMode: 0, oversampling: 0, midiChannel: 1, theme: 'dark', zoom: 1.25 })))
      .toContain('data-action="zoom"');
    expect(renderItemHtml({ action: 'zoom', value: 1.25, label: 'Zoom 125%' }, currentValue({ operationMode: 0, oversampling: 0, midiChannel: 1, theme: 'dark', zoom: 1.25 })))
      .toContain('✓ Zoom 125%');
  });

  it('zoom submenu items tick the active zoom level', () => {
    const view = NAVBAR_MENUS[3];
    const zooms = view.items.filter(i => i.action === 'zoom');
    const html = zooms.map(item => renderItemHtml(item, currentValue({ operationMode: 0, oversampling: 0, midiChannel: 1, theme: 'dark', zoom: 1.5 }))).join('');
    expect(html.match(/✓/g) || []).toHaveLength(1);
    expect(html).toContain('✓ Zoom 150%');
  });
});

describe('Patch randomizer (native PresetRandomizer parity)', () => {
  // Deterministic RNG so tests are reproducible: linear congruential generator.
  const makeRng = (seed = 42) => {
    let s = seed >>> 0;
    return () => {
      s = (s * 1664525 + 1013904223) >>> 0;
      return s / 0x100000000;
    };
  };

  it('returns params within the native ranges', () => {
    const patch = generateRandomPatch(makeRng());
    const p = patch.params;

    expect(p.LINE_SELECT).toBeGreaterThanOrEqual(0);
    expect(p.LINE_SELECT).toBeLessThan(4);
    expect(p.OSC1_WAVEFORM).toBeGreaterThanOrEqual(0);
    expect(p.OSC1_WAVEFORM).toBeLessThan(8);
    expect(p.OSC1_LEVEL).toBeGreaterThanOrEqual(0.8);
    expect(p.OSC1_LEVEL).toBeLessThanOrEqual(1.0);
    expect(p.OSC2_DETUNE).toBeGreaterThanOrEqual(-2.0);
    expect(p.OSC2_DETUNE).toBeLessThanOrEqual(2.0);
    expect(p.LFO_RATE).toBeGreaterThanOrEqual(0.5);
    expect(p.LFO_RATE).toBeLessThanOrEqual(8.5);
    expect(p.LFO_DEPTH).toBeLessThanOrEqual(0.3);
    expect(p.ARP_OCTAVE).toBeGreaterThanOrEqual(1);
    expect(p.ARP_OCTAVE).toBeLessThanOrEqual(3);
    expect(p.ARP_BPM).toBeGreaterThanOrEqual(80);
    expect(p.ARP_BPM).toBeLessThanOrEqual(140);
  });

  it('drive/arp/chorus are mostly off (native probabilities)', () => {
    let driveOn = 0, arpOn = 0, chorusOn = 0;
    for (let i = 0; i < 100; i++) {
      const p = generateRandomPatch(makeRng(i)).params;
      if (p.DRIVE_MIX > 0) driveOn++;
      if (p.ARP_ENABLED === 1) arpOn++;
      if (p.CHORUS_MIX > 0) chorusOn++;
    }
    expect(driveOn).toBeGreaterThan(0);
    expect(driveOn).toBeLessThan(60); // ~20%
    expect(arpOn).toBeGreaterThan(0);
    expect(arpOn).toBeLessThan(60); // ~20%
    expect(chorusOn).toBeGreaterThan(0);
    expect(chorusOn).toBeLessThan(60); // ~30%
  });

  it('envelopes respect the native shape (3-6 stages, DCA endpoint silence)', () => {
    const patch = generateRandomPatch(makeRng());
    const dca = patch.envelopes.dca.line1;
    const dcw = patch.envelopes.dcw.line1;

    expect(dca.endPoint).toBeGreaterThanOrEqual(3);
    expect(dca.endPoint).toBeLessThanOrEqual(6);
    expect(dca.levels[dca.endPoint]).toBe(0); // DCA endpoint must be silence
    expect(dca.rates).toHaveLength(8);
    expect(dcw.levels[dcw.endPoint]).toBeGreaterThanOrEqual(0);

    // line2 mirrors line1 (native copies envelopes to line 2)
    expect(patch.envelopes.dca.line2.endPoint).toBe(dca.endPoint);
    expect(patch.envelopes.pitch.line2.rates).toEqual(patch.envelopes.pitch.line1.rates);
  });

  it('Help menu Manual / Wiki opens the real project repo (not a placeholder)', () => {
    // The navbar actions moved to the extracted navbar module.
    const navbar = fs.readFileSync(path.resolve(__dirname, '../src/ui/navbar.js'), 'utf8');
    // Native item 900 (Manual / Wiki) is a stub in the C++ editor; the web
    // version must point at the actual project repository.
    expect(navbar).toContain("window.open('https://github.com/ajabadia/ABDOmegaUnified', '_blank');");
    expect(navbar).not.toContain("window.open('https://github.com/', '_blank')");
    const help = NAVBAR_MENUS[4];
    expect(help.items.map(i => i.label)).toEqual(['Manual / Wiki', 'About...']);
  });

  it('Audio Settings action opens the SETTINGS drawer (native case 206)', () => {
    // The navbar actions moved to the extracted navbar module.
    const navbar = fs.readFileSync(path.resolve(__dirname, '../src/ui/navbar.js'), 'utf8');
    // Native PluginEditor.cpp case 206 calls the host audio settings; the web
    // equivalent is the drawer holding the OUTPUT / AUDIO OUTPUT device select.
    expect(navbar).toContain("openBlockDrawer('model-modes', 'SETTINGS');");
  });

  it('pitch envelope is flat most of the time (80%)', () => {
    let flatCount = 0;
    for (let i = 0; i < 100; i++) {
      const p = generateRandomPatch(makeRng(i + 1000)).envelopes.pitch.line1;
      if (p.sustainPoint === 0 && p.endPoint === 1 && p.rates.every(r => r === 0.5) && p.levels.every(l => l === 0.5)) {
        flatCount++;
      }
    }
    expect(flatCount).toBeGreaterThan(50); // ~80% flat
    expect(flatCount).toBeLessThan(100);
  });
});
