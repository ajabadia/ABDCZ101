#include "../PresetManager.h"
#include "../ParameterIDs.h"
#include "preset_helpers.h"

namespace CZ101 {
namespace State {

void PresetManager::createBellsPreset()
{
    Preset p;
    p.name = "Digital Bells";
    initEnvelopes(p);
    
    p.parameters[ParameterIDs::osc1Waveform.toStdString()] = 0.0f; p.parameters[ParameterIDs::osc1Level.toStdString()] = 1.0f;
    p.parameters[ParameterIDs::osc2Waveform.toStdString()] = 0.0f; p.parameters[ParameterIDs::osc2Level.toStdString()] = 1.0f;
    p.parameters[ParameterIDs::osc2Detune.toStdString()] = 350.0f; // Detune for bell
    
    // DCW: Short
    p.dcwEnv.rates[0] = 0.99f; p.dcwEnv.levels[0] = 1.0f;
    p.dcwEnv.rates[1] = 0.4f;  p.dcwEnv.levels[1] = 0.0f; // Long decay to 0
    p.dcwEnv.rates[2] = 0.99f; p.dcwEnv.levels[2] = 0.0f; // Hold 0
    p.dcwEnv.rates[3] = 0.5f;  p.dcwEnv.levels[3] = 0.0f;
    p.dcwEnv.sustainPoint = 2; // Sustain silence
    
    // DCA
    p.dcaEnv.rates[0] = 0.99f; p.dcaEnv.levels[0] = 1.0f;
    p.dcaEnv.rates[1] = 0.3f;  p.dcaEnv.levels[1] = 0.0f; // Long decay
    p.dcaEnv.rates[2] = 0.99f; p.dcaEnv.levels[2] = 0.0f;
    p.dcaEnv.rates[3] = 0.5f;  p.dcaEnv.levels[3] = 0.0f;
    
    p.parameters[ParameterIDs::dcwAttack.toStdString()] = 0.0f; p.parameters[ParameterIDs::dcwDecay.toStdString()] = 0.8f; p.parameters[ParameterIDs::dcwSustain.toStdString()] = 0.0f; p.parameters[ParameterIDs::dcwRelease.toStdString()] = 0.5f;
    p.parameters[ParameterIDs::dcaAttack.toStdString()] = 0.0f; p.parameters[ParameterIDs::dcaDecay.toStdString()] = 1.5f; p.parameters[ParameterIDs::dcaSustain.toStdString()] = 0.0f; p.parameters[ParameterIDs::dcaRelease.toStdString()] = 1.0f;

    // Filter
    p.parameters[ParameterIDs::lpfCutoff.toStdString()] = 12000.0f;
    p.parameters[ParameterIDs::lpfReso.toStdString()] = 0.2f;

    // LFO
    p.parameters[ParameterIDs::lfoRate.toStdString()] = 6.0f;
    
    // Effects
    p.parameters[ParameterIDs::delayTime.toStdString()] = 0.0f; p.parameters[ParameterIDs::delayFeedback.toStdString()] = 0.0f; p.parameters[ParameterIDs::delayMix.toStdString()] = 0.0f;
    p.parameters[ParameterIDs::reverbSize.toStdString()] = 0.9f; p.parameters[ParameterIDs::reverbMix.toStdString()] = 0.4f; // Spacey
    
    p.parameters[ParameterIDs::hardSync.toStdString()] = 0.0f;
    p.parameters[ParameterIDs::lineMod.toStdString()] = 1.0f; // Ring 1 for bells
    p.parameters[ParameterIDs::glideTime.toStdString()] = 0.0f;

    presets.push_back(p);
}


} // namespace State
} // namespace CZ101
