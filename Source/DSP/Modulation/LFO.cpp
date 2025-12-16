#include "LFO.h"
#include <algorithm>
#include <random>
#include <mutex>
#include <array>

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

// Static LFO Table
constexpr int LFO_TABLE_SIZE = 2048;
static std::array<float, LFO_TABLE_SIZE> lfoSineTable;
static std::once_flag lfoTableFlag;

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
    // Initialize table once
    std::call_once(lfoTableFlag, [](){
        constexpr float TWO_PI = 6.28318530718f;
        for (int i = 0; i < LFO_TABLE_SIZE; ++i) {
            lfoSineTable[i] = std::sin(TWO_PI * (static_cast<float>(i) / LFO_TABLE_SIZE));
        }
    });

    float indexF = phase * static_cast<float>(LFO_TABLE_SIZE);
    int index = static_cast<int>(indexF);
    float frac = indexF - static_cast<float>(index);
    
    // Safety wrap (phase is 0..1 but float math)
    if (index >= LFO_TABLE_SIZE) index = 0;
    
    int nextIndex = index + 1;
    if (nextIndex >= LFO_TABLE_SIZE) nextIndex = 0;
    
    float val1 = lfoSineTable[index];
    float val2 = lfoSineTable[nextIndex];
    
    return val1 + frac * (val2 - val1);
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
