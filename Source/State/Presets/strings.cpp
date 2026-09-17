#include "../PresetManager.h"
#include "../ParameterIDs.h"
#include "preset_helpers.h"

namespace CZ101 {
namespace State {

void PresetManager::createStringPreset()
{
    Preset p;
    p.name = "Vintage Strings";
    initEnvelopes(p);
    
    // ===== OSCILLATORS (NORMALIZED) =====
    p.parameters[ParameterIDs::osc1Waveform.toStdString()] = 1.0f;      // Saw
    p.parameters[ParameterIDs::osc1Level.toStdString()] = 0.5f;         // âœ… 50% (normalized)
    p.parameters[ParameterIDs::osc2Waveform.toStdString()] = 1.0f;      // Saw
    p.parameters[ParameterIDs::osc2Level.toStdString()] = 0.5f;         // âœ… 50% (normalized)
    // Total: 0.5 + 0.5 = 1.0 âœ…
    
    p.parameters[ParameterIDs::osc2Detune.toStdString()] = 0.05f;      // Subtle +5 cents detune
    
    // ===== ENVELOPES (Explicit for 8-stage engine) =====
    // Pitch: Flat
    p.pitchEnv.rates[0] = 0.99f; p.pitchEnv.levels[0] = 0.5f;
    p.pitchEnv.sustainPoint = 0; p.pitchEnv.endPoint = 0;

    // DCW: Slow Bow
    p.dcwEnv.rates[0] = 0.3f;  p.dcwEnv.levels[0] = 1.0f;  // Slow Attack
    p.dcwEnv.rates[1] = 0.99f; p.dcwEnv.levels[1] = 0.7f;  // Decay/Sustain
    p.dcwEnv.rates[2] = 0.99f; p.dcwEnv.levels[2] = 0.7f;  // Sustain
    p.dcwEnv.rates[3] = 0.4f;  p.dcwEnv.levels[3] = 0.0f;  // Slow Release
    p.dcwEnv.sustainPoint = 2; p.dcwEnv.endPoint = 3;

    // DCA: Slow Bow
    p.dcaEnv.rates[0] = 0.3f;  p.dcaEnv.levels[0] = 1.0f;  // Slow Attack
    p.dcaEnv.rates[1] = 0.99f; p.dcaEnv.levels[1] = 0.8f;  // Decay/Sustain
    p.dcaEnv.rates[2] = 0.99f; p.dcaEnv.levels[2] = 0.8f;  // Sustain
    p.dcaEnv.rates[3] = 0.4f;  p.dcaEnv.levels[3] = 0.0f;  // Slow Release
    p.dcaEnv.sustainPoint = 2; p.dcaEnv.endPoint = 3;

    // Legacy Params for display
    p.parameters[ParameterIDs::dcwAttack.toStdString()] = 0.3f; p.parameters[ParameterIDs::dcwDecay.toStdString()] = 0.4f; p.parameters[ParameterIDs::dcwSustain.toStdString()] = 0.7f; p.parameters[ParameterIDs::dcwRelease.toStdString()] = 0.5f;
    p.parameters[ParameterIDs::dcaAttack.toStdString()] = 0.4f; p.parameters[ParameterIDs::dcaDecay.toStdString()] = 0.3f; p.parameters[ParameterIDs::dcaSustain.toStdString()] = 0.8f; p.parameters[ParameterIDs::dcaRelease.toStdString()] = 0.6f;
    
    // ===== FILTER =====
    p.parameters[ParameterIDs::lpfCutoff.toStdString()] = 8000.0f;   // Open
    p.parameters[ParameterIDs::lpfReso.toStdString()] = 0.3f;   // 30% Q
    
    // ===== LFO (VIBRATO) =====
    p.parameters[ParameterIDs::lfoRate.toStdString()] = 4.5f;           // 4.5 Hz
    p.parameters[ParameterIDs::lfoDepth.toStdString()] = 0.0f;          // Clean unmodulated base pitch
    
    // ===== EFFECTS =====
    p.parameters[ParameterIDs::delayTime.toStdString()] = 0.25f;        // âœ… 250ms
    p.parameters[ParameterIDs::delayFeedback.toStdString()] = 0.4f;     // 40%
    p.parameters[ParameterIDs::delayMix.toStdString()] = 0.3f;          // âœ… 30% wet (longer tail)
    
    p.parameters[ParameterIDs::chorusRate.toStdString()] = 0.6f;        // 0.6 Hz
    p.parameters[ParameterIDs::chorusDepth.toStdString()] = 3.0f;       // 3ms
    p.parameters[ParameterIDs::chorusMix.toStdString()] = 0.15f;        // âœ… 15% light chorus
    
    p.parameters[ParameterIDs::reverbSize.toStdString()] = 0.7f;        // Large room
    p.parameters[ParameterIDs::reverbMix.toStdString()] = 0.4f;         // âœ… 40% wet (lush)
    
    p.parameters[ParameterIDs::hardSync.toStdString()] = 0.0f;
    p.parameters[ParameterIDs::lineMod.toStdString()] = 0.0f;
    p.parameters[ParameterIDs::glideTime.toStdString()] = 0.0f;
    
    presets.push_back(p);
}

void PresetManager::createBrassPreset()
{
    Preset p;
    p.name = "Synth Brass";
    initEnvelopes(p);
    
    p.parameters[ParameterIDs::osc1Waveform.toStdString()] = 1.0f; p.parameters[ParameterIDs::osc1Level.toStdString()] = 1.0f;
    p.parameters[ParameterIDs::osc2Waveform.toStdString()] = 3.0f; p.parameters[ParameterIDs::osc2Level.toStdString()] = 0.6f; // Triangle for body
    p.parameters[ParameterIDs::osc2Detune.toStdString()] = 7.0f; // Slight detune
    
    // Pitch Envelope (Brass Attack: slight drop-up)
    // Stage 0: Fast drop to slightly fla (-2 semitones approx)
    p.pitchEnv.rates[0] = 0.9f; p.pitchEnv.levels[0] = 0.48f; 
    // Stage 1: Rise to slightly sharp (overshoot)
    p.pitchEnv.rates[1] = 0.6f; p.pitchEnv.levels[1] = 0.52f;
    // Stage 2: Settle to Unison
    p.pitchEnv.rates[2] = 0.4f; p.pitchEnv.levels[2] = 0.5f;
    // Stage 3: Sustain at Unison
    p.pitchEnv.rates[3] = 0.99f;p.pitchEnv.levels[3] = 0.5f;
    p.pitchEnv.sustainPoint = 3;
    p.pitchEnv.endPoint = 3;

    // DCW (Brass Swell)
    // Stage 0: Sharp attack
    p.dcwEnv.rates[0] = 0.85f; p.dcwEnv.levels[0] = 0.9f; 
    // Stage 1: Decay slightly to body
    p.dcwEnv.rates[1] = 0.7f;  p.dcwEnv.levels[1] = 0.7f;
    // Stage 2: Swell up a bit (breath)
    p.dcwEnv.rates[2] = 0.4f;  p.dcwEnv.levels[2] = 0.85f;
    // Stage 3: Sustain
    p.dcwEnv.rates[3] = 0.99f; p.dcwEnv.levels[3] = 0.85f;
    p.dcwEnv.sustainPoint = 3;
    p.dcwEnv.endPoint = 4; // Use stage 4 for release
    
    // Stage 4: Release
    p.dcwEnv.rates[4] = 0.6f; p.dcwEnv.levels[4] = 0.0f;
    
    // DCA (Standard ADSR-ish)
    p.dcaEnv.rates[0] = 0.85f; p.dcaEnv.levels[0] = 1.0f;
    p.dcaEnv.rates[1] = 0.7f;  p.dcaEnv.levels[1] = 0.9f;
    p.dcaEnv.rates[2] = 0.99f; p.dcaEnv.levels[2] = 0.9f;
    p.dcaEnv.rates[3] = 0.6f;  p.dcaEnv.levels[3] = 0.0f;
    p.dcaEnv.sustainPoint = 2;
    p.dcaEnv.endPoint = 3;
    
    // UI Params (Approximate for display)
    p.parameters[ParameterIDs::dcwAttack.toStdString()] = 0.2f; p.parameters[ParameterIDs::dcwDecay.toStdString()] = 0.3f; p.parameters[ParameterIDs::dcwSustain.toStdString()] = 0.8f; p.parameters[ParameterIDs::dcwRelease.toStdString()] = 0.4f;
    p.parameters[ParameterIDs::dcaAttack.toStdString()] = 0.1f; p.parameters[ParameterIDs::dcaDecay.toStdString()] = 0.2f; p.parameters[ParameterIDs::dcaSustain.toStdString()] = 0.9f; p.parameters[ParameterIDs::dcaRelease.toStdString()] = 0.4f;

    // Filter
    p.parameters[ParameterIDs::lpfCutoff.toStdString()] = 5000.0f;
    p.parameters[ParameterIDs::lpfReso.toStdString()] = 0.6f;

    // LFO
    p.parameters[ParameterIDs::lfoRate.toStdString()] = 0.5f;
    
    // Effects
    p.parameters[ParameterIDs::delayTime.toStdString()] = 0.0f; p.parameters[ParameterIDs::delayFeedback.toStdString()] = 0.0f; p.parameters[ParameterIDs::delayMix.toStdString()] = 0.0f;
    p.parameters[ParameterIDs::reverbSize.toStdString()] = 0.6f; p.parameters[ParameterIDs::reverbMix.toStdString()] = 0.3f;
    
    p.parameters[ParameterIDs::hardSync.toStdString()] = 0.0f;
    p.parameters[ParameterIDs::lineMod.toStdString()] = 0.0f;
    p.parameters[ParameterIDs::glideTime.toStdString()] = 0.0f;

    presets.push_back(p);
}


} // namespace State
} // namespace CZ101
