import { describe, it, expect, vi } from 'vitest';
import {
  MIDIActivityIndicator,
  MIDI_LED_INTERVAL_MS,
  MIDI_LED_FADE_SPEED
} from '../src/contracts/midiActivity.js';

// Parity with the native MIDIActivityIndicator (Source/UI/Components/
// MIDIActivityIndicator.cpp): timer 30 ms, triggerActivity() -> brightness 1.0
// for one tick, then fade by FADE_SPEED 0.1 per tick, clamped at 0.

describe('MIDIActivityIndicator (native MIDIActivityIndicator parity)', () => {
  it('starts dark (brightness 0) and inactive', () => {
    const ind = new MIDIActivityIndicator();
    expect(ind.brightness).toBe(0);
    expect(ind.isActive).toBe(false);
  });

  it('exposes the native timing constants', () => {
    expect(MIDI_LED_INTERVAL_MS).toBe(30); // startTimer(30)
    expect(MIDI_LED_FADE_SPEED).toBe(0.1); // FADE_SPEED
  });

  it('triggerActivity lights the LED full on the next tick', () => {
    const ind = new MIDIActivityIndicator();
    ind.triggerActivity();
    expect(ind.isActive).toBe(true);
    ind.tick();
    expect(ind.brightness).toBe(1.0);
    expect(ind.isActive).toBe(false);
  });

  it('fades brightness by FADE_SPEED per tick while idle', () => {
    const ind = new MIDIActivityIndicator();
    ind.triggerActivity();
    ind.tick(); // brightness -> 1.0
    ind.tick(); // fade 1
    expect(ind.brightness).toBeCloseTo(1.0 - MIDI_LED_FADE_SPEED);
    ind.tick(); // fade 2
    expect(ind.brightness).toBeCloseTo(1.0 - 2 * MIDI_LED_FADE_SPEED);
  });

  it('clamps brightness at 0 (never negative)', () => {
    const ind = new MIDIActivityIndicator();
    ind.triggerActivity();
    for (let i = 0; i < 20; i++) ind.tick();
    expect(ind.brightness).toBe(0);
    // keeps returning 0 on further idle ticks
    ind.tick();
    expect(ind.brightness).toBe(0);
  });

  it('re-trigger mid-fade jumps back to full brightness', () => {
    const ind = new MIDIActivityIndicator();
    ind.triggerActivity();
    ind.tick(); // 1.0
    ind.tick(); // 0.9
    ind.triggerActivity();
    ind.tick(); // back to 1.0
    expect(ind.brightness).toBe(1.0);
  });

  it('calls onRender with the brightness on every tick', () => {
    const onRender = vi.fn();
    const ind = new MIDIActivityIndicator({ onRender });
    ind.triggerActivity();
    ind.tick();
    expect(onRender).toHaveBeenCalledTimes(1);
    expect(onRender).toHaveBeenLastCalledWith(1.0);
    ind.tick();
    expect(onRender).toHaveBeenLastCalledWith(1.0 - MIDI_LED_FADE_SPEED);
  });

  it('idle ticks (no activity) still render the fade value', () => {
    const onRender = vi.fn();
    const ind = new MIDIActivityIndicator({ onRender });
    ind.tick(); // no activity yet: 0 - 0.1 -> clamped 0
    expect(onRender).toHaveBeenLastCalledWith(0);
  });
});
