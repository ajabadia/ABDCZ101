#include <JuceHeader.h>
#include "DriveEffect.h"
#include <cmath>

namespace CZ101 {
namespace DSP {
namespace Effects {

DriveEffect::DriveEffect() : filterChain(juce::dsp::IIR::Coefficients<float>::makeLowShelf(44100.0, 1000.0f, 1.0f, 1.0f))
{
}

void DriveEffect::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    filterChain.prepare(spec);
    
    // Resize temp buffer
    wetBuffer.setSize((int)spec.numChannels, (int)spec.maximumBlockSize);
    
    reset();
}

void DriveEffect::reset()
{
    filterChain.reset();
    wetBuffer.clear();
}

void DriveEffect::setAmount(float amount) noexcept
{
    currentAmount = juce::jlimit(0.0f, 1.0f, amount);
}

void DriveEffect::setColor(float color) noexcept
{
    currentColor = juce::jlimit(0.0f, 1.0f, color);
    
    // Tone Control: High Shelf
    // 0.0: Dark (-12dB @ 2kHz)
    // 0.5: Flat
    // 1.0: Bright (+12dB @ 2kHz)
    
    float gainDb = (color - 0.5f) * 24.0f; // -12 to +12 dB
    float q = 0.707f;
    float freq = 2000.0f;
    
    *filterChain.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, freq, q, juce::Decibels::decibelsToGain(gainDb));
}

void DriveEffect::setMix(float mix) noexcept
{
    currentMix = juce::jlimit(0.0f, 1.0f, mix);
}

float DriveEffect::processSample(float input)
{
    // Soft Clipping with Drive
    float drive = 1.0f + (currentAmount * 24.0f); // Up to 25x gain
    float x = input * drive;
    
    // Tanh Saturation
    float saturated = std::tanh(x);
    
    // Make-up gain / Headroom
    return saturated * 0.8f; 
}

void DriveEffect::process(juce::dsp::ProcessContextReplacing<float>& context)
{
    // Bypass if fully Dry
    if (currentMix < 0.001f) return;
    
    const auto& inputBlock = context.getInputBlock();
    auto& outputBlock = context.getOutputBlock();
    const int numChannels = (int)outputBlock.getNumChannels();
    const int numSamples = (int)outputBlock.getNumSamples();
    
    // Ensure wet buffer is big enough (jic block size changes dynamically below max)
    if (wetBuffer.getNumSamples() < numSamples)
        wetBuffer.setSize(numChannels, numSamples, true, false, true);

    // 1. Copy Dry to Wet Buffer
    for (int ch = 0; ch < numChannels; ++ch)
    {
        wetBuffer.copyFrom(ch, 0, inputBlock.getChannelPointer(ch), numSamples);
    }
    
    // 2. Process Filters on Wet Buffer
    juce::dsp::AudioBlock<float> wetBlock(wetBuffer);
    // Create a context for the wet block
    // We must use a sub-block to match numSamples
    auto wetSubBlock = wetBlock.getSubBlock(0, numSamples);
    juce::dsp::ProcessContextReplacing<float> wetContext(wetSubBlock);
    
    filterChain.process(wetContext);
    
    // 3. Apply Drive (Saturation) per sample on Wet Buffer
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = wetBuffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            data[i] = processSample(data[i]);
        }
    }
    
    // 4. Mix Wet and Dry into Output
    // Output = Dry * (1-Mix) + Wet * Mix
    float wetGain = currentMix;
    float dryGain = 1.0f - currentMix;
    
    // But note: inputBlock and outputBlock might point to the same memory (Replacing)
    // So 'inputBlock' contents are valid until we write to 'outputBlock'.
    // Safe standard: 
    // If Mix is 100%, just copy wet to output.
    
    if (currentMix >= 0.999f) // Fully Wet
    {
        for (int ch = 0; ch < numChannels; ++ch)
            juce::FloatVectorOperations::copy(outputBlock.getChannelPointer(ch), wetBuffer.getReadPointer(ch), numSamples);
    }
    else
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* out = outputBlock.getChannelPointer(ch);
            auto* wet = wetBuffer.getReadPointer(ch);
            // In a replacing context, 'out' already contains 'dry' input data!
            // So: Out = Out * dryGain + Wet * wetGain
            
            juce::FloatVectorOperations::multiply(out, dryGain, numSamples);
            juce::FloatVectorOperations::addWithMultiply(out, wet, wetGain, numSamples);
        }
    }
}

} // namespace Effects
} // namespace DSP
} // namespace CZ101
