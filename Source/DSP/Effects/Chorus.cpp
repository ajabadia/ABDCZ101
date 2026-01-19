#include "Chorus.h"
#include <cmath>

namespace CZ101 {
namespace DSP {
namespace Effects {

Chorus::Chorus()
{
    // Max delay depth 20ms usually enough (Allocating 50ms for safety)
    // 50ms at 192kHz ~= 9600 samples
    bufferSize = 16384; 
    delayBufferL.resize(bufferSize, 0.0f);
    delayBufferR.resize(bufferSize, 0.0f);
}

void Chorus::prepare(double sr)
{
    sampleRate = sr;
    bbd.prepare(sr); // [NEW]
    setRate(rate); // Recalc increment
    reset();
}

void Chorus::reset()
{
    std::fill(delayBufferL.begin(), delayBufferL.end(), 0.0f);
    std::fill(delayBufferR.begin(), delayBufferR.end(), 0.0f);
    writeIndex = 0;
    lfoPhase = 0.0f;
    bbd.reset(); // [NEW]
}

void Chorus::setRate(float rateHz)
{
    rate = rateHz;
    bbd.setRate(rateHz);
    // Inc per sample = Rate / SR
    // 2PI for sin? Or 0-1 phasor? Using 0-1
    lfoIncrement = rate / static_cast<float>(sampleRate);
}

void Chorus::setDepth(float depthMs)
{
    depth = depthMs;
    // Convert ms to 0-1 range for BBD if needed or pass as is? BBD uses 0-1 depth.
    // Assuming simple mapping for now.
    bbd.setDepth(std::clamp(depthMs / 5.0f, 0.0f, 1.0f));
}

void Chorus::setMix(float mix0to1)
{
    mix = std::clamp(mix0to1, 0.0f, 1.0f);
    bbd.setMix(mix);
}

float Chorus::getInterpolatedSample(const std::vector<float>& buffer, float readIndex) const
{
    if (bufferSize <= 0 || buffer.empty()) return 0.0f;

    // Linear Interpolation
    size_t index1 = static_cast<size_t>(readIndex);
    if (index1 >= bufferSize) index1 = bufferSize - 1;
    
    size_t index2 = (index1 + 1);
    if (index2 >= bufferSize) index2 = 0; // Wrap

    float fraction = readIndex - (float)index1;
    
    // Bounds check for safety
    // Bounds check for safety (size_t is unsigned, so only check upper bound)
    if (index1 >= buffer.size() || index2 >= buffer.size())
        return 0.0f;

    float s1 = buffer[index1];
    float s2 = buffer[index2];
    
    return s1 + fraction * (s2 - s1);
}

void Chorus::process(float* leftChannel, float* rightChannel, int numSamples)
{
    // Phase 8: BBD Authentic Simulation
    if (!isModern)
    {
        bbd.process(leftChannel, rightChannel, numSamples);
        return;
    }

    if (mix < 0.01f) return; // Bypass efficiency
    
    // ... Modern Digital Chorus Code ...
    const float depthSamples = (depth / 1000.0f) * static_cast<float>(sampleRate);
    // Base delay for Chorus usually slightly more than depth excursion
    const float baseDelay = depthSamples * 1.5f + 100.0f; // Offset to avoid crossing write pointer
    
    for (int i = 0; i < numSamples; ++i)
    {
        // Update LFO
        lfoPhase += lfoIncrement;
        // Audit Fix 2.10: Handle negative phase/large increments
        while (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
        while (lfoPhase < 0.0f) lfoPhase += 1.0f;
        
        // Calculate LFO values
        // Left: Sin(phase)
        // Right: Cos(phase) or Sin(phase + 90) -> Separation
        float lfoValL = std::sin(lfoPhase * 2.0f * juce::MathConstants<float>::pi);
        float lfoValR = std::cos(lfoPhase * 2.0f * juce::MathConstants<float>::pi);
        
        // Calculate read positions
        // Delay = Base + Depth * LFO
        float delayL = baseDelay + (depthSamples * lfoValL);
        float delayR = baseDelay + (depthSamples * lfoValR);
        
        // Circular buffer read pointers
        // Audit Fix 1.6: Use fmod for robust circular buffer wrapping
        float readPosL = static_cast<float>(writeIndex) - delayL;
        readPosL = std::fmod(readPosL, static_cast<float>(bufferSize));
        if (readPosL < 0.0f) readPosL += bufferSize;
        
        float readPosR = static_cast<float>(writeIndex) - delayR;
        readPosR = std::fmod(readPosR, static_cast<float>(bufferSize));
        if (readPosR < 0.0f) readPosR += bufferSize;
        
        // Read wet samples
        float wetL = getInterpolatedSample(delayBufferL, readPosL);
        float wetR = getInterpolatedSample(delayBufferR, readPosR);
        
        // Write inputs to buffer with Denormal Protection
        if (writeIndex >= 0 && writeIndex < bufferSize)
        {
            delayBufferL[writeIndex] = leftChannel[i] + 1.0e-18f;
            delayBufferR[writeIndex] = rightChannel[i] + 1.0e-18f;
        }
        
        // Mix
        leftChannel[i] = (leftChannel[i] * (1.0f - mix * 0.5f)) + (wetL * mix);
        rightChannel[i] = (rightChannel[i] * (1.0f - mix * 0.5f)) + (wetR * mix);
        
        // Advance write pointer
        // Advance write pointer
        writeIndex++;
        if (writeIndex >= bufferSize) writeIndex = 0;
    }
}

} // namespace Effects
} // namespace DSP
} // namespace CZ101
