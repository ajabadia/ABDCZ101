import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import {
  attachHoldRepeat,
  HOLD_REPEAT_INITIAL_MS,
  HOLD_REPEAT_INTERVAL_MS
} from '../src/contracts/holdRepeat.js';

// Parity with the native LCDKeypad::cursorUp/Down/Left/Right.setRepeatSpeed(400, 60):
// first action fires immediately on press, then after 400 ms the button keeps
// firing every 60 ms while held. Uses pointer events with a click fallback for
// keyboard activation, so the action never double-fires on a real tap.
//
// The vitest environment is node, so the element is a tiny mock that records
// its listeners; tests drive it by invoking those listeners directly.

function makeButton() {
  const listeners = {};
  const el = {
    addEventListener(type, fn) {
      listeners[type] = fn;
    },
    _fire(type, ev) {
      if (listeners[type]) listeners[type](ev || {});
    }
  };
  return el;
}

const pointerDown = (el, button = 0) => el._fire('pointerdown', { button });
const pointerUp = (el) => el._fire('pointerup', {});
const click = (el) => el._fire('click', {});

describe('attachHoldRepeat (native setRepeatSpeed parity)', () => {
  beforeEach(() => {
    vi.useFakeTimers();
  });
  afterEach(() => {
    vi.clearAllTimers();
    vi.useRealTimers();
  });

  it('exports the native repeat timings (400 ms initial, 60 ms repeat)', () => {
    expect(HOLD_REPEAT_INITIAL_MS).toBe(400);
    expect(HOLD_REPEAT_INTERVAL_MS).toBe(60);
  });

  it('fires the action immediately on pointerdown', () => {
    const el = makeButton();
    let calls = 0;
    attachHoldRepeat(el, () => { calls += 1; });
    pointerDown(el);
    expect(calls).toBe(1);
  });

  it('ignores non-left mouse buttons', () => {
    const el = makeButton();
    let calls = 0;
    attachHoldRepeat(el, () => { calls += 1; });
    pointerDown(el, 2); // right click
    expect(calls).toBe(0);
  });

  it('repeats every HOLD_REPEAT_INTERVAL_MS while held after the initial delay', () => {
    const el = makeButton();
    let calls = 0;
    attachHoldRepeat(el, () => { calls += 1; });
    pointerDown(el);
    expect(calls).toBe(1); // immediate
    // After the initial delay the interval starts; its first tick lands at
    // initial + repeat (setInterval semantics).
    vi.advanceTimersByTime(HOLD_REPEAT_INITIAL_MS + HOLD_REPEAT_INTERVAL_MS);
    expect(calls).toBe(2); // first repeat fired
    vi.advanceTimersByTime(HOLD_REPEAT_INTERVAL_MS);
    expect(calls).toBe(3); // second repeat fired
  });

  it('stops repeating on pointerup', () => {
    const el = makeButton();
    let calls = 0;
    attachHoldRepeat(el, () => { calls += 1; });
    pointerDown(el);
    vi.advanceTimersByTime(HOLD_REPEAT_INITIAL_MS + HOLD_REPEAT_INTERVAL_MS);
    const before = calls;
    pointerUp(el);
    vi.advanceTimersByTime(HOLD_REPEAT_INTERVAL_MS * 2);
    expect(calls).toBe(before); // no more repeats after release
  });

  it('fires once on a real click (pointerdown + click dedup)', () => {
    const el = makeButton();
    let calls = 0;
    attachHoldRepeat(el, () => { calls += 1; });
    pointerDown(el);
    click(el); // browser fires click after pointerup
    expect(calls).toBe(1); // not double-fired
  });

  it('fires on click without pointerdown (keyboard Enter/Space activation)', () => {
    const el = makeButton();
    let calls = 0;
    attachHoldRepeat(el, () => { calls += 1; });
    click(el);
    expect(calls).toBe(1);
  });
});
