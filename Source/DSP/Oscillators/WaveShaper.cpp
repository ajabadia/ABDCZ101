#include "WaveShaper.h"
#include <algorithm>

namespace CZ101 {
namespace DSP {

float WaveShaper::applyPhaseDistortion(float phase, float amount) const noexcept
{
    // Clamp inputs
    phase = std::clamp(phase, 0.0f, 1.0f);
    amount = std::clamp(amount, 0.0f, 1.0f);
    
    // No distortion: return original phase
    if (amount < 0.001f)
        return phase;
    
    // Apply resonance curve
    // This creates the characteristic CZ-101 timbre by
    // compressing/expanding different parts of the waveform
    float distorted = resonanceCurve(phase, amount);
    
    return std::clamp(distorted, 0.0f, 1.0f);
}

float WaveShaper::resonanceCurve(float phase, float resonance) const noexcept
{
    // CZ-101 Phase Distortion algorithm
    // Based on the original Casio implementation
    
    // The curve compresses the first half and expands the second half
    // creating harmonic content similar to filter resonance
    
    // Calculate distortion factor
    // Higher resonance = more compression/expansion
    float factor = 1.0f + resonance * 3.0f;
    
    // Apply non-linear curve
    // This creates the phase distortion effect
    float distorted;
    
    if (phase < 0.5f)
    {
        // First half: compress (speeds up playback)
        float t = phase * 2.0f;  // Normalize to [0, 1]
        distorted = std::pow(t, factor) * 0.5f;
    }
    else
    {
        // Second half: expand (slows down playback)
        float t = (phase - 0.5f) * 2.0f;  // Normalize to [0, 1]
        distorted = 0.5f + (1.0f - std::pow(1.0f - t, factor)) * 0.5f;
    }
    
    return distorted;
}

} // namespace DSP
} // namespace CZ101
