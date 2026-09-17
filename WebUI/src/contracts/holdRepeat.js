// Hold-to-repeat for LCD keypad buttons — parity with the native
// LCDKeypad::cursorUp/Down/Left/Right.setRepeatSpeed(400, 60):
// first action fires immediately on press, then after the initial delay the
// button repeats every `repeat` ms while held. Pure and testable: attachHoldRepeat
// wires the pointer events itself, so the app just passes the fire callback.
//
// Native (Source/UI/Components/LCDKeypad.cpp):
//   m_upButton->setRepeatSpeed(400, 60); // etc.
// Initial delay 400 ms, repeat every 60 ms (JUCE setRepeatSpeed semantics).

export const HOLD_REPEAT_INITIAL_MS = 400;
export const HOLD_REPEAT_INTERVAL_MS = 60;

export function attachHoldRepeat(el, fire, { initial = HOLD_REPEAT_INITIAL_MS, repeat = HOLD_REPEAT_INTERVAL_MS } = {}) {
  if (!el) return () => {};
  let holdTimer = null;
  let repeatTimer = null;
  let lastPointerDown = 0;

  const clear = () => {
    if (holdTimer) clearTimeout(holdTimer);
    if (repeatTimer) clearInterval(repeatTimer);
    holdTimer = null;
    repeatTimer = null;
  };

  const start = (e) => {
    if (e.button !== undefined && e.button !== 0) return; // left button only
    lastPointerDown = Date.now();
    fire();
    clear();
    holdTimer = setTimeout(() => {
      repeatTimer = setInterval(fire, repeat);
    }, initial);
  };

  el.addEventListener('pointerdown', start);
  el.addEventListener('pointerup', clear);
  el.addEventListener('pointerleave', clear);
  el.addEventListener('pointercancel', clear);
  // Keyboard activation (Enter/Space) and synthetic .click() fire the action
  // too, but a real pointer tap fires pointerdown + click — skip the duplicate.
  el.addEventListener('click', () => {
    if (Date.now() - lastPointerDown < 250) return;
    fire();
  });

  return clear;
}
