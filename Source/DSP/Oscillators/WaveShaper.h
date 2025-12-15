#pragma once

#include <cmath>

namespace CZ101 {
namespace DSP {

/**
 * @brief WaveShaper for Phase Distortion synthesis
 * 
 * Implements the core CZ-101 phase distortion algorithm.
 * Modulates the phase of a waveform to create harmonic content.
 */
class WaveShaper
{
public:
    WaveShaper() = default;
    
    /**
     * @brief Apply phase distortion to a normalized phase value
     * 
     * @param phase Input phase [0.0, 1.0]
     * @param amount Distortion amount [0.0, 1.0]
     *               0.0 = no distortion (linear)
     *               1.0 = maximum distortion
     * @return Distorted phase [0.0, 1.0]
     */
    float applyPhaseDistortion(float phase, float amount) const noexcept;
    
private:
    /**
     * @brief Resonance curve for phase distortion
     * Creates the characteristic CZ-101 timbre
     */
    float resonanceCurve(float phase, float resonance) const noexcept;
};

} // namespace DSP
} // namespace CZ101
