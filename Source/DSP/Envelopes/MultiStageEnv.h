#pragma once

#include <array>
#include <cmath>

namespace CZ101 {
namespace DSP {

/**
 * @brief Multi-Stage Envelope Generator (8 stages)
 * 
 * Complex envelope with 8 configurable stages for advanced modulation.
 * Used in CZ-101 for DCW (Digital Controlled Wave) modulation.
 */
class MultiStageEnvelope
{
public:
    static constexpr int MAX_STAGES = 8;
    
    struct Stage
    {
        float level = 0.0f;      // Target level [0.0, 1.0]
        float time = 0.1f;       // Time to reach level (seconds)
        bool sustain = false;    // Hold at this stage until noteOff
    };
    
    MultiStageEnvelope();
    
    void setSampleRate(double sampleRate) noexcept;
    void setStage(int index, float level, float time, bool sustain = false) noexcept;
    
    void noteOn() noexcept;
    void noteOff() noexcept;
    void reset() noexcept;
    
    float getNextValue() noexcept;
    
    int getCurrentStage() const noexcept { return currentStage; }
    bool isActive() const noexcept { return currentStage >= 0; }
    
private:
    double sampleRate = 44100.0;
    std::array<Stage, MAX_STAGES> stages;
    
    int currentStage = -1;        // -1 = idle
    float currentValue = 0.0f;
    float stageProgress = 0.0f;
    int sustainStage = -1;        // Which stage is sustain
    bool released = false;
    
    static constexpr float CURVE_FACTOR = 4.0f;
    
    float calculateCurve(float t, float start, float end) const noexcept;
    void advanceToNextStage() noexcept;
};

} // namespace DSP
} // namespace CZ101
