#pragma once

// ─── Shared WASM bridge state ───
// WasmBridge.cpp was a ~44KB god object: DSP init/process, the parameter
// dispatch, preset/bank/SysEx management, envelope access and the Modern
// performance macros all lived in one translation unit. The bridge is split
// into focused modules (WasmParams / WasmPresets / WasmBank / WasmEnvelopes /
// WasmMacros + a slim WasmBridge core); this header owns the state they all
// share.
//
// C++20 `inline` variables give every TU that includes this header the SAME
// instance (no need to pick a "defining" TU), and the inline function
// `getMidiProcessor()` shares its function-local static across TUs the same
// way. Behaviour is identical to the previous file-scope statics.

#include "WasmBridge.h"
#include "../Core/VoiceManager.h"
#include "../Core/AudioThreadSnapshot.h"
#include "../DSP/Effects/EffectsChain.h"
#include "../State/ParameterRegistry.gen.h"
#include "../MIDI/MIDIProcessor.h"
#include <cmath>
#include <cstring>
#include <algorithm>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

struct WasmEnvelope {
    float rates[8];
    float levels[8];
    int sustainPoint;
    int endPoint;
};

struct WasmPreset {
    std::string name;
    std::map<std::string, float> parameters;
    WasmEnvelope pitchEnv, dcwEnv, dcaEnv;
    WasmEnvelope pitchEnv2, dcwEnv2, dcaEnv2;
};

inline CZ101::Core::VoiceManager gVoiceManager;
inline CZ101::DSP::Effects::EffectsChain gEffectsChain;
inline CZ101::Core::ParameterSnapshot gWasmSnapshot;
inline bool gEffectsPrepared = false;
inline bool gInitialized = false;

// Lazily-created MIDI processor (constructed on first use inside wasm_init,
// after gVoiceManager). Handles channel filtering (MIDI_CH), pitch bend range
// scaling (PITCH_BEND_RANGE), sustain, mod wheel and the hardcoded CC->param
// mappings shared with the native plugin.
inline CZ101::MIDI::MIDIProcessor& getMidiProcessor() {
    static CZ101::MIDI::MIDIProcessor midiProcessor(gVoiceManager);
    return midiProcessor;
}
inline CZ101::Core::Voice::ModulationMatrix gModulationMatrix;
inline std::map<std::string, float> gWasmParams;
inline std::vector<WasmPreset> gPresets(64);
inline int gCurrentPresetIndex = 0;
inline int gKeyTranspose = 0; // KEY_TRANSPOSE (-12..+12 semitones)
inline int gOctave = 0;           // OCTAVE from SysEx PFLAG (-1..+1 octave = ±12 semitones)

// Parameter state variables for WASM DSP synchronization
inline float gOsc1Level = 1.0f;
inline float gOsc2Level = 1.0f;
inline int gOsc1Wave = 0;
inline int gOsc1Wave2 = 8; // None
inline int gOsc1Window = 0;
inline int gOsc2Wave = 0;
inline int gOsc2Wave2 = 8; // None
inline int gOsc2Window = 0;
inline int gLineSelect = 2; // Line 1+1'
inline float gLineMix = 0.5f;

// Macro variables
inline float gMacroBrilliance = 0.5f;
inline float gMacroTone = 0.5f;
inline float gMacroSpace = 0.0f;
inline float gBaseReverbMix = 0.2f; // REVERB_MIX without the Space macro contribution
inline bool gArpEnabled = false;     // raw ARP_ENABLED flag (gated by opMode at apply time)

// Reproduce juce::NormalisableRange::convertFrom0to1 / convertTo0to1 for a
// non-symmetric skew factor (spec->skew, 1.0 = linear). This keeps the WASM
// standalone engine bit-consistent with the native APVTS, where e.g.
// MODERN_LPF_CUTOFF uses skew 0.3 (20..20000 Hz, logarithmic feel).
inline float rawFromNormalized(const CZ101::ParameterRegistry::ParameterSpec* spec, float norm) {
    const float clamped = std::clamp(norm, 0.0f, 1.0f);
    if (spec == nullptr) return clamped;
    const float range = spec->maxValue - spec->minValue;
    if (range <= 0.0f) return spec->minValue;
    float proportion = clamped;
    if (spec->skew > 0.0f && spec->skew != 1.0f)
        proportion = std::pow(clamped, 1.0f / spec->skew);
    return spec->minValue + proportion * range;
}

inline float normalizedFromRaw(const CZ101::ParameterRegistry::ParameterSpec* spec, float raw) {
    if (spec == nullptr) return 0.0f;
    const float range = spec->maxValue - spec->minValue;
    if (range <= 0.0f) return 0.0f;
    float proportion = std::clamp((raw - spec->minValue) / range, 0.0f, 1.0f);
    if (spec->skew > 0.0f && spec->skew != 1.0f)
        proportion = std::pow(proportion, spec->skew);
    return proportion;
}

// ─── Cross-module helpers (defined in the module that owns them) ───
// DSP layer (WasmParams.cpp): re-derives osc levels from Line Select/Mix.
void updateWasmDSP();

// Modern performance macros (WasmMacros.cpp). Each helper is re-applied
// whenever its inputs or the operation mode change.
void applyBrilliance() noexcept;
void applyTone() noexcept;
void applyMacroSpace() noexcept;
void applyArpState() noexcept;
void pushModulationMatrix() noexcept;

// Preset helpers (WasmPresets.cpp).
void initWasmEnvelope(WasmEnvelope& env);
void initWasmPitchEnvelope(WasmEnvelope& env);
void initWasmPresets();
WasmPreset convertToWasmPreset(const CZ101::State::Preset& src);
void applyWasmPreset(const WasmPreset& preset);
