import { describe, it, expect } from 'vitest';
import fs from 'fs';
import path from 'path';

// CSS is read lazily by the tests that need it (loaded below, inside the block).
// The audio-output selector + Web MIDI cluster moved to src/ui/midiSystem.js.

describe('Audio Output Device Selector (parity with native Audio Settings)', () => {
  const appJs = fs.readFileSync(path.resolve(__dirname, '../src/app.js'), 'utf8');
  const midiJs = fs.readFileSync(path.resolve(__dirname, '../src/ui/midiSystem.js'), 'utf8');
  const html = fs.readFileSync(path.resolve(__dirname, '../index.html'), 'utf8');

  it('index.html provides the output device select and REFRESH button in the Settings drawer', () => {
    expect(html).toContain('id="audio-output-device"');
    expect(html).toContain('id="btn-refresh-audio-devices"');
    // Must live in the SET / Settings drawer section, near the top (OUTPUT / SYSTEM).
    const settingsRegion = html.slice(html.indexOf('drawer-section-title">OUTPUT'), html.indexOf('drawer-section-title">SYSTEM'));
    expect(settingsRegion).toContain('AUDIO OUTPUT');
    expect(settingsRegion).toContain('audio-output-device');
  });

  it('midiSystem enumerates audiooutput devices into the select', () => {
    expect(midiJs).toContain('navigator.mediaDevices.enumerateDevices');
    expect(midiJs).toContain("devices.filter(d => d.kind === 'audiooutput')");
    // Labels only appear after a user gesture, so it is (re)scanned at init.
    expect(appJs).toContain('midiApi.populateAudioOutputDevices()');
    expect(midiJs).toContain('btnRefreshAudioDevices.addEventListener(\'click\', populateAudioOutputDevices)');
  });

  it('midiSystem routes the AudioContext to the chosen device via setSinkId', () => {
    // The native requestAudioSettings opens JUCE's device chooser; on the web
    // the equivalent is AudioContext.setSinkId (Chromium) — same user intent:
    // pick which physical output the synth plays through.
    expect(midiJs).toContain('typeof ctx.setSinkId !== \'function\'');
    expect(midiJs).toContain('ctx.setSinkId(deviceId === \'default\' ? \'\' : deviceId)');
    // A device that disappears (unplugged) must fall back to the default.
    expect(midiJs).toContain('if (audioOutputSelect) audioOutputSelect.value = \'default\';');
  });

  it('START AUDIO pulses orange while audio is not connected, stops when connected', () => {
    const css = fs.readFileSync(path.resolve(__dirname, '../src/styles/lcd.css'), 'utf8');
    // Disconnected state: the button carries an orange pulse animation.
    expect(css).toMatch(/\.lcd-btn-audio \{[\s\S]*?animation: audio-wait-pulse/);
    expect(css).toContain('@keyframes audio-wait-pulse');
    // Connected state kills the pulse and switches to the green online glow.
    expect(css).toMatch(/\.lcd-btn-audio\.connected \{[\s\S]*?animation: none/);
    expect(css).toMatch(/\.lcd-btn-audio\.connected \{[\s\S]*?#00ffcc/);
  });

  it('app.js toggles the connected class on the audio button after engine init', () => {
    // The inline green box-shadow is gone — the .connected class governs the glow.
    expect(appJs).not.toContain("btnInit.style.boxShadow = '0 0 20px #00ffcc'");
    expect(appJs).toContain("btnInit.classList.add('connected')");
  });

  it('midiSystem persists the chosen output across launches (native persists device selection)', () => {
    expect(midiJs).toContain("localStorage.setItem(AUDIO_OUTPUT_STORAGE_KEY, deviceId)");
    // Restored from storage once the engine exists (needs a running AudioContext):
    // app.js reads the saved device id through the midiApi storage key.
    expect(appJs).toContain("localStorage.getItem(midiApi ? midiApi.AUDIO_OUTPUT_STORAGE_KEY : 'cz101.audioOutputDevice')");
  });

  it('midiSystem keeps the list fresh on device changes', () => {
    expect(midiJs).toContain("navigator.mediaDevices.addEventListener('devicechange', populateAudioOutputDevices)");
  });
});
