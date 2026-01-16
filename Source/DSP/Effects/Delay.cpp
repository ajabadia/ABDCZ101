#include "Delay.h"

namespace CZ101 {
namespace DSP {
namespace Effects {

Delay::Delay()
{
    // Default start
    setSampleRate(44100.0);
}

void Delay::setSampleRate(double sr) noexcept
{
    sampleRate = sr;
    // Audit Fix 1.4: Dynamic Buffer Sizing based on SR * MaxSeconds
    size_t requiredSize = static_cast<size_t>(sr * MAX_DELAY_SECONDS) + 256; // +Headroom
    if (buffer.size() != requiredSize)
        buffer.resize(requiredSize);
        
    reset();
}

void Delay::setDelayTime(float seconds) noexcept
{
    seconds = std::clamp(seconds, 0.001f, MAX_DELAY_SECONDS);
    delayInSamples = static_cast<int>(seconds * sampleRate);
    // Ensure we don't exceed buffer size
    delayInSamples = std::min(delayInSamples, (int)buffer.size() - 1);
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
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    writePos = 0;
}

float Delay::processSample(float input) noexcept
{
    int size = (int)buffer.size();
    if (size == 0) return input; // Safety

    int readPos = writePos - delayInSamples;
    if (readPos < 0)
        readPos += size;
    
    // Safety clamp (just in case logic fails or delayInSamples > size)
    while (readPos < 0) readPos += size;
    readPos = readPos % size;

    float delayed = buffer[readPos];
    buffer[writePos] = input + delayed * feedback;
    
    writePos = (writePos + 1) % size;
    
    return input * (1.0f - mix) + delayed * mix;
}

} // namespace Effects
} // namespace DSP
} // namespace CZ101
