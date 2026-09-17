// Compare A/B and Write-mode managers — shared by app.js (the LCD keypad flow)
// and the test suite. The DOM/engine side effects are injected through `deps`,
// so the state machines are fully testable without a browser:
//
//   CompareManager — parity with the native PresetManager::setCompareMode:
//     enter() snapshots the current edit buffer (params + envelopes + slot) and
//     loads the SAVED version of that slot from the bank; exit() restores the
//     edited buffer exactly. A null/empty snapshot makes enter() a no-op.
//
//   WriteManager — the WRT -> slot -> name -> save sequence:
//     enter() starts in the 'slot' phase (▲/▼ pick the destination slot),
//     advanceToName() seeds the name and moves to the 'name' phase (◀/▶ move
//     the cursor, ▲/▼ cycle the character through the LCD alphabet), and
//     commit() saves to the chosen slot with the edited name, then resets.
//     ◀/▶ in the slot phase cancels without saving.

const deepCopy = (obj) => JSON.parse(JSON.stringify(obj));

// --- Compare A/B ---
export function createCompareManager(deps) {
  let active = null; // { slot, params, envelopes } | null

  return {
    get isActive() { return active !== null; },

    /** 0-based slot being compared (null when no compare session is active). */
    get slot() { return active ? active.slot : null; },

    /** Snapshot the edit buffer and load the SAVED version of the current slot. */
    enter() {
      const snap = deps.readSnapshot();
      if (!snap) return false;
      active = snap;
      deps.loadSaved(snap.slot);
      return true;
    },

    /** Restore the edited buffer (params + envelopes). No-op when idle. */
    exit() {
      if (!active) return;
      const snap = active;
      active = null;
      deps.restore(snap);
    },

    clear() {
      active = null;
    }
  };
}

// --- Write mode (WRT -> slot -> name -> save) ---
export function createWriteManager(deps) {
  const { slotCount = 64 } = deps;
  const state = {
    active: false,
    phase: 'slot', // 'slot' | 'name'
    targetSlot: -1,
    name: '',
    cursor: 0
  };

  return {
    get state() { return state; },

    get isActive() { return state.active; },

    enter() {
      let slot = deps.getCurrentSlot();
      if (!Number.isInteger(slot) || slot < 0) slot = 0;
      state.active = true;
      state.phase = 'slot';
      state.targetSlot = slot;
      state.name = '';
      state.cursor = 0;
    },

    exit() {
      state.active = false;
      state.phase = 'slot';
      state.targetSlot = -1;
      state.name = '';
      state.cursor = 0;
    },

    /** ▲/▼ in the slot phase (clamped to 0..slotCount-1). */
    cycleSlot(isUp) {
      if (!state.active || state.phase !== 'slot') return;
      state.targetSlot = isUp
        ? Math.min(slotCount - 1, state.targetSlot + 1)
        : Math.max(0, state.targetSlot - 1);
    },

    /** WRT in the slot phase: seed the name and move to the name phase. */
    advanceToName() {
      if (!state.active || state.phase !== 'slot') return;
      state.name = deps.seedName(state.targetSlot);
      state.cursor = Math.min(state.name.length, deps.nameMaxLen - 1);
      state.phase = 'name';
    },

    /** ◀/▶ in the name phase (clamped to 0..nameMaxLen-1). */
    moveCursor(delta) {
      if (!state.active || state.phase !== 'name') return;
      state.cursor = Math.max(0, Math.min(deps.nameMaxLen - 1, state.cursor + delta));
    },

    /** ▲/▼ in the name phase: cycle the character under the cursor. */
    cycleChar(isUp) {
      if (!state.active || state.phase !== 'name') return;
      state.name = deps.cycleChar(state.name, state.cursor, isUp);
    },

    /**
     * Replace the whole name at once (fast text-editor path: the hybrid flow
     * opens the NameEditor overlay seeded with the LCD name; on save the new
     * text comes back here). Sanitized through the same seed path as
     * advanceToName (uppercase, LCD-width), cursor placed at the end so the
     * user can keep fine-tuning with ◀/▶ + ▲/▼. No-op outside the name phase.
     */
    setName(raw) {
      if (!state.active || state.phase !== 'name') return;
      state.name = deps.sanitizeName
        ? deps.sanitizeName(raw)
        : String(raw == null ? '' : raw).toUpperCase().slice(0, deps.nameMaxLen);
      state.cursor = Math.min(state.name.length, deps.nameMaxLen - 1);
    },

    /**
     * WRT in the name phase: save to the chosen slot with the edited name and
     * reset. Returns true when the save could not happen (deps.savePreset
     * returned falsy, e.g. engine offline) — the caller shows the message and
     * the state stays reset like the native flow.
     */
    commit() {
      if (!state.active) return true;
      const index = state.targetSlot; // capture BEFORE reset
      const name = state.name.trim() || `Slot ${index + 1}`;
      this.exit();
      return deps.savePreset(index, name) !== false;
    }
  };
}
