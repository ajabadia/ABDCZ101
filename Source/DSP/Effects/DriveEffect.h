#pragma once

#include <JuceHeader.h>


namespace CZ101 {
namespace DSP {
namespace Effects {

class DriveEffect
{
public:
    DriveEffect();
    
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();
    
    // Parameters
    void setAmount(float amount) noexcept; // 0.0 to 1.0 (Saturation)
    void setColor(float color) noexcept;   // 0.0 (Dark) to 1.0 (Bright)
    void setMix(float mix) noexcept;       // 0.0 (Dry) to 1.0 (Wet)
    
    void process(juce::dsp::ProcessContextReplacing<float>& context);
    
private:
    float currentAmount = 0.0f;
    float currentColor = 0.5f;
    float currentMix = 0.0f;
    
    // DSP Components
    juce::dsp::IIR::Filter<float> toneFilter;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> filterChain;
    
    juce::AudioBuffer<float> wetBuffer; // Internal buffer for wet path processing
    
    double sampleRate = 44100.0;
    
    // Internal helper for saturation curve
    float processSample(float input);
};

} // namespace Effects
} // namespace DSP
} // namespace CZ101
