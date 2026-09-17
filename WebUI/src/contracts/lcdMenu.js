// LCD menu definition, shared by app.js (the WebView2/web interface) and the
// test suite. The metadata is DERIVED from the parameter registry (PARAM_MAP)
// so ids/ranges/steps/choices can never drift from the APVTS (Parameters.cpp).
// `name` is the LCD label (hardware style, uppercase). Bool params are shown on
// the LCD as OFF/ON choices (their DOM control is a toggle button).
import { PARAM_MAP } from './registry.gen.js';

export const LCD_MENU_ITEMS = [
  // DCO
  { id: 'LINE_SELECT', name: 'LINE SELECT' },
  { id: 'OSC1_WAVEFORM', name: 'OSC1 WAVE 1' },
  { id: 'OSC1_WAVEFORM2', name: 'OSC1 WAVE 2' },
  { id: 'OSC1_WINDOW', name: 'OSC1 WINDOW' },
  { id: 'OSC1_LEVEL', name: 'OSC1 LEVEL' },
  { id: 'OSC2_WAVEFORM', name: 'OSC2 WAVE 1' },
  { id: 'OSC2_WAVEFORM2', name: 'OSC2 WAVE 2' },
  { id: 'OSC2_WINDOW', name: 'OSC2 WINDOW' },
  { id: 'OSC2_LEVEL', name: 'OSC2 LEVEL' },
  { id: 'OSC2_DETUNE', name: 'OSC2 DETUNE' },
  { id: 'DETUNE_OCT', name: 'DETUNE OCT' },
  { id: 'DETUNE_COARSE', name: 'DETUNE COARSE' },
  { id: 'DETUNE_FINE', name: 'DETUNE FINE' },
  { id: 'LINE_MIX', name: 'LINE MIX' },
  { id: 'HARD_SYNC', name: 'HARD SYNC' },
  { id: 'LINE_MODULATION', name: 'LINE MOD' },
  { id: 'MOD_SPECIAL', name: 'MOD SPECIAL' },
  { id: 'GLIDE', name: 'PORTAMENTO' },

  // VIB (LFO)
  { id: 'LFO_WAVE', name: 'LFO WAVE' },
  { id: 'LFO_RATE', name: 'LFO SPEED' },
  { id: 'LFO_DEPTH', name: 'LFO DEPTH' },
  { id: 'LFO_DELAY', name: 'LFO DELAY' },

  // MODULATION MATRIX (the routing matrix is a Modern-only addition of this
  // emulator; CZ-101/CZ-5000 only had velocity, mod wheel->vibrato and key
  // follow/track). The fixed ROUTES (Velo->DCW/DCA, Wheel->Vib, KeyTrack->Pitch,
  // ...) were removed: every one is replicable with the free 8-slot matrix in
  // the MOD drawer, so they no longer need dedicated controls. Only the Key
  // Follow MODES (OFF/FIX/VAR) stay — they are mode switches, not routes, and
  // the native LCDStateManager lists them under Section::MOD.
  { id: 'KEY_FOLLOW_DCO', name: 'KF DCO' },
  { id: 'KEY_FOLLOW_DCW', name: 'KF DCW' },
  { id: 'KEY_FOLLOW_DCA', name: 'KF DCA' },

  // SYSTEM (reachable from the SET button / Edit > Settings System Mode; the
  // native LCDStateManager Mode::SYSTEM lists only these parameters)
  { id: 'MIDI_CH', name: 'MIDI CHANNEL', system: true },
  { id: 'PITCH_BEND_RANGE', name: 'BEND RANGE', system: true },
  { id: 'KEY_TRANSPOSE', name: 'TRANSPOSE', system: true },
  { id: 'MASTER_VOLUME', name: 'MASTER VOLUME', system: true },
  { id: 'MASTER_TUNE', name: 'MASTER TUNE', system: true },
  { id: 'OPERATION_MODE', name: 'MODEL', system: true },
  { id: 'PROTECT_SWITCH', name: 'PROTECT', system: true },
  { id: 'HARDWARE_NOISE', name: 'NOISE', system: true },
  // WebUI-only UI settings (no engine parameter, no sysex): the LCD scroll
  // timing. Marked `uiSetting` so buildLcdMenu uses their own spec (they are
  // NOT in the parameter registry) and app.js edits the settings store instead
  // of the engine — like the native "Scroll" UI preferences.
  { id: 'LCD_SCROLL_SPEED', name: 'SCROLL SPEED', system: true, uiSetting: true, type: 'int', min: 40, max: 400, step: 10, unit: 'MS' },
  { id: 'LCD_SCROLL_PAUSE', name: 'SCROLL PAUSE', system: true, uiSetting: true, type: 'int', min: 0, max: 5000, step: 100, unit: 'MS' }
];

// Pure builder: returns the LCD menu for a given operation mode (0 = Classic
// 101, 1 = Classic 5000, 2 = Modern). Mirrors LCDStateManager::buildParameterList,
// which rebuilds its list when the operation mode changes.
export function buildLcdMenu(mode) {
  const isModern = mode === 3;
  const items = LCD_MENU_ITEMS
    .filter(item => !item.modernOnly || isModern)
    .map(item => {
      // WebUI-only UI settings (scroll timing) carry their own spec — they are
      // not registry parameters, so PARAM_MAP has nothing for them.
      if (item.uiSetting) {
        return { ...item, choices: null };
      }
      const spec = PARAM_MAP.get(item.id);
      if (!spec) {
        // Should never happen: kept as a safe fallback so a registry typo shows up
        // as "NOT BOUND" on the LCD instead of crashing.
        return { ...item, type: 'float', min: 0, max: 1, step: 0.01, unit: '', choices: null };
      }
      // bool params are rendered as OFF/ON choices on the LCD
      const type = spec.type === 'bool' ? 'choice' : spec.type;
      return {
        ...item,
        type,
        min: spec.min,
        max: spec.max,
        step: spec.step || 1,
        unit: spec.unit || '',
        choices: type === 'choice' ? spec.choices : null
      };
    });

  if (isModern) {
    // Parity with the native LCD "K-TRACK DCW/PIT" readouts (Section::MOD):
    // the free matrix can route the Authentic Key Track curve (source 11) to
    // any destination, so the LCD shows the active route like the hardware did.
    // This is a COMPUTED read-only entry (no registry param): its value is
    // derived from the matrix slots at display time by app.js, and it is only
    // reachable in Modern (the free matrix is Modern-only).
    items.push({
      id: 'AUTH_KTRACK',
      name: 'AUTH K-TRACK',
      type: 'computed',
      computed: true,
      modernOnly: true,
      min: 0, max: 1, step: 0.01, unit: '', choices: null
    });
  }
  return items;
}
