#include "LFO.h"
#include <algorithm>

namespace CZ101 {
namespace DSP {

LFO::LFO()
{
    updatePhaseIncrement();
}

void LFO::setSampleRate(double sr) noexcept
{
    sampleRate = sr;
    updatePhaseIncrement();
}

void LFO::setFrequency(float hz) noexcept
{
    frequency = std::clamp(hz, 0.01f, 30.0f); // Range updated to 30Hz authentic/useful range
    updatePhaseIncrement();
}

void LFO::setWaveform(Waveform waveform) noexcept
{
    currentWaveform = waveform;
}

void LFO::setDelay(float seconds) noexcept
{
    delayTime = std::max(0.0f, seconds);
}

void LFO::reset() noexcept
{
    phase = 0.0f;
    delayTimer = 0.0f; // Reset delay timer on Note On
}

void LFO::updatePhaseIncrement() noexcept
{
    phaseIncrement = (frequency * freqScale) / static_cast<float>(sampleRate);
}

void LFO::setFrequencyScale(float scale) noexcept
{
    freqScale = scale;
    updatePhaseIncrement();
}

void LFO::setPhaseOffset(float offset) noexcept
{
    phaseOffset = offset;
}

float LFO::getNextValue() noexcept
{
    // Handle Delay
    if (delayTimer < delayTime)
    {
        delayTimer += (1.0f / static_cast<float>(sampleRate));
        if (delayTimer < delayTime)
        {
            // Still in delay phase
            return 0.0f;
        }
        else
        {
            // Delay finished, reset phase?
            phase = 0.0f;
        }
    }
    
    // Calculate current position including offset
    float currentPos = phase + phaseOffset;
    while (currentPos >= 1.0f) currentPos -= 1.0f;
    while (currentPos < 0.0f) currentPos += 1.0f;

    float value = 0.0f;
    
    switch (currentWaveform)
    {
        case TRIANGLE: 
            if (currentPos < 0.25f) value = 4.0f * currentPos;
            else if (currentPos < 0.75f) value = 2.0f - 4.0f * currentPos;
            else value = 4.0f * currentPos - 4.0f;
            break;
            
        case SAW_UP: // Ramp Up
            value = 2.0f * currentPos - 1.0f;
            break;
            
        case SAW_DOWN: // Ramp Down
            value = 1.0f - 2.0f * currentPos;
            break;
            
        case SQUARE: // Trill
            value = (currentPos < 0.5f) ? 1.0f : -1.0f;
            break;
            
        default: value = 0.0f;
    }
    
    phase += phaseIncrement;
    if (phase >= 1.0f)
        phase -= 1.0f;
    
    return value;
}

} // namespace DSP
} // namespace CZ101
