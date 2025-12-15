#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace CZ101 {
namespace DSP {

class Reverb
{
public:
    Reverb();
    
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    
    void setParameters(float roomSize, float damping, float wetLevel, float dryLevel, float width);
    
    void process(juce::AudioBuffer<float>& buffer);
    
private:
    juce::Reverb reverb;
    juce::Reverb::Parameters params;
    
    double currentSampleRate = 44100.0;
};

} // namespace DSP
} // namespace CZ101
