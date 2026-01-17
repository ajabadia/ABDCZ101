#pragma once

#include <array>
#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>

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
    
    // Audit Fix [2.2]: Hardware Model Selection
    enum class Model { CZ101, CZ5000 };
    void setModel(Model newModel) noexcept;

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
    // Audit Fix [1.1]: Allow external reset of value (for pitch centering)
    void setCurrentValue(float val) noexcept;
    
    float getNextValue() noexcept;
    float getCurrentValue() const noexcept { return mapValue(smoother.getCurrentValue()); } // Audit Fix: Added getter
    
    // Getters for adapter logic
    float getStageRate(int index) const noexcept;
    float getStageLevel(int index) const noexcept;
    
    bool isActive() const noexcept { return active; }
    bool isReleased() const noexcept { return released; }
    int getCurrentStage() const noexcept { return currentStage; }
    
    int getSustainPoint() const noexcept { return sustainPoint; }
    int getEndPoint() const noexcept { return endPoint; }
    
private:
    double sampleRate = 44100.0;
    std::array<Stage, MAX_STAGES> stages;
    
    // Envelope Smoother
    juce::LinearSmoothedValue<float> smoother;
    
    int currentStage = 0;
    // Removed manual currentValue/Increment/targetValue as smoother handles it
    
    int sustainPoint = -1;  // -1 = no sustain (or one-shot)
    int endPoint = 7;       // Default to using all 8 stages
    
    bool active = false;
    bool released = false;
    
    // CZ-101 Rate to Time conversion (internal helper)
    // Rate 0-99 mapped to ms
    float rateToSeconds(float rate) const noexcept;
    
    // Velocity Sensitivity [NEW]
    float rateScaler = 1.0f; 
    
    Model activeModel = Model::CZ101; // Audit Fix [2.2] 
    
    // Helper to map normalized smoother value if needed, but smoother returns float directly.
    // If smoother.getCurrentValue() is already correct, mapValue might be identity.
    // However, if we assume 0..1 range or something else?
    // Let's assume identity for now if not defined elsewhere.
    float mapValue(float v) const noexcept { return v; } 
public:
    void setRateScaler(float scale) noexcept { rateScaler = scale; }
};

} // namespace DSP
} // namespace CZ101
