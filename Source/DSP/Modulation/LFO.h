#pragma once

#include <cmath>

namespace CZ101 {
namespace DSP {

class LFO
{
public:
    enum Waveform
    {
        SINE = 0,
        TRIANGLE,
        SAWTOOTH,
        SQUARE,
        RANDOM,
        NUM_WAVEFORMS
    };
    
    LFO();
    
    void setSampleRate(double sampleRate) noexcept;
    void setFrequency(float hz) noexcept;
    void setWaveform(Waveform waveform) noexcept;
    void reset() noexcept;
    
    float getNextValue() noexcept;
    
private:
    double sampleRate = 44100.0;
    float frequency = 1.0f;
    Waveform currentWaveform = SINE;
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    float randomValue = 0.0f;
    
    void updatePhaseIncrement() noexcept;
    float renderSine() noexcept;
    float renderTriangle() noexcept;
    float renderSawtooth() noexcept;
    float renderSquare() noexcept;
    float renderRandom() noexcept;
};

} // namespace DSP
} // namespace CZ101
