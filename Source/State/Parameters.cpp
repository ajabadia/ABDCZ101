#include "Parameters.h"

namespace CZ101 {
namespace State {

Parameters::Parameters(juce::AudioProcessor& processor)
    : audioProcessor(processor)
{
}

void Parameters::createParameters()
{
    using namespace juce;
    
    // Oscillator 1
    osc1Level = new AudioParameterFloat("osc1_level", "Osc 1 Level", 0.0f, 1.0f, 0.5f);
    audioProcessor.addParameter(osc1Level);
    
    osc1Waveform = new AudioParameterChoice("osc1_waveform", "Osc 1 Waveform",
        StringArray{"Sine", "Sawtooth", "Square", "Triangle"}, 0);
    audioProcessor.addParameter(osc1Waveform);
    
    // Oscillator 2
    osc2Level = new AudioParameterFloat("osc2_level", "Osc 2 Level", 0.0f, 1.0f, 0.5f);
    audioProcessor.addParameter(osc2Level);
    
    osc2Waveform = new AudioParameterChoice("osc2_waveform", "Osc 2 Waveform",
        StringArray{"Sine", "Sawtooth", "Square", "Triangle"}, 0);
    audioProcessor.addParameter(osc2Waveform);
    
    osc2Detune = new AudioParameterFloat("osc2_detune", "Osc 2 Detune", -100.0f, 100.0f, 0.0f);
    audioProcessor.addParameter(osc2Detune);
    
    // DCA Envelope
    dcaAttack = new AudioParameterFloat("dca_attack", "DCA Attack", 0.001f, 5.0f, 0.01f);
    audioProcessor.addParameter(dcaAttack);
    
    dcaDecay = new AudioParameterFloat("dca_decay", "DCA Decay", 0.001f, 5.0f, 0.1f);
    audioProcessor.addParameter(dcaDecay);
    
    dcaSustain = new AudioParameterFloat("dca_sustain", "DCA Sustain", 0.0f, 1.0f, 0.7f);
    audioProcessor.addParameter(dcaSustain);
    
    dcaRelease = new AudioParameterFloat("dca_release", "DCA Release", 0.001f, 5.0f, 0.2f);
    audioProcessor.addParameter(dcaRelease);
    
    // DCW Envelope
    dcwAttack = new AudioParameterFloat("dcw_attack", "DCW Attack", 0.001f, 5.0f, 0.05f);
    audioProcessor.addParameter(dcwAttack);
    
    dcwDecay = new AudioParameterFloat("dcw_decay", "DCW Decay", 0.001f, 5.0f, 0.2f);
    audioProcessor.addParameter(dcwDecay);
    
    dcwSustain = new AudioParameterFloat("dcw_sustain", "DCW Sustain", 0.0f, 1.0f, 0.5f);
    audioProcessor.addParameter(dcwSustain);
    
    dcwRelease = new AudioParameterFloat("dcw_release", "DCW Release", 0.001f, 5.0f, 0.3f);
    audioProcessor.addParameter(dcwRelease);
    
    // Filter
    filterCutoff = new AudioParameterFloat("filter_cutoff", "Filter Cutoff", 20.0f, 20000.0f, 1000.0f);
    audioProcessor.addParameter(filterCutoff);
    
    filterResonance = new AudioParameterFloat("filter_resonance", "Filter Resonance", 0.1f, 10.0f, 0.7f);
    audioProcessor.addParameter(filterResonance);
    
    filterType = new AudioParameterChoice("filter_type", "Filter Type",
        StringArray{"Lowpass", "Highpass", "Bandpass"}, 0);
    audioProcessor.addParameter(filterType);
    
    // Delay
    delayTime = new AudioParameterFloat("delay_time", "Delay Time", 0.001f, 2.0f, 0.5f);
    audioProcessor.addParameter(delayTime);
    
    delayFeedback = new AudioParameterFloat("delay_feedback", "Delay Feedback", 0.0f, 0.95f, 0.5f);
    audioProcessor.addParameter(delayFeedback);
    
    delayMix = new AudioParameterFloat("delay_mix", "Delay Mix", 0.0f, 1.0f, 0.3f);
    audioProcessor.addParameter(delayMix);
    
    // LFO
    lfoRate = new AudioParameterFloat("lfo_rate", "LFO Rate", 0.01f, 20.0f, 1.0f);
    audioProcessor.addParameter(lfoRate);
    
    lfoWaveform = new AudioParameterChoice("lfo_waveform", "LFO Waveform",
        StringArray{"Sine", "Triangle", "Sawtooth", "Square", "Random"}, 0);
    audioProcessor.addParameter(lfoWaveform);
    
    // Populate parameter map for fast lookup
    for (auto* param : audioProcessor.getParameters())
    {
        if (auto* rangedParam = dynamic_cast<juce::RangedAudioParameter*>(param))
        {
            // JUCE 7+ sometimes adds prefix, ensure we map the ID correctly
            parameterMap[rangedParam->paramID] = rangedParam;
        }
    }
}

juce::RangedAudioParameter* Parameters::getParameter(const juce::String& paramId) const
{
    auto it = parameterMap.find(paramId);
    if (it != parameterMap.end())
        return it->second;
    return nullptr;
}

} // namespace State
} // namespace CZ101
