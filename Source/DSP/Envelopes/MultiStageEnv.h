#pragma once

#include <array>
#include <cmath>

namespace CZ101 {
namespace DSP {

/**
 * @brief Multi-Stage Envelope Generator (8 stages)
 * 
 * Authentic CZ-101 Envelope Architecture:
 * - 8 Steps per envelope
 * - Each step has a Rate (speed) and Level (target)
 * - Sustain Point: The step where the envelope holds while key is pressed.
 * - End Point: The final step of the envelope.
 */
class MultiStageEnvelope
{
public:
    static constexpr int MAX_STAGES = 8;
    
    struct Stage
    {
        float level = 0.0f;      // Target level [0.0, 1.0]
        float rate = 0.5f;       // Speed to reach level [0.0, 1.0] (1.0 = fast, 0.0 = slow)
    };
    
    MultiStageEnvelope();
    
    void setSampleRate(double sampleRate) noexcept;
    
    // Configuration
    void setStage(int index, float rate, float level) noexcept;
    void setSustainPoint(int stageIndex) noexcept;
    void setEndPoint(int stageIndex) noexcept;
    
    // Runtime
    void noteOn() noexcept;
    void noteOff() noexcept;
    void reset() noexcept;
    
    float getNextValue() noexcept;
    
    // Getters for adapter logic
    float getStageRate(int index) const noexcept;
    float getStageLevel(int index) const noexcept;
    
    bool isActive() const noexcept { return active; }
    int getCurrentStage() const noexcept { return currentStage; }
    
    int getSustainPoint() const noexcept { return sustainPoint; }
    int getEndPoint() const noexcept { return endPoint; }
    
private:
    double sampleRate = 44100.0;
    std::array<Stage, MAX_STAGES> stages;
    
    int currentStage = 0;
    float currentValue = 0.0f;
    float currentIncrement = 0.0f;
    float targetValue = 0.0f;
    
    int sustainPoint = -1;  // -1 = no sustain (or one-shot)
    int endPoint = 7;       // Default to using all 8 stages
    
    bool active = false;
    bool released = false;
    
    // CZ-101 Rate to Time conversion (internal helper)
    // Rate 0-99 mapped to ms
    float rateToSeconds(float rate) const noexcept;
};

} // namespace DSP
} // namespace CZ101
