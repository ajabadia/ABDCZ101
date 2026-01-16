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
    enum CzWaveform
    {
        SAWTOOTH,
        SQUARE,
        PULSE,
        DOUBLE_SINE,
        SAW_PULSE,
        RESONANCE_1,
        RESONANCE_2,
        RESONANCE_3,
        NUM_CZ_WAVEFORMS
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
    /**
     * @brief Set composite waveforms (Authentic CZ Behavior)
     * @param first First waveform (1-8)
     * @param second Second waveform (0-8, 0=None/Off)
     */
    void setWaveforms(CzWaveform first, CzWaveform second) noexcept;
    
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
    CzWaveform firstWaveform = SAWTOOTH;
    CzWaveform secondWaveform = SAWTOOTH; 
    bool secondWaveformActive = false;
    
    float phase = 0.0f;           // Current phase [0.0, 1.0]
    float phaseIncrement = 0.0f;  // Phase increment per sample

    /**
     * @brief Applies phase distortion to the current phase based on the selected waveform and DCW amount.
     * @param linearPhase The current, unmodified phase [0.0, 1.0].
     * @param dcwValue The DCW amount [0.0, 1.0] controlling the intensity of the distortion.
     * @return The distorted phase.
     */
    float applyPhaseDistortion(float linearPhase, float dcwValue, CzWaveform waveform) noexcept;
    
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
    
    void updatePhaseIncrement() noexcept;

    struct Constants {
        static constexpr float Resonance1Freq = 2.0f;
        static constexpr float Resonance2Freq = 4.0f;
        static constexpr float Resonance3Freq = 7.0f;
        
        static constexpr float Resonance1MaxMod = 0.15f;
        static constexpr float Resonance2MaxMod = 0.20f;
        static constexpr float Resonance3MaxMod = 0.25f;

        static constexpr float HalfPhase = 0.5f;
        static constexpr float QuarterPhase = 0.25f;
        static constexpr float ThreeQuarterPhase = 0.75f;
    };
};

} // namespace DSP
} // namespace CZ101
