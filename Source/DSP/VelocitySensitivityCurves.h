#pragma once

#include <juce_core/juce_core.h>

namespace CZ101 {
namespace DSP {

/**
 * Defines how different synth parameters respond to velocity.
 */
struct VelocityCurve {
    // 0.0 = No response (flat), 1.0 = Linear full response
    // Negative values = Inverse response (Harder hits reduce value)
    
    float amplitudeResponse = 1.0f;      // Louder with velocity
    float pitchResponse = 0.0f;          // Slight pitch bend with velocity (usually 0 or very small like 0.1)
    float attackResponse = 0.0f;         // Faster attack with velocity (usually negative, e.g., -0.5)
    float dcwResponse = 0.6f;            // Brighter tone with velocity
    float vibratoDepthResponse = 0.0f;   // Deeper vibrato with velocity
};

class VelocitySensitivityProcessor {
public:
    /**
     * Presets for common instrument behaviors
     */
    static VelocityCurve getPercussiveCurve() {
        return { 1.0f, 0.0f, -0.6f, 0.8f, 0.0f }; // Hard hits = Loud, Fast Attack, Bright
    }
    
    static VelocityCurve getPadCurve() {
        return { 0.8f, 0.0f, -0.2f, 0.4f, 0.3f }; // Softer response, slight brightness/vibrato
    }
    
    static VelocityCurve getLeadCurve() {
        return { 0.9f, 0.1f, -0.3f, 0.5f, 0.5f }; // Expressive lead with pitch/vibrato nuances
    }
    
    static VelocityCurve getFlatCurve() {
        return { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f }; // Standard velocity = volume only
    }

    /**
     * Calculates a scaling factor based on velocity and sensitivity.
     * @param velocity Normalized MIDI velocity (0.0 - 1.0)
     * @param sensitivity Sensitivity factor (can be negative)
     * @return Scaling factor centered around 1.0 or appropriate range
     */
    static float apply(float velocity, float sensitivity) noexcept {
        if (std::abs(sensitivity) < 0.001f) return 1.0f;
        
        // Linear mapping: 
        // If sensitivity > 0: Range [1.0 - sens, 1.0] ??? No, standard is usually [0, 1] mapped to [1-sens, 1] is reduction?
        // Let's use standard modulation logic:
        // Value = Base * (1.0 + (Velocity * Sensitivity) - Sensitivity) ??? 
        // 
        // Simpler: 
        // If sens = 1.0, Vel 0 -> 0, Vel 1 -> 1
        // If sens = 0.5, Vel 0 -> 0.5, Vel 1 -> 1
        // Formula: 1.0 - Sensitivity + (Velocity * Sensitivity)
        
        if (sensitivity > 0.0f) {
            return 1.0f - sensitivity + (velocity * sensitivity);
        } else {
            // Inverse mapping (Negative sensitivity)
            // If sens = -0.5 (Attack faster on high vel)
            // Vel 0 -> 1.0 (Slowest/Base), Vel 1 -> 0.5 (Fastest)
            // Formula: 1.0 + (Velocity * Sensitivity)  (remember Sensitivity is negative)
             return 1.0f + (velocity * sensitivity);
        }
    }
    
    static float applyPitch(float velocity, float sensitivity) noexcept {
        // Pitch needs to be additive in semitones usually, or ratio.
        // Returning a ratio here.
        // Sensitivity 0.1 -> Max +10% frequency (approx 1.6 semitones)
        // Usually subtle: 0.01
        return 1.0f + (velocity * sensitivity * 0.1f); // Scaled down for safety
    }
};

} // namespace DSP
} // namespace CZ101
