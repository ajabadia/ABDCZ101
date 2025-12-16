#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <map>
#include <string>

namespace CZ101 {
namespace State {

class Parameters
{
public:
    Parameters(juce::AudioProcessor& processor);
    
    void createParameters();
    
    // Oscillator parameters
    juce::AudioParameterFloat* osc1Level = nullptr;
    juce::AudioParameterChoice* osc1Waveform = nullptr;
    juce::AudioParameterFloat* osc2Level = nullptr;
    juce::AudioParameterChoice* osc2Waveform = nullptr;
    juce::AudioParameterFloat* osc2Detune = nullptr;
    juce::AudioParameterBool* hardSync = nullptr;
    juce::AudioParameterBool* ringMod = nullptr;
    juce::AudioParameterFloat* glideTime = nullptr;
    
    // DCA Envelope
    juce::AudioParameterFloat* dcaAttack = nullptr;
    juce::AudioParameterFloat* dcaDecay = nullptr;
    juce::AudioParameterFloat* dcaSustain = nullptr;
    juce::AudioParameterFloat* dcaRelease = nullptr;
    
    // DCW Envelope
    juce::AudioParameterFloat* dcwAttack = nullptr;
    juce::AudioParameterFloat* dcwDecay = nullptr;
    juce::AudioParameterFloat* dcwSustain = nullptr;
    juce::AudioParameterFloat* dcwRelease = nullptr;
    
    // Filter
    juce::AudioParameterFloat* filterCutoff = nullptr;
    juce::AudioParameterFloat* filterResonance = nullptr;
    juce::AudioParameterChoice* filterType = nullptr;
    
    // Effects
    juce::AudioParameterFloat* delayTime = nullptr;
    juce::AudioParameterFloat* delayFeedback = nullptr;
    juce::AudioParameterFloat* delayMix = nullptr;
    
    juce::AudioParameterFloat* reverbSize = nullptr;
    juce::AudioParameterFloat* reverbMix = nullptr;

    juce::AudioParameterFloat* chorusRate = nullptr;
    juce::AudioParameterFloat* chorusDepth = nullptr;
    juce::AudioParameterFloat* chorusMix = nullptr;
    
    // LFO
    juce::AudioParameterFloat* lfoRate = nullptr;
    juce::AudioParameterFloat* lfoDepth = nullptr; // Vibrato depth
    juce::AudioParameterChoice* lfoWaveform = nullptr;
    
    // Get parameter by ID helper
    juce::RangedAudioParameter* getParameter(const juce::String& paramId) const;
    
    // Expose map for iteration (Saving)
    const std::map<juce::String, juce::RangedAudioParameter*>& getParameterMap() const { return parameterMap; }
    
private:
    juce::AudioProcessor& audioProcessor;
    std::map<juce::String, juce::RangedAudioParameter*> parameterMap;
};

} // namespace State
} // namespace CZ101
