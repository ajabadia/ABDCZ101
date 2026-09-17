import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const ROOT_DIR = path.resolve(__dirname, '..');
const SCHEMAS_DIR = path.join(ROOT_DIR, 'schemas');
const CPP_STATE_DIR = path.join(ROOT_DIR, 'Source', 'State');
const WEBUI_CONTRACTS_DIR = path.join(ROOT_DIR, 'WebUI', 'src', 'contracts');

// NOTE: Values below are the CONTRACT for the WebUI <-> C++/WASM bridge.
// They must match Source/State/Parameters.cpp (the real APVTS layout) exactly.
// Regenerated artifacts: schemas/parameter-registry.data.json,
// Source/State/ParameterRegistry.gen.{h,cpp}, WebUI/src/contracts/registry.gen.js
const parameters = [
  // --- Oscillators & Line Selection ---
  { id: "LINE_SELECT", name: "Line Select", category: "Oscillators", type: "choice", min: 0, max: 3, default: 2, choices: ["Line 1", "Line 2", "Line 1+1'", "Line 1+2"] },
  { id: "OSC1_WAVEFORM", name: "Osc 1 Waveform 1", category: "Oscillators", type: "choice", min: 0, max: 7, default: 0, choices: ["Sawtooth", "Square", "Pulse", "Double Sine", "Saw Pulse", "Reso 1", "Reso 2", "Reso 3"] },
  { id: "OSC1_WAVEFORM2", name: "Osc 1 Waveform 2", category: "Oscillators", type: "choice", min: 0, max: 8, default: 0, choices: ["None", "Sawtooth", "Square", "Pulse", "Double Sine", "Saw Pulse", "Reso 1", "Reso 2", "Reso 3"] },
  { id: "OSC1_WINDOW", name: "Osc 1 Window", category: "Oscillators", type: "choice", min: 0, max: 5, default: 0, choices: ["None", "Saw", "Triangle", "Trapezoid", "Pulse", "Dbl Saw"] },
  { id: "OSC1_LEVEL", name: "Osc 1 Level", category: "Oscillators", type: "float", min: 0.0, max: 1.0, default: 1.0, step: 0.01 },
  { id: "OSC2_WAVEFORM", name: "Osc 2 Waveform 1", category: "Oscillators", type: "choice", min: 0, max: 7, default: 0, choices: ["Sawtooth", "Square", "Pulse", "Double Sine", "Saw Pulse", "Reso 1", "Reso 2", "Reso 3"] },
  { id: "OSC2_WAVEFORM2", name: "Osc 2 Waveform 2", category: "Oscillators", type: "choice", min: 0, max: 8, default: 0, choices: ["None", "Sawtooth", "Square", "Pulse", "Double Sine", "Saw Pulse", "Reso 1", "Reso 2", "Reso 3"] },
  { id: "OSC2_WINDOW", name: "Osc 2 Window", category: "Oscillators", type: "choice", min: 0, max: 5, default: 0, choices: ["None", "Saw", "Triangle", "Trapezoid", "Pulse", "Dbl Saw"] },
  { id: "OSC2_LEVEL", name: "Osc 2 Level", category: "Oscillators", type: "float", min: 0.0, max: 1.0, default: 0.0, step: 0.01 },
  { id: "OSC2_DETUNE", name: "Osc 2 Detune", category: "Oscillators", type: "float", min: -12.0, max: 12.0, default: 0.0, step: 0.1, unit: "st" },
  { id: "DETUNE_OCT", name: "Detune Octave", category: "Oscillators", type: "int", min: -3, max: 3, default: 0, step: 1 },
  { id: "DETUNE_COARSE", name: "Detune Coarse", category: "Oscillators", type: "int", min: -12, max: 12, default: 0, step: 1 },
  { id: "DETUNE_FINE", name: "Detune Fine", category: "Oscillators", type: "int", min: -50, max: 50, default: 0, step: 1, unit: "cents" },
  { id: "LINE_MIX", name: "Line Mix", category: "Oscillators", type: "float", min: 0.0, max: 1.0, default: 0.5, step: 0.01 },
  { id: "HARD_SYNC", name: "Hard Sync", category: "Oscillators", type: "bool", min: 0, max: 1, default: 0 },
  { id: "LINE_MODULATION", name: "Line Modulation", category: "Oscillators", type: "choice", min: 0, max: 5, default: 0, choices: ["Off", "Ring 1", "Noise 1", "Ring 2", "Ring 3", "Noise 2"] },
  { id: "MOD_SPECIAL", name: "Mod Special (Mute Line 1)", category: "Oscillators", type: "bool", min: 0, max: 1, default: 0 },
  { id: "GLIDE", name: "Glide Time", category: "Oscillators", type: "float", min: 0.0, max: 1.0, default: 0.0, step: 0.01, unit: "s" },

  // --- LFO / Vibrato ---
  { id: "LFO_WAVE", name: "LFO Waveform", category: "LFO", type: "choice", min: 0, max: 3, default: 0, choices: ["Triangle", "Saw Up", "Saw Down", "Square"] },
  { id: "LFO_RATE", name: "LFO Rate", category: "LFO", type: "float", min: 0.1, max: 30.0, default: 5.0, step: 0.1, unit: "Hz" },
  { id: "LFO_DEPTH", name: "LFO Depth", category: "LFO", type: "float", min: 0.0, max: 1.0, default: 0.0, step: 0.01 },
  { id: "LFO_DELAY", name: "LFO Delay", category: "LFO", type: "float", min: 0.0, max: 2.0, default: 0.0, step: 0.01, unit: "s" },

  // --- DCA / DCW Envelopes ---
  { id: "DCA_ATTACK", name: "DCA Attack", category: "Envelopes", type: "float", min: 0.0, max: 10.0, default: 0.0, step: 0.01, unit: "s" },
  { id: "DCA_DECAY", name: "DCA Decay", category: "Envelopes", type: "float", min: 0.0, max: 10.0, default: 0.0, step: 0.01, unit: "s" },
  { id: "DCA_SUSTAIN", name: "DCA Sustain", category: "Envelopes", type: "float", min: 0.0, max: 1.0, default: 1.0, step: 0.01 },
  { id: "DCA_RELEASE", name: "DCA Release", category: "Envelopes", type: "float", min: 0.0, max: 10.0, default: 0.0, step: 0.01, unit: "s" },
  { id: "DCW_ATTACK", name: "DCW Attack", category: "Envelopes", type: "float", min: 0.0, max: 10.0, default: 0.0, step: 0.01, unit: "s" },
  { id: "DCW_DECAY", name: "DCW Decay", category: "Envelopes", type: "float", min: 0.0, max: 10.0, default: 0.0, step: 0.01, unit: "s" },
  { id: "DCW_SUSTAIN", name: "DCW Sustain", category: "Envelopes", type: "float", min: 0.0, max: 1.0, default: 1.0, step: 0.01 },
  { id: "DCW_RELEASE", name: "DCW Release", category: "Envelopes", type: "float", min: 0.0, max: 10.0, default: 0.0, step: 0.01, unit: "s" },

  // --- Modulation Matrix ---
  { id: "MOD_VELO_DCW", name: "Velocity -> DCW", category: "Modulation", type: "float", min: 0.0, max: 1.0, default: 0.0, step: 0.01 },
  { id: "MOD_VELO_DCA", name: "Velocity -> DCA", category: "Modulation", type: "float", min: 0.0, max: 1.0, default: 1.0, step: 0.01 },
  { id: "MOD_WHEEL_DCW", name: "ModWheel -> DCW", category: "Modulation", type: "float", min: 0.0, max: 1.0, default: 0.0, step: 0.01 },
  { id: "MOD_WHEEL_LFORATE", name: "ModWheel -> LFO Rate", category: "Modulation", type: "float", min: 0.0, max: 1.0, default: 0.0, step: 0.01 },
  { id: "MOD_WHEEL_VIB", name: "ModWheel -> Vibrato", category: "Modulation", type: "float", min: 0.0, max: 1.0, default: 0.0, step: 0.01 },
  { id: "MOD_AT_DCW", name: "Aftertouch -> DCW", category: "Modulation", type: "float", min: 0.0, max: 1.0, default: 0.0, step: 0.01 },
  { id: "MOD_AT_VIB", name: "Aftertouch -> Vibrato", category: "Modulation", type: "float", min: 0.0, max: 1.0, default: 0.0, step: 0.01 },
  { id: "KEY_TRACK_DCW", name: "Key Track -> DCW", category: "Modulation", type: "float", min: 0.0, max: 1.0, default: 0.0, step: 0.01 },
  { id: "KEY_TRACK_PITCH", name: "Key Track -> Pitch", category: "Modulation", type: "float", min: 0.0, max: 1.0, default: 1.0, step: 0.01 },
  { id: "KEY_FOLLOW_DCO", name: "Key Follow DCO", category: "Modulation", type: "choice", min: 0, max: 2, default: 2, choices: ["OFF", "FIX", "VAR"] },
  { id: "KEY_FOLLOW_DCW", name: "Key Follow DCW", category: "Modulation", type: "choice", min: 0, max: 2, default: 0, choices: ["OFF", "FIX", "VAR"] },
  { id: "KEY_FOLLOW_DCA", name: "Key Follow DCA", category: "Modulation", type: "choice", min: 0, max: 2, default: 0, choices: ["OFF", "FIX", "VAR"] },

  // --- CZ-1 Velocity Sensitivities (Per-Line) ---
  { id: "LINE1_VELO_PITCH", name: "Line 1 Velo -> Pitch", category: "Modulation", type: "float", min: 0.0, max: 15.0, default: 0.0, step: 1.0 },
  { id: "LINE1_VELO_DCW", name: "Line 1 Velo -> DCW", category: "Modulation", type: "float", min: 0.0, max: 15.0, default: 0.0, step: 1.0 },
  { id: "LINE1_VELO_DCA", name: "Line 1 Velo -> DCA", category: "Modulation", type: "float", min: 0.0, max: 15.0, default: 0.0, step: 1.0 },
  { id: "LINE2_VELO_PITCH", name: "Line 2 Velo -> Pitch", category: "Modulation", type: "float", min: 0.0, max: 15.0, default: 0.0, step: 1.0 },
  { id: "LINE2_VELO_DCW", name: "Line 2 Velo -> DCW", category: "Modulation", type: "float", min: 0.0, max: 15.0, default: 0.0, step: 1.0 },
  { id: "LINE2_VELO_DCA", name: "Line 2 Velo -> DCA", category: "Modulation", type: "float", min: 0.0, max: 15.0, default: 0.0, step: 1.0 },

  // --- Key Follow per-line (CZ-1 SYSEX, 0-9) ---
  { id: "LINE1_KF_PITCH", name: "Line 1 KF Pitch", category: "Modulation", type: "int", min: 0, max: 9, default: 0, step: 1 },
  { id: "LINE1_KF_DCW", name: "Line 1 KF DCW", category: "Modulation", type: "int", min: 0, max: 9, default: 0, step: 1 },
  { id: "LINE1_KF_DCA", name: "Line 1 KF DCA", category: "Modulation", type: "int", min: 0, max: 9, default: 0, step: 1 },
  { id: "LINE2_KF_PITCH", name: "Line 2 KF Pitch", category: "Modulation", type: "int", min: 0, max: 9, default: 0, step: 1 },
  { id: "LINE2_KF_DCW", name: "Line 2 KF DCW", category: "Modulation", type: "int", min: 0, max: 9, default: 0, step: 1 },
  { id: "LINE2_KF_DCA", name: "Line 2 KF DCA", category: "Modulation", type: "int", min: 0, max: 9, default: 0, step: 1 },

  // --- Free Modulation Matrix (ABDEEP-style, Modern-only) ---
  // 8 slots, each Source (0..8) -> Dest (0..5) -> Depth (bipolar -1..1). The
  // DSP applies them as additive contributions to the matching destination.
  { id: "MOD_SLOT_1_SRC", name: "Mod Slot 1 Source", category: "Modulation", type: "choice", min: 0, max: 11, default: 0, choices: ["None", "Velocity", "Mod Wheel", "Aftertouch", "Key Track", "LFO", "Env DCW", "Env DCA", "Env Pitch", "Pitch Bend", "Noise", "Authentic Key Track"] },
  { id: "MOD_SLOT_1_DEST", name: "Mod Slot 1 Destination", category: "Modulation", type: "choice", min: 0, max: 7, default: 0, choices: ["None", "DCW", "DCA", "Pitch", "Vibrato", "LFO Rate", "Osc2 Detune", "Pan"] },
  { id: "MOD_SLOT_1_DEPTH", name: "Mod Slot 1 Depth", category: "Modulation", type: "float", min: -1.0, max: 1.0, default: 0.0, step: 0.01 },
  { id: "MOD_SLOT_2_SRC", name: "Mod Slot 2 Source", category: "Modulation", type: "choice", min: 0, max: 11, default: 0, choices: ["None", "Velocity", "Mod Wheel", "Aftertouch", "Key Track", "LFO", "Env DCW", "Env DCA", "Env Pitch", "Pitch Bend", "Noise", "Authentic Key Track"] },
  { id: "MOD_SLOT_2_DEST", name: "Mod Slot 2 Destination", category: "Modulation", type: "choice", min: 0, max: 7, default: 0, choices: ["None", "DCW", "DCA", "Pitch", "Vibrato", "LFO Rate", "Osc2 Detune", "Pan"] },
  { id: "MOD_SLOT_2_DEPTH", name: "Mod Slot 2 Depth", category: "Modulation", type: "float", min: -1.0, max: 1.0, default: 0.0, step: 0.01 },
  { id: "MOD_SLOT_3_SRC", name: "Mod Slot 3 Source", category: "Modulation", type: "choice", min: 0, max: 11, default: 0, choices: ["None", "Velocity", "Mod Wheel", "Aftertouch", "Key Track", "LFO", "Env DCW", "Env DCA", "Env Pitch", "Pitch Bend", "Noise", "Authentic Key Track"] },
  { id: "MOD_SLOT_3_DEST", name: "Mod Slot 3 Destination", category: "Modulation", type: "choice", min: 0, max: 7, default: 0, choices: ["None", "DCW", "DCA", "Pitch", "Vibrato", "LFO Rate", "Osc2 Detune", "Pan"] },
  { id: "MOD_SLOT_3_DEPTH", name: "Mod Slot 3 Depth", category: "Modulation", type: "float", min: -1.0, max: 1.0, default: 0.0, step: 0.01 },
  { id: "MOD_SLOT_4_SRC", name: "Mod Slot 4 Source", category: "Modulation", type: "choice", min: 0, max: 11, default: 0, choices: ["None", "Velocity", "Mod Wheel", "Aftertouch", "Key Track", "LFO", "Env DCW", "Env DCA", "Env Pitch", "Pitch Bend", "Noise", "Authentic Key Track"] },
  { id: "MOD_SLOT_4_DEST", name: "Mod Slot 4 Destination", category: "Modulation", type: "choice", min: 0, max: 7, default: 0, choices: ["None", "DCW", "DCA", "Pitch", "Vibrato", "LFO Rate", "Osc2 Detune", "Pan"] },
  { id: "MOD_SLOT_4_DEPTH", name: "Mod Slot 4 Depth", category: "Modulation", type: "float", min: -1.0, max: 1.0, default: 0.0, step: 0.01 },
  { id: "MOD_SLOT_5_SRC", name: "Mod Slot 5 Source", category: "Modulation", type: "choice", min: 0, max: 11, default: 0, choices: ["None", "Velocity", "Mod Wheel", "Aftertouch", "Key Track", "LFO", "Env DCW", "Env DCA", "Env Pitch", "Pitch Bend", "Noise", "Authentic Key Track"] },
  { id: "MOD_SLOT_5_DEST", name: "Mod Slot 5 Destination", category: "Modulation", type: "choice", min: 0, max: 7, default: 0, choices: ["None", "DCW", "DCA", "Pitch", "Vibrato", "LFO Rate", "Osc2 Detune", "Pan"] },
  { id: "MOD_SLOT_5_DEPTH", name: "Mod Slot 5 Depth", category: "Modulation", type: "float", min: -1.0, max: 1.0, default: 0.0, step: 0.01 },
  { id: "MOD_SLOT_6_SRC", name: "Mod Slot 6 Source", category: "Modulation", type: "choice", min: 0, max: 11, default: 0, choices: ["None", "Velocity", "Mod Wheel", "Aftertouch", "Key Track", "LFO", "Env DCW", "Env DCA", "Env Pitch", "Pitch Bend", "Noise", "Authentic Key Track"] },
  { id: "MOD_SLOT_6_DEST", name: "Mod Slot 6 Destination", category: "Modulation", type: "choice", min: 0, max: 7, default: 0, choices: ["None", "DCW", "DCA", "Pitch", "Vibrato", "LFO Rate", "Osc2 Detune", "Pan"] },
  { id: "MOD_SLOT_6_DEPTH", name: "Mod Slot 6 Depth", category: "Modulation", type: "float", min: -1.0, max: 1.0, default: 0.0, step: 0.01 },
  { id: "MOD_SLOT_7_SRC", name: "Mod Slot 7 Source", category: "Modulation", type: "choice", min: 0, max: 11, default: 0, choices: ["None", "Velocity", "Mod Wheel", "Aftertouch", "Key Track", "LFO", "Env DCW", "Env DCA", "Env Pitch", "Pitch Bend", "Noise", "Authentic Key Track"] },
  { id: "MOD_SLOT_7_DEST", name: "Mod Slot 7 Destination", category: "Modulation", type: "choice", min: 0, max: 7, default: 0, choices: ["None", "DCW", "DCA", "Pitch", "Vibrato", "LFO Rate", "Osc2 Detune", "Pan"] },
  { id: "MOD_SLOT_7_DEPTH", name: "Mod Slot 7 Depth", category: "Modulation", type: "float", min: -1.0, max: 1.0, default: 0.0, step: 0.01 },
  { id: "MOD_SLOT_8_SRC", name: "Mod Slot 8 Source", category: "Modulation", type: "choice", min: 0, max: 11, default: 0, choices: ["None", "Velocity", "Mod Wheel", "Aftertouch", "Key Track", "LFO", "Env DCW", "Env DCA", "Env Pitch", "Pitch Bend", "Noise", "Authentic Key Track"] },
  { id: "MOD_SLOT_8_DEST", name: "Mod Slot 8 Destination", category: "Modulation", type: "choice", min: 0, max: 7, default: 0, choices: ["None", "DCW", "DCA", "Pitch", "Vibrato", "LFO Rate", "Osc2 Detune", "Pan"] },
  { id: "MOD_SLOT_8_DEPTH", name: "Mod Slot 8 Depth", category: "Modulation", type: "float", min: -1.0, max: 1.0, default: 0.0, step: 0.01 },

  // --- Modern Filters & Effects ---
  // Note: MODERN_LPF_CUTOFF / MODERN_HPF_CUTOFF use NormalisableRange with a
  // non-symmetric skew factor of 0.3 in Parameters.cpp (the real APVTS). The
  // WebUI bridge must reproduce JUCE's convertTo0to1/convertFrom0to1 formulas:
  //   raw->norm: pow(prop, skew)      norm->raw: pow(norm, 1/skew)
  { id: "MODERN_LPF_CUTOFF", name: "LPF Cutoff", category: "Filter", type: "float", min: 20.0, max: 20000.0, default: 20000.0, step: 1.0, unit: "Hz", skew: 0.3 },
  { id: "MODERN_LPF_RESO", name: "LPF Resonance", category: "Filter", type: "float", min: 0.0, max: 1.0, default: 0.0, step: 0.01 },
  { id: "MODERN_HPF_CUTOFF", name: "HPF Cutoff", category: "Filter", type: "float", min: 20.0, max: 10000.0, default: 20.0, step: 1.0, unit: "Hz", skew: 0.3 },
  { id: "DRIVE_AMOUNT", name: "Drive Amount", category: "Effects", type: "float", min: 0.0, max: 1.0, default: 0.0, step: 0.01 },
  { id: "DRIVE_COLOR", name: "Drive Tone", category: "Effects", type: "float", min: 0.0, max: 1.0, default: 0.5, step: 0.01 },
  { id: "DRIVE_MIX", name: "Drive Mix", category: "Effects", type: "float", min: 0.0, max: 1.0, default: 0.0, step: 0.01 },
  { id: "CHORUS_RATE", name: "Chorus Rate", category: "Effects", type: "float", min: 0.1, max: 10.0, default: 0.5, step: 0.01, unit: "Hz" },
  { id: "CHORUS_DEPTH", name: "Chorus Depth", category: "Effects", type: "float", min: 0.0, max: 1.0, default: 0.2, step: 0.01 },
  { id: "CHORUS_MIX", name: "Chorus Mix", category: "Effects", type: "float", min: 0.0, max: 1.0, default: 0.3, step: 0.01 },
  { id: "DELAY_TIME", name: "Delay Time", category: "Effects", type: "float", min: 0.0, max: 2.0, default: 0.5, step: 0.01, unit: "s" },
  { id: "DELAY_FEEDBACK", name: "Delay Feedback", category: "Effects", type: "float", min: 0.0, max: 0.95, default: 0.3, step: 0.01 },
  { id: "DELAY_MIX", name: "Delay Mix", category: "Effects", type: "float", min: 0.0, max: 1.0, default: 0.0, step: 0.01 },
  { id: "REVERB_SIZE", name: "Reverb Room Size", category: "Effects", type: "float", min: 0.0, max: 1.0, default: 0.5, step: 0.01 },
  { id: "REVERB_MIX", name: "Reverb Mix", category: "Effects", type: "float", min: 0.0, max: 1.0, default: 0.2, step: 0.01 },

  // --- Performance Macros & System ---
  { id: "MACRO_BRILLIANCE", name: "Brilliance Macro", category: "Macros", type: "float", min: 0.0, max: 1.0, default: 0.5, step: 0.01 },
  { id: "MACRO_TONE", name: "Tone Macro", category: "Macros", type: "float", min: 0.0, max: 1.0, default: 0.5, step: 0.01 },
  { id: "MACRO_SPACE", name: "Space Macro", category: "Macros", type: "float", min: 0.0, max: 1.0, default: 0.0, step: 0.01 },
  { id: "MASTER_VOLUME", name: "Master Volume", category: "System", type: "float", min: 0.0, max: 1.0, default: 1.0, step: 0.01 },
  { id: "MIDI_CH", name: "MIDI Channel", category: "System", type: "int", min: 1, max: 16, default: 1, step: 1 },
  { id: "MASTER_TUNE", name: "Master Tuning", category: "System", type: "float", min: -50.0, max: 50.0, default: 0.0, step: 1.0, unit: "cents" },
  { id: "PITCH_BEND_RANGE", name: "Pitch Bend Range", category: "System", type: "int", min: 0, max: 12, default: 2, step: 1, unit: "st" },
  { id: "KEY_TRANSPOSE", name: "Transpose", category: "System", type: "int", min: -12, max: 12, default: 0, step: 1, unit: "st" },
  { id: "OPERATION_MODE", name: "Operation Mode", category: "System", type: "choice", min: 0, max: 3, default: 0, choices: ["Classic CZ-101", "Classic CZ-1", "Classic CZ-5000", "Modern"] },
  { id: "PROTECT_SWITCH", name: "Memory Protect", category: "System", type: "bool", min: 0, max: 1, default: 1 },
  { id: "OVERSAMPLING_QUALITY", name: "Oversampling Quality", category: "System", type: "choice", min: 0, max: 2, default: 0, choices: ["1x (Eco)", "2x (High)", "4x (Ultra)"] },
  { id: "HARDWARE_NOISE", name: "Hardware Noise Emulation", category: "System", type: "bool", min: 0, max: 1, default: 1 },
  { id: "ARP_ENABLED", name: "Arp Enabled", category: "Arpeggiator", type: "bool", min: 0, max: 1, default: 0 },
  { id: "ARP_LATCH", name: "Arp Latch", category: "Arpeggiator", type: "bool", min: 0, max: 1, default: 0 },
  { id: "ARP_RATE", name: "Arp Rate", category: "Arpeggiator", type: "choice", min: 0, max: 3, default: 2, choices: ["1/4", "1/8", "1/16", "1/32"] },
  { id: "ARP_BPM", name: "Arp Sync BPM", category: "Arpeggiator", type: "float", min: 40.0, max: 240.0, default: 120.0, step: 1.0, unit: "BPM" },
  { id: "ARP_GATE", name: "Arp Gate", category: "Arpeggiator", type: "float", min: 0.0, max: 1.0, default: 0.5, step: 0.01 },
  { id: "ARP_SWING", name: "Arp Swing", category: "Arpeggiator", type: "float", min: 0.0, max: 1.0, default: 0.0, step: 0.01 },
  { id: "ARP_SWING_MODE", name: "Arp Swing Mode", category: "Arpeggiator", type: "choice", min: 0, max: 2, default: 0, choices: ["Off", "1/8", "1/16"] },
  { id: "ARP_PATTERN", name: "Arp Pattern", category: "Arpeggiator", type: "choice", min: 0, max: 4, default: 0, choices: ["Up", "Down", "Up/Down", "Random", "As Played"] },
  { id: "ARP_OCTAVE", name: "Arp Octaves", category: "Arpeggiator", type: "int", min: 1, max: 4, default: 1, step: 1 },

  // --- Octave + System (missing from registry, present in APVTS) ---
  { id: "OCTAVE", name: "Octave", category: "Oscillators", type: "int", min: -1, max: 1, default: 0, step: 1 },
  { id: "BYPASS", name: "Bypass", category: "System", type: "bool", min: 0, max: 1, default: 0 },
  { id: "SYSTEM_PRG", name: "SysEx Data Interchange", category: "System", type: "bool", min: 0, max: 1, default: 0 }
];

const registryData = {
  schemaVersion: 1,
  pluginId: "CZ101Emulator",
  parameters: parameters
};

// Write parameter-registry.data.json
fs.mkdirSync(SCHEMAS_DIR, { recursive: true });
fs.writeFileSync(
  path.join(SCHEMAS_DIR, 'parameter-registry.data.json'),
  JSON.stringify(registryData, null, 2),
  'utf8'
);

// Generate ParameterRegistry.gen.h
fs.mkdirSync(CPP_STATE_DIR, { recursive: true });
const cppHeaderContent = `// Auto-generated by scripts/registry_generator.js - DO NOT EDIT MANUALLY
#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <string>

namespace CZ101 {
namespace ParameterRegistry {

enum class ParamType { Float, Int, Choice, Bool };

struct ParameterSpec {
    const char* id;
    const char* name;
    const char* category;
    ParamType type;
    float minValue;
    float maxValue;
    float defaultValue;
    float step;
    const char* unit;
    float skew; // Non-symmetric skew factor (1.0 = linear), mirrors juce::NormalisableRange
};

extern const std::vector<ParameterSpec>& getRegistry();
const ParameterSpec* findSpec(const juce::String& paramId);

} // namespace ParameterRegistry
} // namespace CZ101
`;

fs.writeFileSync(path.join(CPP_STATE_DIR, 'ParameterRegistry.gen.h'), cppHeaderContent, 'utf8');

function toCppFloat(val) {
  const s = val.toString();
  if (s.indexOf('.') === -1 && s.indexOf('e') === -1) {
    return s + '.0f';
  }
  return s + 'f';
}

// Generate ParameterRegistry.gen.cpp
const cppSourceContent = `// Auto-generated by scripts/registry_generator.js - DO NOT EDIT MANUALLY
#include "ParameterRegistry.gen.h"

namespace CZ101 {
namespace ParameterRegistry {

static const std::vector<ParameterSpec> g_registry = {
${parameters.map(p => {
  const typeEnum = p.type === 'float' ? 'ParamType::Float' : p.type === 'int' ? 'ParamType::Int' : p.type === 'choice' ? 'ParamType::Choice' : 'ParamType::Bool';
  const unit = p.unit ? `"${p.unit}"` : '""';
  const step = p.step || 0.0;
  const skew = p.skew || 1.0;
  return `    { "${p.id}", "${p.name}", "${p.category}", ${typeEnum}, ${toCppFloat(p.min)}, ${toCppFloat(p.max)}, ${toCppFloat(p.default)}, ${toCppFloat(step)}, ${unit}, ${toCppFloat(skew)} }`;
}).join(',\n')}
};

const std::vector<ParameterSpec>& getRegistry() {
    return g_registry;
}

const ParameterSpec* findSpec(const juce::String& paramId) {
    for (const auto& spec : g_registry) {
        if (paramId == spec.id) return &spec;
    }
    return nullptr;
}

} // namespace ParameterRegistry
} // namespace CZ101
`;

fs.writeFileSync(path.join(CPP_STATE_DIR, 'ParameterRegistry.gen.cpp'), cppSourceContent, 'utf8');

// Generate registry.gen.js
fs.mkdirSync(WEBUI_CONTRACTS_DIR, { recursive: true });
const jsRegistryContent = `// Auto-generated by scripts/registry_generator.js - DO NOT EDIT MANUALLY
export const PARAMETER_REGISTRY = ${JSON.stringify(registryData, null, 2)};

export const PARAM_MAP = new Map(
  PARAMETER_REGISTRY.parameters.map(p => [p.id, p])
);

export function rawToNormalized(paramId, rawValue) {
  const spec = PARAM_MAP.get(paramId);
  if (!spec) return 0.0;
  const range = spec.max - spec.min;
  if (range === 0) return 0.0;
  const proportion = Math.min(1.0, Math.max(0.0, (rawValue - spec.min) / range));
  // Reproduce juce::NormalisableRange::convertTo0to1 for a non-symmetric skew.
  // With skew < 1 (e.g. 0.3 on the filter cutoffs) the lower end of the range
  // fills more of the normalized space, exactly like the native APVTS.
  if (spec.skew && spec.skew !== 1.0) {
    return Math.pow(proportion, spec.skew);
  }
  return proportion;
}

export function normalizedToRaw(paramId, normalizedValue) {
  const spec = PARAM_MAP.get(paramId);
  if (!spec) return 0.0;
  const clampedNorm = Math.min(1.0, Math.max(0.0, normalizedValue));
  let proportion = clampedNorm;
  // Reproduce juce::NormalisableRange::convertFrom0to1 for a non-symmetric skew
  // (inverse of rawToNormalized: norm -> pow(norm, 1/skew)).
  if (spec.skew && spec.skew !== 1.0) {
    proportion = Math.pow(clampedNorm, 1.0 / spec.skew);
  }
  const raw = spec.min + proportion * (spec.max - spec.min);
  if (spec.type === 'int' || spec.type === 'choice' || spec.type === 'bool') {
    return Math.round(raw);
  }
  return raw;
}
`;

fs.writeFileSync(path.join(WEBUI_CONTRACTS_DIR, 'registry.gen.js'), jsRegistryContent, 'utf8');

console.log("✅ Successfully generated CZ-101 Parameter Registry artifacts:");
console.log("   - schemas/parameter-registry.data.json");
console.log("   - Source/State/ParameterRegistry.gen.h");
console.log("   - Source/State/ParameterRegistry.gen.cpp");
console.log("   - WebUI/src/contracts/registry.gen.js");
