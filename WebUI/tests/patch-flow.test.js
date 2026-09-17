import { describe, it, expect, beforeEach } from 'vitest';
import { createCompareManager, createWriteManager } from '../src/contracts/patchFlow.js';

// Tests for the two LCD flows implemented in ./contracts/patchFlow.js:
//   - Compare A/B: enter() snapshots the edit buffer (params + envelopes + slot)
//     and loads the SAVED version; exit() restores the edited buffer exactly.
//   - Write: WRT -> slot (▲/▼) -> name (◀/▶ cursor, ▲/▼ char) -> save to slot.

const makeEnvelopes = (overrides = {}) => ({
  dca: {
    line1: { rates: [1, 2, 3, 4, 5, 6, 7, 8], levels: [0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2], sustainPoint: 2, endPoint: 3 },
    line2: { rates: [11, 12, 13, 14, 15, 16, 17, 18], levels: [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8], sustainPoint: 1, endPoint: 4 }
  },
  dcw: {
    line1: { rates: [1, 1, 1, 1, 1, 1, 1, 1], levels: [0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5], sustainPoint: 0, endPoint: 1 },
    line2: { rates: [2, 2, 2, 2, 2, 2, 2, 2], levels: [0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6], sustainPoint: 2, endPoint: 3 }
  },
  pitch: {
    line1: { rates: [3, 3, 3, 3, 3, 3, 3, 3], levels: [0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4], sustainPoint: 3, endPoint: 4 },
    line2: { rates: [4, 4, 4, 4, 4, 4, 4, 4], levels: [0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3], sustainPoint: 4, endPoint: 5 }
  },
  ...overrides
});

const clone = (obj) => JSON.parse(JSON.stringify(obj));

describe('CompareManager (A/B snapshot/restore)', () => {
  let deps;
  let manager;

  const editBuffer = {
    slot: 3,
    params: { OSC1_LEVEL: 0.9, LFO_RATE: 0.4, CHORUS_MIX: 0.0 },
    envelopes: makeEnvelopes()
  };

  beforeEach(() => {
    const saved = [];
    const restored = [];
    deps = {
      saved,
      restored,
      readSnapshot: () => clone(editBuffer),
      loadSaved: (slot) => saved.push(slot),
      restore: (snap) => restored.push(clone(snap))
    };
    manager = createCompareManager(deps);
  });

  it('enter() loads the SAVED version of the current slot and becomes active', () => {
    const ok = manager.enter();
    expect(ok).toBe(true);
    expect(manager.isActive).toBe(true);
    expect(deps.saved).toEqual([3]); // loaded the saved version of slot 3
    expect(deps.restored).toEqual([]);
  });

  it('slot getter exposes the compared slot (0-based) while active, null when idle', () => {
    expect(manager.slot).toBeNull(); // no compare session yet
    manager.enter();
    expect(manager.slot).toBe(3); // editBuffer.slot
    manager.exit();
    expect(manager.slot).toBeNull();
  });

  it('exit() restores the edited snapshot (params + envelopes) exactly', () => {
    manager.enter();
    manager.exit();

    expect(manager.isActive).toBe(false);
    expect(deps.restored).toHaveLength(1);
    const restored = deps.restored[0];
    expect(restored.slot).toBe(3);
    expect(restored.params).toEqual(editBuffer.params);
    expect(restored.envelopes).toEqual(editBuffer.envelopes);
  });

  it('exit() restores a deep copy: mutating the snapshot later cannot corrupt restore data', () => {
    manager.enter();
    // Mutate the ORIGINAL envelope object in place after entering compare.
    editBuffer.envelopes.dca.line1.levels[0] = 0.0;
    editBuffer.params.OSC1_LEVEL = 0.0;

    manager.exit();
    expect(deps.restored[0].params.OSC1_LEVEL).toBe(0.9);
    expect(deps.restored[0].envelopes.dca.line1.levels[0]).toBe(0.9);
  });

  it('exit() is a no-op when no compare session is active', () => {
    manager.exit();
    expect(deps.restored).toEqual([]);
    expect(manager.isActive).toBe(false);
  });

  it('enter() returns false when the snapshot source is unavailable (engine offline)', () => {
    deps.readSnapshot = () => null;
    const ok = manager.enter();
    expect(ok).toBe(false);
    expect(manager.isActive).toBe(false);
    expect(deps.saved).toEqual([]);
  });

  it('clear() abandons an active session without restoring', () => {
    manager.enter();
    manager.clear();
    expect(manager.isActive).toBe(false);
    expect(deps.restored).toEqual([]);
  });

  it('enter() after exit() starts a fresh session (repeat A/B toggling)', () => {
    manager.enter();
    manager.exit();
    manager.enter();
    expect(deps.saved).toEqual([3, 3]);
    expect(manager.isActive).toBe(true);
  });
});

describe('WriteManager (WRT -> slot -> name -> save)', () => {
  let deps;
  let manager;

  // Mirrors app.js wiring: deps.seedName is seedLcdName, which uppercases.
  const seedName = (slot) => `SLOT ${slot + 1}`;
  const cycleChar = (name, cursor, isUp) => {
    const chars = ' ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.';
    const cur = Math.max(0, Math.min(9, cursor));
    const ch = (name[cur] || ' ').toUpperCase();
    let idx = chars.indexOf(ch);
    if (idx === -1) idx = 0;
    idx = (idx + (isUp ? 1 : -1) + chars.length) % chars.length;
    const arr = name.padEnd(10, ' ').split('');
    arr[cur] = chars[idx];
    return arr.join('').slice(0, 10);
  };

  beforeEach(() => {
    const saved = [];
    deps = {
      slotCount: 64,
      nameMaxLen: 10,
      getCurrentSlot: () => 0,
      seedName,
      cycleChar,
      saved,
      savePreset: (index, name) => { saved.push({ index, name }); return true; }
    };
    manager = createWriteManager(deps);
  });

  it('enter() starts in the slot phase on the current slot', () => {
    manager.enter();
    expect(manager.isActive).toBe(true);
    expect(manager.state.phase).toBe('slot');
    expect(manager.state.targetSlot).toBe(0);
  });

  it('▲/▼ cycle the destination slot within 0..63', () => {
    manager.enter();
    manager.cycleSlot(true); // slot 1
    manager.cycleSlot(true); // slot 2
    expect(manager.state.targetSlot).toBe(2);

    manager.cycleSlot(false); // back to 1
    expect(manager.state.targetSlot).toBe(1);
  });

  it('cycleSlot clamps at the bank edges', () => {
    deps.getCurrentSlot = () => 63;
    manager.enter();
    manager.cycleSlot(true);
    expect(manager.state.targetSlot).toBe(63); // clamped

    deps.getCurrentSlot = () => 0;
    manager.enter();
    manager.cycleSlot(false);
    expect(manager.state.targetSlot).toBe(0); // clamped
  });

  it('advanceToName() seeds the name and moves to the name phase', () => {
    manager.enter();
    manager.cycleSlot(true); // slot 1
    manager.advanceToName();

    expect(manager.state.phase).toBe('name');
    expect(manager.state.name).toBe('SLOT 2'); // seedName(slot+1) uppercased path
    expect(manager.state.cursor).toBe(manager.state.name.length); // cursor at end
  });

  it('advanceToName() seeds from the injected seedName (uppercased in app via seedLcdName)', () => {
    deps.seedName = () => 'MY INIT';
    manager.enter();
    manager.advanceToName();
    expect(manager.state.name).toBe('MY INIT');
  });

  it('◀/▶ move the cursor within the name bounds', () => {
    manager.enter();
    manager.advanceToName();
    const start = manager.state.cursor;

    manager.moveCursor(-1);
    expect(manager.state.cursor).toBe(start - 1);
    manager.moveCursor(1);
    expect(manager.state.cursor).toBe(start);

    // clamp at 0 and at nameMaxLen-1
    manager.moveCursor(-99);
    expect(manager.state.cursor).toBe(0);
    manager.moveCursor(99);
    expect(manager.state.cursor).toBe(9); // nameMaxLen - 1
  });

  it('▲/▼ cycle the character under the cursor through the alphabet', () => {
    manager.enter();
    manager.advanceToName();
    manager.moveCursor(-99); // cursor at 0

    const before = manager.state.name[0];
    manager.cycleChar(true);
    const afterUp = manager.state.name[0];
    expect(afterUp).not.toBe(before);

    manager.cycleChar(false);
    expect(manager.state.name[0]).toBe(before);
  });

  it('commit() saves to the chosen slot with the edited name, then resets', () => {
    manager.enter();
    manager.cycleSlot(true); // slot 1
    manager.advanceToName();
    manager.moveCursor(-99);
    manager.cycleChar(true); // change first char

    const nameBefore = manager.state.name.trim();
    const ok = manager.commit();

    expect(ok).toBe(true);
    expect(deps.saved).toEqual([{ index: 1, name: nameBefore }]);
    expect(manager.isActive).toBe(false);
    expect(manager.state.phase).toBe('slot'); // reset for next session
  });

  it('commit() falls back to "Slot N" when the name is blank', () => {
    deps.seedName = () => '';
    manager.enter();
    manager.advanceToName();
    manager.commit();

    expect(deps.saved).toEqual([{ index: 0, name: 'Slot 1' }]);
  });

  it('commit() returns false and does NOT save when the engine is offline', () => {
    deps.savePreset = () => false;
    manager.enter();
    manager.advanceToName();
    const ok = manager.commit();

    expect(ok).toBe(false);
    expect(deps.saved).toEqual([]);
    expect(manager.isActive).toBe(false); // state still reset (native flow)
  });

  it('exit() in the slot phase cancels without saving', () => {
    manager.enter();
    manager.cycleSlot(true); // slot 1
    manager.exit();

    expect(manager.isActive).toBe(false);
    expect(deps.saved).toEqual([]);
    expect(manager.state.targetSlot).toBe(-1);
  });

  it('phase guards: slot ops ignored in name phase and vice versa', () => {
    manager.enter();
    manager.advanceToName();
    const slotBefore = manager.state.targetSlot;

    manager.cycleSlot(true); // ignored in name phase
    expect(manager.state.targetSlot).toBe(slotBefore);

    manager.exit();
    manager.enter(); // back in slot phase
    manager.cycleChar(true); // ignored in slot phase
    expect(manager.state.name).toBe('');
  });

  it('the full WRT -> slot -> name -> save sequence saves the expected slot+name', () => {
    deps.getCurrentSlot = () => 5;
    manager.enter();                     // slot phase, target = 5
    manager.cycleSlot(true);             // target = 6
    manager.cycleSlot(true);             // target = 7
    manager.advanceToName();             // name = 'SLOT 8'
    manager.moveCursor(-99);             // cursor at 0
    manager.cycleChar(true);             // 'S' -> 'T'
    manager.cycleChar(true);             // 'T' -> 'U'
    manager.commit();

    expect(deps.saved).toEqual([{ index: 7, name: 'ULOT 8' }]);
  });

  it('the final name is trimmed of trailing spaces before saving', () => {
    manager.enter();
    manager.advanceToName();
    manager.cycleChar(true); // cursor at end: appends 'A' to 'SLOT 1'
    manager.cycleChar(true); // 'A' -> 'B'
    manager.commit();

    expect(deps.saved[0].name).toBe('SLOT 1B');
    expect(deps.saved[0].name.endsWith(' ')).toBe(false);
  });

  it('setName() replaces the whole name (hybrid text-editor path) and puts the cursor at the end', () => {
    // Hybrid naming: the NameEditor overlay seeds from the LCD name; SAVE
    // writes the new text back via setName, then ▲/▼ + ◀/▶ keep working.
    deps.sanitizeName = (raw) => String(raw).toUpperCase().slice(0, 10);
    manager.enter();
    manager.advanceToName();
    expect(manager.state.name).toBe('SLOT 1');

    manager.setName('my awesome patch');
    expect(manager.state.name).toBe('MY AWESOME'); // uppercased, sliced to 10
    expect(manager.state.cursor).toBe(9); // end of the LCD name

    // The LCD editor still works after the fast path (char under cursor).
    manager.moveCursor(-99); // cursor at 0
    manager.cycleChar(true); // 'M' -> 'N'
    expect(manager.state.name[0]).toBe('N');
  });

  it('setName() sanitizes through the injected sanitizeName (app: seedLcdName)', () => {
    deps.sanitizeName = (raw) => seedToUpper(raw);
    manager.enter();
    manager.advanceToName();
    manager.setName('  hello world  ');
    expect(manager.state.name).toBe('HELLO WORL'); // trim + upper + LCD width
  });

  it('setName() is a no-op outside the name phase', () => {
    manager.enter(); // slot phase
    manager.setName('NOPE');
    expect(manager.state.name).toBe('');
    expect(manager.state.phase).toBe('slot');
  });

  it('commit() saves the name typed through the hybrid text editor', () => {
    deps.sanitizeName = (raw) => String(raw).toUpperCase().slice(0, 10);
    manager.enter();
    manager.advanceToName();
    manager.setName('lead pad');
    manager.commit();

    expect(deps.saved).toEqual([{ index: 0, name: 'LEAD PAD' }]);
  });
});

// Mirrors seedLcdName: trim, uppercase, clamp to the LCD width.
function seedToUpper(raw) {
  return String(raw == null ? '' : raw).trim().toUpperCase().slice(0, 10);
}
