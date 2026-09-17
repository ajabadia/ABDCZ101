// Free Modulation Matrix (ABDEEP-style): 8 slots of Source→Dest→Depth.
// Extracted from app.js to keep responsibilities separated.
//
// The controls are plain <select>/<input> elements with the registered param
// ids (MOD_SLOT_<n>_SRC/DEST/DEPTH), so the generic registry wiring in app.js
// already sends each change to the WASM engine. Here we only build the DOM and
// keep the per-slot ON/OFF badge in sync.
//
// Factory pattern: the engine/registry helpers and the LCD state are app-scope
// references, so deps receives getters (read at runtime, never at init).
import { MOD_MATRIX_EXAMPLE_SEEDS, isMatrixEmpty } from '../contracts/modMatrixDefaults.js';

export function createModMatrix(deps) {
  const sendParameter = deps.getSendParameter();
  const rawToNormalized = deps.getRawToNormalized();
  const getLcdMenuIndex = deps.getLcdMenuIndex;
  // LCD panel refs are read lazily: the LCD panel factory is instantiated
  // AFTER this module in app.js (the two are mutually dependent — the matrix
  // refreshes the LCD readout, the LCD seeds the matrix in Modern mode), so
  // never dereference them at factory time.
  const getActiveMenuParameters = deps.getActiveMenuParameters;
  const getUpdateLCD = deps.getUpdateLCD;
  const getCurrentOperationMode = deps.getCurrentOperationMode;

  const MOD_SLOT_SOURCES = ['None', 'Velocity', 'Mod Wheel', 'Aftertouch', 'Key Track', 'LFO', 'Env DCW', 'Env DCA', 'Env Pitch', 'Pitch Bend', 'Noise', 'Authentic Key Track'];
  const MOD_SLOT_DESTS = ['None', 'DCW', 'DCA', 'Pitch', 'Vibrato', 'LFO Rate', 'Osc2 Detune', 'Pan'];
  const MOD_MATRIX_SLOTS = 8;

  const buildModMatrix = () => {
    const container = document.getElementById('mod-matrix');
    if (!container || container.children.length > 0) return;
    for (let s = 1; s <= MOD_MATRIX_SLOTS; s++) {
      const row = document.createElement('div');
      row.className = 'mod-slot';

      const title = document.createElement('div');
      title.className = 'mod-slot-title';
      title.textContent = `SLOT ${s}`;
      const badge = document.createElement('span');
      badge.className = 'mod-slot-badge';
      badge.id = `mod-slot-badge-${s}`;
      badge.textContent = 'OFF';
      title.appendChild(badge);

      const srcSel = document.createElement('select');
      srcSel.id = `MOD_SLOT_${s}_SRC`;
      srcSel.className = 'synth-select mod-slot-src';
      MOD_SLOT_SOURCES.forEach((name, i) => srcSel.appendChild(new Option(name, i)));

      const destSel = document.createElement('select');
      destSel.id = `MOD_SLOT_${s}_DEST`;
      destSel.className = 'synth-select mod-slot-dest';
      MOD_SLOT_DESTS.forEach((name, i) => destSel.appendChild(new Option(name, i)));

      const depth = document.createElement('input');
      depth.type = 'range';
      depth.id = `MOD_SLOT_${s}_DEPTH`;
      depth.min = '-1';
      depth.max = '1';
      depth.step = '0.01';
      depth.value = '0';
      const depthVal = document.createElement('span');
      depthVal.className = 'mod-slot-depth-val';
      depthVal.id = `val-MOD_SLOT_${s}_DEPTH`;
      depthVal.textContent = '0.00';

      row.appendChild(title);
      row.appendChild(srcSel);
      row.appendChild(destSel);
      row.appendChild(depth);
      row.appendChild(depthVal);
      container.appendChild(row);
    }
  };

  // Keep the slot badge + depth label in sync (depth values are read from the
  // registry-driven controls; called on load and after each change). Also keeps
  // the active-slot counter on the surface EDIT button up to date.
  const syncModSlotBadges = () => {
    let activeCount = 0;
    for (let s = 1; s <= MOD_MATRIX_SLOTS; s++) {
      const src = document.getElementById(`MOD_SLOT_${s}_SRC`);
      const depth = document.getElementById(`MOD_SLOT_${s}_DEPTH`);
      const val = document.getElementById(`val-MOD_SLOT_${s}_DEPTH`);
      const badge = document.getElementById(`mod-slot-badge-${s}`);
      if (!src || !depth || !val || !badge) continue;
      const active = parseInt(src.value) !== 0;
      if (active) activeCount++;
      badge.textContent = active ? 'ON' : 'OFF';
      badge.classList.toggle('active', active);
      val.textContent = parseFloat(depth.value).toFixed(2);
    }
    const counter = document.getElementById('mod-edit-active-count');
    if (counter) {
      counter.textContent = activeCount;
      counter.hidden = activeCount === 0;
    }
  };

  // LCD readout for the computed AUTH K-TRACK entry (Modern only): mirrors the
  // native "K-TRACK DCW/PIT" rows by showing the active Authentic Key Track
  // route(s) of the free matrix. Returns e.g. "DCW 0.50", "DCW 0.50 +1" when
  // several slots use the source, or "OFF" when none does.
  const getAuthKeyTrackLcdValue = () => {
    const routes = [];
    for (let s = 1; s <= MOD_MATRIX_SLOTS; s++) {
      const src = document.getElementById(`MOD_SLOT_${s}_SRC`);
      const dest = document.getElementById(`MOD_SLOT_${s}_DEST`);
      const depth = document.getElementById(`MOD_SLOT_${s}_DEPTH`);
      if (!src || !dest || !depth) continue;
      if (parseInt(src.value) !== 11) continue; // 11 = Authentic Key Track
      const destName = MOD_SLOT_DESTS[parseInt(dest.value)] || '?';
      routes.push(`${destName} ${parseFloat(depth.value).toFixed(2)}`);
    }
    if (routes.length === 0) return 'OFF';
    const extra = routes.length > 1 ? ` +${routes.length - 1}` : '';
    return `${routes[0]}${extra}`;
  };

  const initModMatrix = () => {
    buildModMatrix();
    syncModSlotBadges();
    // The matrix controls are created at runtime (after the generic registry
    // wiring ran), so register the sendParameter + badge wiring by hand.
    for (let s = 1; s <= MOD_MATRIX_SLOTS; s++) {
      const src = document.getElementById(`MOD_SLOT_${s}_SRC`);
      const dest = document.getElementById(`MOD_SLOT_${s}_DEST`);
      const depth = document.getElementById(`MOD_SLOT_${s}_DEPTH`);
      const wire = (el) => {
        if (!el) return;
        const update = () => {
          const rawVal = el.tagName === 'SELECT' ? parseInt(el.value) : parseFloat(el.value);
          sendParameter(el.id, rawToNormalized(el.id, rawVal));
          syncModSlotBadges();
          // If the LCD is parked on the computed AUTH K-TRACK entry, refresh its
          // readout (user interaction only — safe, the LCD refs resolve then).
          if (getLcdMenuIndex() >= 0 && getActiveMenuParameters()[getLcdMenuIndex()]?.computed) getUpdateLCD();
        };
        el.addEventListener('input', update);
        el.addEventListener('change', update);
      };
      wire(src);
      wire(dest);
      wire(depth);
    }
  };
  initModMatrix();

  // Seed the example matrix slots on NEW presets in Modern mode. The fixed
  // hardware routes were removed (each one is replicable with the free matrix),
  // so a fresh preset starts with Velocity→DCA (the authentic base) and a
  // KeyTrack→Pitch example. Only applies when the matrix is completely empty
  // (nothing user-configured) — a routing the user built is never clobbered.
  const applyModernMatrixSeeds = () => {
    // Always refresh the slot badges + EDIT counter after a preset/bank load:
    // preset params are applied to the DOM by applyParameterToUI, which does not
    // update the badges — without this a preset that carries its own matrix
    // (e.g. the factory "Modern Key Track") would show all slots as OFF.
    syncModSlotBadges();
    if (getCurrentOperationMode() !== 2) return;
    const srcs = Array.from({ length: 8 }, (_, i) => document.getElementById(`MOD_SLOT_${i + 1}_SRC`));
    if (srcs.some(el => !el)) return; // matrix drawer not built yet
    if (!isMatrixEmpty(srcs.map(el => el.value))) return;
    MOD_MATRIX_EXAMPLE_SEEDS.forEach(({ slot, src, dest, depth }) => {
      const srcEl = document.getElementById(`MOD_SLOT_${slot}_SRC`);
      const destEl = document.getElementById(`MOD_SLOT_${slot}_DEST`);
      const depthEl = document.getElementById(`MOD_SLOT_${slot}_DEPTH`);
      if (!srcEl || !destEl || !depthEl) return;
      srcEl.value = String(src);
      destEl.value = String(dest);
      depthEl.value = String(depth);
      // The matrix wire handlers (initModMatrix) send each change to the engine
      // and keep the badges in sync — dispatch input + change for each control.
      [srcEl, destEl, depthEl].forEach(el => {
        el.dispatchEvent(new Event('input', { bubbles: true }));
        el.dispatchEvent(new Event('change', { bubbles: true }));
      });
    });
    syncModSlotBadges();
  };

  return {
    MOD_SLOT_SOURCES,
    MOD_SLOT_DESTS,
    MOD_MATRIX_SLOTS,
    syncModSlotBadges,
    getAuthKeyTrackLcdValue,
    applyModernMatrixSeeds
  };
}
