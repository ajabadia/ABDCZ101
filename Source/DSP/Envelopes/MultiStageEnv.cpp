#include "MultiStageEnv.h"
#include <algorithm>
#include "../../Core/AuthenticHardware.h"

namespace CZ101 {
namespace DSP {

MultiStageEnvelope::MultiStageEnvelope()
{
    // Default: Simple ADSR-like shape using 8 stages
    // Stage 0: Attack to 1.0
    setStage(0, 0.9f, 1.0f);
    // Stage 1: Decay to 0.5
    setStage(1, 0.8f, 0.5f);
    // Stage 2: Sustain at 0.5
    setStage(2, 0.99f, 0.5f);
    
    // Sets sustain point at Stage 2
    setSustainPoint(2);
    
    // Stage 3: Release to 0
    setStage(3, 0.8f, 0.0f);
    
    // End point at Stage 3
    setEndPoint(3);
}

void MultiStageEnvelope::setSampleRate(double sr) noexcept
{
    sampleRate = sr;
}

void MultiStageEnvelope::setStage(int index, float rate, float level) noexcept
{
    if (index >= 0 && index < MAX_STAGES)
    {
        stages[index].rate = std::clamp(rate, 0.0f, 1.0f);
        stages[index].level = std::clamp(level, 0.0f, 1.0f);
    }
}

void MultiStageEnvelope::setSustainPoint(int stageIndex) noexcept
{
    if (stageIndex >= -1 && stageIndex < MAX_STAGES)
        sustainPoint = stageIndex;
}

void MultiStageEnvelope::setEndPoint(int stageIndex) noexcept
{
    if (stageIndex >= 0 && stageIndex < MAX_STAGES)
        endPoint = stageIndex;
}

void MultiStageEnvelope::noteOn() noexcept
{
    currentStage = 0;
    active = true;
    released = false;
    
    // Start from Configured Initial Value (0.0 for Amp/DCW, 0.5 for Pitch)
    float startVal = initialValue;
    smoother.setCurrentAndTargetValue(startVal);
    
    // Setup first stage
    float seconds = rateToSeconds(stages[0].rate);
    smoother.reset(sampleRate, seconds > 0.001f ? seconds : 0.001f); 
    smoother.setCurrentAndTargetValue(startVal);
    smoother.setTargetValue(stages[0].level);
}

void MultiStageEnvelope::noteOff() noexcept
{
    released = true;
    
    // AUTHENTIC CZ BEHAVIOR: "Dampening" / Jump to End Point
    // When key is released, regardless of current stage (even if before sustain),
    // the envelope immediately targets the End Point Level using the End Point Rate.
    if (active)
    {
        // Jump state to End Point
        // Note: In CZ, the "End Point" step IS the release phase.
        currentStage = endPoint;

        // Verify validity
        if (currentStage < MAX_STAGES)
        {
            // Retarget smoother from current value to End Point Level
            float currentVal = smoother.getCurrentValue();
            float seconds = rateToSeconds(stages[currentStage].rate);
            
            // Audit Fix 1.2: Ensure cleaner reset
            smoother.reset(sampleRate, seconds > 0.001f ? seconds : 0.001f);
            smoother.setCurrentAndTargetValue(currentVal);
            smoother.setTargetValue(stages[currentStage].level);
        }
        else
        {
            active = false;
        }
    }
}

// Audit Fix 1.1: Implementation
// Audit Fix 1.1: Implementation
void MultiStageEnvelope::setCurrentValue(float val) noexcept
{
    // Force smoother target to value instantly
    // We must reset the smoother to snap it, otherwise it ramps from previous value
    smoother.reset(sampleRate, 0.001); // Minimal time to avoid division by zero but effectively instant
    smoother.setCurrentAndTargetValue(val);
}

void MultiStageEnvelope::reset() noexcept
{
    active = false;
    currentStage = 0;
    smoother.setCurrentAndTargetValue(0.0f);
}

float MultiStageEnvelope::getNextValue() noexcept
{
    if (!active) return 0.0f;
    
    float val = smoother.getNextValue();
    
    // Check if stage finished
    if (!smoother.isSmoothing())
    {
        val = smoother.getTargetValue(); // Ensure snap
        
        // Are we at Sustain Point?
        if (currentStage == sustainPoint && !released)
        {
            // Hold here until Note Off
            // Do nothing, just return val
        }
        else if (currentStage >= endPoint)
        {
            // End of envelope
            // If released or no sustain, we are done
            // If we are sustaining at end (unlikely for CZ architecture, end is end), disable.
            active = false;
        }
        else
        {
            // Move to next stage
            currentStage++;
            
            if (currentStage < MAX_STAGES)
            {
                float currentVal = val;
                float seconds = rateToSeconds(stages[currentStage].rate);
                smoother.reset(sampleRate, seconds > 0.001f ? seconds : 0.001f);
                smoother.setCurrentAndTargetValue(currentVal);
                smoother.setTargetValue(stages[currentStage].level);
            }
            else
            {
                active = false;
            }
        }
    }
    
    return val;
}

void MultiStageEnvelope::setModel(Model newModel) noexcept
{
    activeModel = newModel;
}

float MultiStageEnvelope::rateToSeconds(float rate) const noexcept
{
    // Authentic 0-99 Step Mapping
    int rate99 = static_cast<int>(rate * 99.0f);
    
    // Use AuthenticHardware utility
    float seconds = CZ101::Core::HardwareConstants::getRateInSeconds(rate99, activeModel == Model::CZ5000);

    return seconds * rateScaler;
}

float MultiStageEnvelope::getStageRate(int index) const noexcept
{
    if (index >= 0 && index < MAX_STAGES)
        return stages[index].rate;
    return 0.0f;
}

float MultiStageEnvelope::getStageLevel(int index) const noexcept
{
    if (index >= 0 && index < MAX_STAGES)
        return stages[index].level;
    return 0.0f;
}

} // namespace DSP
} // namespace CZ101
