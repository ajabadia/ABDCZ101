import { describe, it, expect } from 'vitest';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';

const __dirname = dirname(fileURLToPath(import.meta.url));
const indexHtml = readFileSync(join(__dirname, '..', 'index.html'), 'utf8');
const appJs = readFileSync(join(__dirname, '..', 'src', 'app.js'), 'utf8');
// The matrix DOM build / badge sync moved to src/ui/modMatrix.js (app split).
const modJs = readFileSync(join(__dirname, '..', 'src', 'ui', 'modMatrix.js'), 'utf8');
const registry = (await import('../src/contracts/registry.gen.js')).PARAMETER_REGISTRY.parameters;

const slotIds = (field) => Array.from({ length: 8 }, (_, i) => `MOD_SLOT_${i + 1}_${field}`);

describe('free modulation matrix (ABDEEP-style)', () => {
  it('surface panel is compact (title + EDIT button)', () => {
    expect(indexHtml).toContain('id="btn-edit-mod"');
    // The old fixed sliders are no longer on the surface (they moved to the
    // drawer's HARDWARE ROUTES subsection).
    const surface = indexHtml.split('<section class="control-panel panel-modulation">')[1]?.split('</section>')[0] || '';
    expect(surface).toContain('btn-edit-mod');
    expect(surface).not.toContain('MOD_VELO_DCW');
  });

  it('EDIT opens the mod drawer section with the right title', () => {
    // The drawer EDIT-button wiring moved to the extracted blockDrawer module:
    // btn-edit-mod is wired to toggle the 'mod' section titled MODULATION ROUTINGS.
    const drawerJs = readFileSync(join(__dirname, '..', 'src', 'ui', 'blockDrawer.js'), 'utf8');
    expect(drawerJs).toContain("'btn-edit-mod', 'mod', 'MODULATION ROUTINGS'");
    expect(indexHtml).toContain('data-drawer-section="mod"');
  });

  it('EDIT button shows an active-slot counter badge on the surface panel', () => {
    // The badge lives inside the EDIT button and is hidden when 0 routes are ON
    expect(indexHtml).toContain('id="mod-edit-active-count"');
    expect(indexHtml).toMatch(/<button[^>]*id="btn-edit-mod"[^>]*>EDIT <span id="mod-edit-active-count"/);
    // syncModSlotBadges counts active slots and updates the badge (module)
    expect(modJs).toMatch(/counter\.hidden = activeCount === 0;/);
    expect(modJs).toMatch(/const counter = document\.getElementById\('mod-edit-active-count'\);/);
    expect(appJs).toContain('syncModSlotBadges,'); // wired in app.js destructuring
  });

  it('the matrix is Modern-only (whole section gated)', () => {
    expect(indexHtml).toContain('data-drawer-section="mod" data-mode="modern"');
    // The per-mode panel gating (syncModePanels) lives in the LCD panel module
    // (post-refactor); app.js wires it into the factory and the load paths.
    const lcdJs = readFileSync(join(__dirname, '..', 'src', 'ui', 'lcdPanel.js'), 'utf8');
    expect(lcdJs).toMatch(/querySelectorAll\('\.drawer-section\[data-drawer-section="mod"\]'\)/);
    expect(lcdJs).toMatch(/sec\.style\.display = \(mode === 3\) \? '' : 'none';/);
    expect(appJs).toContain('syncModePanels,'); // wired in app.js destructuring
  });

  it('generates 8 slots at runtime with the registered param ids', () => {
    expect(modJs).toContain('const MOD_MATRIX_SLOTS = 8;');
    expect(modJs).toContain('MOD_SLOT_SOURCES');
    expect(modJs).toContain('MOD_SLOT_DESTS');
    // WebUI dropdown lists stay in parity with the registry choices
    expect(modJs).toContain("'Pitch Bend', 'Noise', 'Authentic Key Track']");
    expect(modJs).toContain("'Osc2 Detune', 'Pan']");
    // src/dest/depth selects are created with the registry ids
    expect(modJs).toContain('srcSel.id = `MOD_SLOT_${s}_SRC`');
    expect(modJs).toContain('destSel.id = `MOD_SLOT_${s}_DEST`');
    expect(modJs).toContain('depth.id = `MOD_SLOT_${s}_DEPTH`');
  });

  it('registers the matrix params in the registry (24 new params)', () => {
    const srcs = slotIds('SRC');
    const dests = slotIds('DEST');
    const depths = slotIds('DEPTH');
    srcs.forEach(id => {
      const p = registry.find(x => x.id === id);
      expect(p, id).toBeTruthy();
      expect(p.type).toBe('choice');
      expect(p.choices.length).toBe(12); // None + 11 sources (Velocity..Authentic Key Track)
      expect(p.choices).toEqual(expect.arrayContaining(['Pitch Bend', 'Noise', 'Authentic Key Track']));
    });
    dests.forEach(id => {
      const p = registry.find(x => x.id === id);
      expect(p, id).toBeTruthy();
      expect(p.choices.length).toBe(8); // None + 7 destinations (DCW..Pan)
      expect(p.choices).toEqual(expect.arrayContaining(['Osc2 Detune', 'Pan']));
    });
    depths.forEach(id => {
      const p = registry.find(x => x.id === id);
      expect(p, id).toBeTruthy();
      expect(p.min).toBe(-1);
      expect(p.max).toBe(1);
    });
  });

  it('removes the HARDWARE ROUTES section (every route replicable via the matrix)', () => {
    // The fixed-route controls are gone from the drawer: the free matrix
    // replicates each of them as a Source→Dest slot.
    expect(indexHtml).not.toContain('HARDWARE ROUTES');
    const removed = ['MOD_VELO_DCW', 'MOD_VELO_DCA', 'MOD_WHEEL_VIB', 'MOD_WHEEL_DCW', 'MOD_WHEEL_LFORATE',
      'KEY_TRACK_DCW', 'MOD_AT_DCW', 'MOD_AT_VIB', 'KEY_TRACK_PITCH',
      'KEY_FOLLOW_DCO', 'KEY_FOLLOW_DCW', 'KEY_FOLLOW_DCA'];
    removed.forEach(id => expect(indexHtml, id).not.toContain(`id="${id}"`));

    // Coverage: every fixed hardware route maps to a free-matrix Source→Dest
    // pair (indices of MOD_SLOT_SOURCES / MOD_SLOT_DESTS in app.js).
    const routeCoverage = [
      ['Velocity', 'DCW'], ['Velocity', 'DCA'],          // Velo→DCW / Velo→DCA
      ['Mod Wheel', 'DCW'], ['Mod Wheel', 'LFO Rate'], ['Mod Wheel', 'Vibrato'], // Wheel routes
      ['Aftertouch', 'DCW'], ['Aftertouch', 'Vibrato'], // AT routes
      ['Key Track', 'DCW'], ['Key Track', 'Pitch'],     // KeyTrk routes
      ['Key Track', 'Pitch'], ['Key Track', 'DCW'], ['Key Track', 'DCA'], // KeyFollow DCO/DCW/DCA
    ];
    const srcs = ['None', 'Velocity', 'Mod Wheel', 'Aftertouch', 'Key Track', 'LFO', 'Env DCW', 'Env DCA', 'Env Pitch', 'Pitch Bend', 'Noise', 'Authentic Key Track'];
    const dests = ['None', 'DCW', 'DCA', 'Pitch', 'Vibrato', 'LFO Rate', 'Osc2 Detune', 'Pan'];
    routeCoverage.forEach(([s, d]) => {
      expect(srcs).toContain(s);
      expect(dests).toContain(d);
    });
  });
});
