#include "../PresetManager.h"
#include "../ParameterIDs.h"
#include "preset_helpers.h"

namespace CZ101 {
namespace State {

void PresetManager::createBassPreset()
{
    Preset p;
    p.name = "CZ Bass";
    initEnvelopes(p);
    
    // ===== OSCILLATORS (NORMALIZED) =====
    p.parameters[ParameterIDs::osc1Waveform.toStdString()] = 1.0f;      // Saw
    p.parameters[ParameterIDs::osc1Level.toStdString()] = 0.6f;         // âœ… 60% (normalized)
    p.parameters[ParameterIDs::osc2Waveform.toStdString()] = 2.0f;      // Square
    p.parameters[ParameterIDs::osc2Level.toStdString()] = 0.4f;         // âœ… 40% (normalized)
    // Total: 0.6 + 0.4 = 1.0 âœ…
    
    p.parameters[ParameterIDs::osc2Detune.toStdString()] = -10.0f;      // -10 cents
    
    // ===== ENVELOPES (Explicit for 8-stage engine) =====
    // Pitch: Flat
    p.pitchEnv.rates[0] = 0.99f; p.pitchEnv.levels[0] = 0.5f;
    p.pitchEnv.sustainPoint = 0; p.pitchEnv.endPoint = 0;

    // DCW: Pluck (Filter)
    p.dcwEnv.rates[0] = 0.95f; p.dcwEnv.levels[0] = 0.9f;  // Attack
    p.dcwEnv.rates[1] = 0.5f;  p.dcwEnv.levels[1] = 0.2f;  // Decay to Sustain
    p.dcwEnv.rates[2] = 0.99f; p.dcwEnv.levels[2] = 0.2f;  // Sustain
    p.dcwEnv.rates[3] = 0.6f;  p.dcwEnv.levels[3] = 0.0f;  // Release
    p.dcwEnv.sustainPoint = 2; p.dcwEnv.endPoint = 3;

    // DCA: Pluck (Amp)
    p.dcaEnv.rates[0] = 0.99f; p.dcaEnv.levels[0] = 1.0f;  // Instant Attack
    p.dcaEnv.rates[1] = 0.6f;  p.dcaEnv.levels[1] = 0.5f;  // Decay
    p.dcaEnv.rates[2] = 0.99f; p.dcaEnv.levels[2] = 0.5f;  // Sustain
    p.dcaEnv.rates[3] = 0.6f;  p.dcaEnv.levels[3] = 0.0f;  // Release
    p.dcaEnv.sustainPoint = 2; p.dcaEnv.endPoint = 3;

    // Also set legacy params for display
    p.parameters[ParameterIDs::dcwAttack.toStdString()] = 0.01f; p.parameters[ParameterIDs::dcwDecay.toStdString()] = 0.2f; p.parameters[ParameterIDs::dcwSustain.toStdString()] = 0.2f; p.parameters[ParameterIDs::dcwRelease.toStdString()] = 0.1f;
    p.parameters[ParameterIDs::dcaAttack.toStdString()] = 0.001f; p.parameters[ParameterIDs::dcaDecay.toStdString()] = 0.2f; p.parameters[ParameterIDs::dcaSustain.toStdString()] = 0.5f; p.parameters[ParameterIDs::dcaRelease.toStdString()] = 0.15f;
    
    // ===== FILTER =====
    p.parameters[ParameterIDs::lpfCutoff.toStdString()] = 2000.0f;   // 2000 Hz
    p.parameters[ParameterIDs::lpfReso.toStdString()] = 0.5f;   // 50% Q
    
    // ===== LFO =====
    p.parameters[ParameterIDs::lfoRate.toStdString()] = 0.5f;           // 0.5 Hz
    p.parameters[ParameterIDs::lfoDepth.toStdString()] = 0.0f;          // No vibrato
    
    // ===== EFFECTS =====
    p.parameters[ParameterIDs::delayTime.toStdString()] = 0.3f;         // âœ… 300ms
    p.parameters[ParameterIDs::delayFeedback.toStdString()] = 0.3f;     // 30%
    p.parameters[ParameterIDs::delayMix.toStdString()] = 0.08f;         // âœ… 8% wet
    
    p.parameters[ParameterIDs::chorusRate.toStdString()] = 0.5f;        // 0.5 Hz
    p.parameters[ParameterIDs::chorusDepth.toStdString()] = 2.0f;       // 2ms
    p.parameters[ParameterIDs::chorusMix.toStdString()] = 0.0f;         // Off
    
    p.parameters[ParameterIDs::reverbSize.toStdString()] = 0.3f;        // Small room
    p.parameters[ParameterIDs::reverbMix.toStdString()] = 0.08f;        // âœ… 8% wet
    
    p.parameters[ParameterIDs::hardSync.toStdString()] = 0.0f;          // Off
    p.parameters[ParameterIDs::lineMod.toStdString()] = 0.0f;    // Off
    p.parameters[ParameterIDs::glideTime.toStdString()] = 0.0f;         // No portamento
    
    presets.push_back(p);
}

void PresetManager::createLeadPreset()
{
    Preset p;
    p.name = "Solo Lead";
    initEnvelopes(p);
    
    p.parameters[ParameterIDs::osc1Waveform.toStdString()] = 2.0f; p.parameters[ParameterIDs::osc1Level.toStdString()] = 1.0f;
    p.parameters[ParameterIDs::osc2Waveform.toStdString()] = 2.0f; p.parameters[ParameterIDs::osc2Level.toStdString()] = 0.6f;
    p.parameters[ParameterIDs::osc2Detune.toStdString()] = 0.0f;
    
    // DCW: Open
    p.dcwEnv.rates[0] = 0.99f; p.dcwEnv.levels[0] = 1.0f;
    p.dcwEnv.rates[1] = 0.99f; p.dcwEnv.levels[1] = 1.0f; // Sustain High
    p.dcwEnv.rates[2] = 0.99f; p.dcwEnv.levels[2] = 1.0f;
    p.dcwEnv.rates[3] = 0.5f;  p.dcwEnv.levels[3] = 0.0f;
    
    // DCA
    p.dcaEnv.rates[0] = 0.99f; p.dcaEnv.levels[0] = 1.0f; // Click attack
    p.dcaEnv.rates[1] = 0.9f;  p.dcaEnv.levels[1] = 1.0f;
    p.dcaEnv.rates[2] = 0.99f; p.dcaEnv.levels[2] = 1.0f;
    p.dcaEnv.rates[3] = 0.7f;  p.dcaEnv.levels[3] = 0.0f;
    
    p.parameters[ParameterIDs::dcwAttack.toStdString()] = 0.0f; p.parameters[ParameterIDs::dcwDecay.toStdString()] = 0.0f; p.parameters[ParameterIDs::dcwSustain.toStdString()] = 1.0f; p.parameters[ParameterIDs::dcwRelease.toStdString()] = 0.1f;
    p.parameters[ParameterIDs::dcaAttack.toStdString()] = 0.001f; p.parameters[ParameterIDs::dcaDecay.toStdString()] = 0.1f; p.parameters[ParameterIDs::dcaSustain.toStdString()] = 1.0f; p.parameters[ParameterIDs::dcaRelease.toStdString()] = 0.2f;
    
    // Filter
    p.parameters[ParameterIDs::lpfCutoff.toStdString()] = 20000.0f;
    p.parameters[ParameterIDs::lpfReso.toStdString()] = 0.1f;

    // LFO
    p.parameters[ParameterIDs::lfoRate.toStdString()] = 4.0f;
    
    // Effects
    p.parameters[ParameterIDs::delayTime.toStdString()] = 0.4f; p.parameters[ParameterIDs::delayFeedback.toStdString()] = 0.5f; p.parameters[ParameterIDs::delayMix.toStdString()] = 0.4f;
    p.parameters[ParameterIDs::reverbSize.toStdString()] = 0.4f; p.parameters[ParameterIDs::reverbMix.toStdString()] = 0.2f;
    
    p.parameters[ParameterIDs::hardSync.toStdString()] = 1.0f; // ENABLE HARD SYNC FOR LEAD
    p.parameters[ParameterIDs::lineMod.toStdString()] = 0.0f;
    p.parameters[ParameterIDs::glideTime.toStdString()] = 0.2f; // ENABLE GLIDE FOR LEAD!
    p.parameters[ParameterIDs::chorusRate.toStdString()] = 0.5f; p.parameters[ParameterIDs::chorusDepth.toStdString()] = 2.0f; p.parameters[ParameterIDs::chorusMix.toStdString()] = 0.0f;
p.parameters[ParameterIDs::chorusRate.toStdString()] = 0.5f; p.parameters[ParameterIDs::chorusDepth.toStdString()] = 2.0f; p.parameters[ParameterIDs::chorusMix.toStdString()] = 0.0f;

    presets.push_back(p);
}


} // namespace State
} // namespace CZ101
