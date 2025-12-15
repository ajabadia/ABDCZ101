#include "LFO.h"
#include <algorithm>
#include <random>

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
    frequency = std::clamp(hz, 0.01f, 20.0f);
    updatePhaseIncrement();
}

void LFO::setWaveform(Waveform waveform) noexcept
{
    currentWaveform = waveform;
}

void LFO::reset() noexcept
{
    phase = 0.0f;
}

void LFO::updatePhaseIncrement() noexcept
{
    phaseIncrement = frequency / static_cast<float>(sampleRate);
}

float LFO::getNextValue() noexcept
{
    float value = 0.0f;
    
    switch (currentWaveform)
    {
        case SINE: value = renderSine(); break;
        case TRIANGLE: value = renderTriangle(); break;
        case SAWTOOTH: value = renderSawtooth(); break;
        case SQUARE: value = renderSquare(); break;
        case RANDOM: value = renderRandom(); break;
        default: value = 0.0f;
    }
    
    phase += phaseIncrement;
    if (phase >= 1.0f)
        phase -= 1.0f;
    
    return value;
}

float LFO::renderSine() noexcept
{
    constexpr float TWO_PI = 6.28318530718f;
    return std::sin(TWO_PI * phase);
}

float LFO::renderTriangle() noexcept
{
    if (phase < 0.25f)
        return 4.0f * phase;
    else if (phase < 0.75f)
        return 2.0f - 4.0f * phase;
    else
        return 4.0f * phase - 4.0f;
}

float LFO::renderSawtooth() noexcept
{
    return 2.0f * phase - 1.0f;
}

float LFO::renderSquare() noexcept
{
    return (phase < 0.5f) ? 1.0f : -1.0f;
}

float LFO::renderRandom() noexcept
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
    
    if (phase < phaseIncrement)
        randomValue = dis(gen);
    
    return randomValue;
}

} // namespace DSP
} // namespace CZ101
