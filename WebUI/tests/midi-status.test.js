import { describe, it, expect } from 'vitest';
import {
  buildMidiStatus,
  clampChannel,
  formatDeviceNames,
  MIDI_CHANNEL_MIN,
  MIDI_CHANNEL_MAX
} from '../src/contracts/midiStatus.js';

describe('midiStatus badge', () => {
  it('offline is the default state (red, no channel shown)', () => {
    const s = buildMidiStatus();
    expect(s.text).toBe('MIDI: OFFLINE');
    expect(s.color).toBe('#ff5555');
  });

  it('online shows the device count and the active channel', () => {
    const s = buildMidiStatus({ state: 'online', devices: [{ name: 'KeyLab 61' }], channel: 3 });
    expect(s.text).toBe('MIDI: 1 DEVICE · CH 3');
    expect(s.color).toBe('#00ffcc');
    expect(s.tooltip).toContain('KeyLab 61');
    expect(s.tooltip).toContain('Ch 3');
  });

  it('online pluralizes and lists multiple device names', () => {
    const s = buildMidiStatus({
      state: 'online',
      devices: [{ name: 'A' }, { name: 'B' }],
      channel: 16
    });
    expect(s.text).toBe('MIDI: 2 DEVICES · CH 16');
    expect(s.tooltip).toContain('A, B');
  });

  it('no-devices state shows the channel (orange)', () => {
    const s = buildMidiStatus({ state: 'no-devices', channel: 8 });
    expect(s.text).toBe('MIDI: NO DEVICES · CH 8');
    expect(s.color).toBe('#ffa500');
  });

  it('unsupported / blocked states are red and channel-free', () => {
    const unsupported = buildMidiStatus({ state: 'unsupported', channel: 2 });
    expect(unsupported.text).toBe('MIDI: UNSUPPORTED');
    expect(unsupported.color).toBe('#ff5555');

    const blocked = buildMidiStatus({ state: 'blocked', channel: 2 });
    expect(blocked.text).toBe('MIDI: BLOCKED');
    expect(blocked.color).toBe('#ff5555');
  });

  it('clampChannel bounds the channel to 1..16', () => {
    expect(clampChannel(0)).toBe(MIDI_CHANNEL_MIN);
    expect(clampChannel(99)).toBe(MIDI_CHANNEL_MAX);
    expect(clampChannel(4.6)).toBe(5);
    expect(clampChannel(NaN)).toBe(MIDI_CHANNEL_MIN);
    expect(clampChannel(undefined)).toBe(MIDI_CHANNEL_MIN);
  });

  it('formatDeviceNames dedupes, shortens and marks the remainder', () => {
    expect(formatDeviceNames([{ name: 'X' }, { name: 'X' }])).toBe('X');
    expect(formatDeviceNames([{ name: 'A' }, { name: 'B' }, { name: 'C' }])).toBe('A, B +1 more');
    expect(formatDeviceNames([])).toBe('');
    expect(formatDeviceNames([{}, { name: '' }])).toBe('MIDI Input');
  });
});
