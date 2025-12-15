#include "Delay.h"

namespace CZ101 {
namespace DSP {

Delay::Delay()
{
    buffer.fill(0.0f);
}

void Delay::setSampleRate(double sr) noexcept
{
    sampleRate = sr;
}

void Delay::setDelayTime(float seconds) noexcept
{
    seconds = std::clamp(seconds, 0.001f, 2.0f);
    delayInSamples = static_cast<int>(seconds * sampleRate);
    delayInSamples = std::min(delayInSamples, MAX_DELAY_SAMPLES - 1);
}

void Delay::setFeedback(float amount) noexcept
{
    feedback = std::clamp(amount, 0.0f, 0.95f);
}

void Delay::setMix(float amount) noexcept
{
    mix = std::clamp(amount, 0.0f, 1.0f);
}

void Delay::reset() noexcept
{
    buffer.fill(0.0f);
    writePos = 0;
}

float Delay::processSample(float input) noexcept
{
    int readPos = writePos - delayInSamples;
    if (readPos < 0)
        readPos += MAX_DELAY_SAMPLES;
    
    float delayed = buffer[readPos];
    buffer[writePos] = input + delayed * feedback;
    
    writePos = (writePos + 1) % MAX_DELAY_SAMPLES;
    
    return input * (1.0f - mix) + delayed * mix;
}

} // namespace DSP
} // namespace CZ101
