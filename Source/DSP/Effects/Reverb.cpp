#include "Reverb.h"

namespace CZ101 {
namespace DSP {

Reverb::Reverb()
{
    // Set default parameters
    params.roomSize = 0.5f;
    params.damping = 0.5f;
    params.wetLevel = 0.33f;
    params.dryLevel = 1.0f; // Normally we control mix differently, but juce::Reverb has explicit wet/dry
    params.width = 1.0f;
    params.freezeMode = 0.0f;
    
    reverb.setParameters(params);
}

void Reverb::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    reverb.setSampleRate(sampleRate);
    reverb.reset();
    (void)samplesPerBlock;
}

void Reverb::reset()
{
    reverb.reset();
}

void Reverb::setParameters(float roomSize, float damping, float wetLevel, float dryLevel, float width)
{
    params.roomSize = roomSize;
    params.damping = damping;
    params.wetLevel = wetLevel;
    params.dryLevel = dryLevel;
    params.width = width;
    
    reverb.setParameters(params);
}

void Reverb::process(juce::AudioBuffer<float>& buffer)
{
    // juce::Reverb processes stereo buffers naturally
    if (buffer.getNumChannels() == 2)
    {
        reverb.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), buffer.getNumSamples());
    }
    else if (buffer.getNumChannels() == 1)
    {
        reverb.processMono(buffer.getWritePointer(0), buffer.getNumSamples());
    }
}

} // namespace DSP
} // namespace CZ101
