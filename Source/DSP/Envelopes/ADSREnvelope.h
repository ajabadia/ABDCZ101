#pragma once

#include <cmath>

namespace CZ101 {
namespace DSP {

/**
 * @brief ADSR Envelope Generator
 * 
 * Classic Attack-Decay-Sustain-Release envelope with exponential curves.
 * Used for both amplitude (DCA) and filter/timbre (DCW) modulation.
 */
class ADSREnvelope
{
public:
    enum Stage
    {
        IDLE = 0,
        ATTACK,
        DECAY,
        SUSTAIN,
        RELEASE,
        NUM_STAGES
    };
    
    ADSREnvelope();
    
    void setSampleRate(double sampleRate) noexcept;
    
    // Parameters in seconds
    void setAttackTime(float seconds) noexcept;
    void setDecayTime(float seconds) noexcept;
    void setSustainLevel(float level) noexcept;  // [0.0, 1.0]
    void setReleaseTime(float seconds) noexcept;
    
    void noteOn() noexcept;
    void noteOff() noexcept;
    void reset() noexcept;
    
    /**
     * @brief Get next envelope value
     * @return Envelope value [0.0, 1.0]
     */
    float getNextValue() noexcept;
    
    Stage getCurrentStage() const noexcept { return currentStage; }
    bool isActive() const noexcept { return currentStage != IDLE; }
    
private:
    double sampleRate = 44100.0;
    
    // Parameters
    float attackTime = 0.01f;    // 10ms
    float decayTime = 0.1f;      // 100ms
    float sustainLevel = 0.7f;   // 70%
    float releaseTime = 0.2f;    // 200ms
    
    // State
    Stage currentStage = IDLE;
    float currentValue = 0.0f;
    float stageProgress = 0.0f;  // [0.0, 1.0]
    
    // Exponential curve factor (higher = more exponential)
    static constexpr float CURVE_FACTOR = 4.0f;
    
    float calculateExponentialCurve(float t) const noexcept;
    void advanceStage() noexcept;
};

} // namespace DSP
} // namespace CZ101
