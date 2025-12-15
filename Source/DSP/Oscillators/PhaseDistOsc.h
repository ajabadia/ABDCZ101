#pragma once

#include "WaveTable.h"
#include <cmath>

namespace CZ101 {
namespace DSP {

/**
 * @brief Phase Distortion Oscillator with PolyBLEP anti-aliasing
 * 
 * Core oscillator for CZ-101 emulation. Generates waveforms with
 * professional quality anti-aliasing using PolyBLEP technique.
 */
class PhaseDistOscillator
{
public:
    enum Waveform
    {
        SINE = 0,
        SAWTOOTH,
        SQUARE,
        TRIANGLE,
        NUM_WAVEFORMS
    };
    
    PhaseDistOscillator();
    
    /**
     * @brief Set sample rate
     * @param sampleRate Sample rate in Hz (e.g., 44100.0)
     */
    void setSampleRate(double sampleRate) noexcept;
    
    /**
     * @brief Set frequency
     * @param frequency Frequency in Hz (e.g., 440.0 for A4)
     */
    void setFrequency(float frequency) noexcept;
    
    /**
     * @brief Set waveform type
     * @param waveform Waveform enum value
     */
    void setWaveform(Waveform waveform) noexcept;
    
    /**
     * @brief Reset phase to zero
     */
    void reset() noexcept;
    
    /**
     * @brief Render next sample with Phase Distortion simulation
     * @param dcwAmount Timbre control [0.0 = Pure Sine, 1.0 = Full Waveform]
     * @param outDidWrap Pointer to bool that will be set to true if phase wrapped (optional)
     * @return Audio sample [-1.0, 1.0]
     */
    float renderNextSample(float dcwAmount, bool* outDidWrap = nullptr) noexcept;
    
private:
    WaveTable waveTable;
    
    double sampleRate = 44100.0;
    float frequency = 440.0f;
    Waveform currentWaveform = SINE;
    
    float phase = 0.0f;           // Current phase [0.0, 1.0]
    float phaseIncrement = 0.0f;  // Phase increment per sample
    
    /**
     * @brief PolyBLEP: Polynomial Bandlimited Step
     * 
     * Reduces aliasing by smoothing discontinuities in waveforms.
     * Essential for sawtooth and square waves.
     * 
     * @param t Normalized phase [0.0, 1.0]
     * @param dt Phase increment (frequency/sampleRate)
     * @return Correction value to subtract from naive waveform
     */
    float polyBLEP(float t, float dt) const noexcept;
    
    // Waveform renderers
    float renderSine() noexcept;
    float renderSawtooth() noexcept;
    float renderSquare() noexcept;
    float renderTriangle() noexcept;
    
    void updatePhaseIncrement() noexcept;
};

} // namespace DSP
} // namespace CZ101
