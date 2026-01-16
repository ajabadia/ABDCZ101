#pragma once

#include <cmath>

namespace CZ101 {
namespace DSP {

class LFO
{
public:
    enum Waveform
    {
        TRIANGLE = 0,
        SAW_UP,
        SAW_DOWN,
        SQUARE,
        NUM_WAVEFORMS
    };
    
    LFO();
    
    void setSampleRate(double sampleRate) noexcept;
    void setFrequency(float hz) noexcept;
    void setWaveform(Waveform waveform) noexcept;
    void setDelay(float seconds) noexcept;
    void setPhaseOffset(float offset) noexcept; // For Chorus
    void setFrequencyScale(float scale) noexcept; // For Modulation
    
    void reset() noexcept; // Resets phase AND delay timer
    
    float getNextValue() noexcept;
    
private:
    double sampleRate = 44100.0;
    float frequency = 1.0f;
    Waveform currentWaveform = TRIANGLE;
    float phase = 0.0f;
    float phaseOffset = 0.0f;
    float freqScale = 1.0f;
    float phaseIncrement = 0.0f;
    
    // Delay Logic
    float delayTime = 0.0f;
    float delayTimer = 0.0f;
    
    void updatePhaseIncrement() noexcept;
    float renderSine() noexcept;
    float renderTriangle() noexcept;
    float renderSawtooth() noexcept;
    float renderSquare() noexcept;
    float renderRandom() noexcept;
};

} // namespace DSP
} // namespace CZ101
