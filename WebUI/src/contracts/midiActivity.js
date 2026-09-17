// MIDI activity LED — parity with the native MIDIActivityIndicator
// (Source/UI/Components/MIDIActivityIndicator.cpp).
//
// Native behaviour:
//   - startTimer(30): timerCallback runs every 30 ms.
//   - triggerActivity(): isActive = true.
//   - timerCallback(): if isActive -> brightness = 1.0 (full on, one tick),
//     then isActive = false; otherwise brightness -= FADE_SPEED (0.1),
//     clamped at 0.0. Repaints.
//   - paint(): dark outer circle (#2a2a2a); when brightness > 0, an inner
//     circle (#4a9eff) drawn with alpha = brightness.
//
// This module reproduces the same state machine as a pure, testable class;
// the DOM rendering is delegated through an `onRender(brightness)` callback.

export const MIDI_LED_INTERVAL_MS = 30;   // native startTimer(30)
export const MIDI_LED_FADE_SPEED = 0.1;   // native FADE_SPEED

export class MIDIActivityIndicator {
  constructor({ onRender } = {}) {
    this.onRender = onRender || null;
    this.isActive = false;
    this.brightness = 0.0;
  }

  // Native triggerActivity(): called on any MIDI message; lights the LED on
  // the next tick.
  triggerActivity() {
    this.isActive = true;
  }

  // Native timerCallback(): runs every MIDI_LED_INTERVAL_MS.
  tick() {
    if (this.isActive) {
      this.brightness = 1.0;
      this.isActive = false;
    } else {
      this.brightness -= MIDI_LED_FADE_SPEED;
      if (this.brightness < 0.0) this.brightness = 0.0;
    }
    if (this.onRender) this.onRender(this.brightness);
  }
}
