#include "MultiStageEnv.h"
#include <algorithm>
#include "../../Core/HardwareConstants.h"
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
    // Envelopes are advanced at control rate (every CONTROL_RATE_DIVIDER samples),
    // so the smoother must be told the effective poll rate.
    effectiveRate = sr / static_cast<double>(CZ101::Core::HardwareConstants::CONTROL_RATE_DIVIDER);
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
    float targetVal = getScaledLevel(0);
    float delta = std::max(0.0001f, std::abs(targetVal - startVal));
    float seconds = rateToSeconds(stages[0].rate) * delta;
    smoother.reset(effectiveRate, seconds > 0.001f ? seconds : 0.001f);
    smoother.setCurrentAndTargetValue(startVal);
    smoother.setTargetValue(targetVal);
}

void MultiStageEnvelope::noteOff() noexcept
{
    released = true;
    
    if (active)
    {
        if (sustainPoint >= 0)
        {
            if (sustainPoint < endPoint)
            {
                // When key is released, proceed to the release stage (immediately following sustainPoint)
                currentStage = sustainPoint + 1;
            }
            else
            {
                // Sustain point is the end point. Key release means end immediately.
                active = false;
                return;
            }
        }
        else
        {
            // sustainPoint == -1 (No sustain). Key release has no effect, the envelope continues its normal course.
            return;
        }

        if (currentStage < MAX_STAGES)
        {
            float currentVal = smoother.getCurrentValue();
            float targetVal = getScaledLevel(currentStage);
            float delta = std::max(0.0001f, std::abs(targetVal - currentVal));
            float seconds = rateToSeconds(stages[currentStage].rate) * delta;
            
            smoother.reset(effectiveRate, seconds > 0.001f ? seconds : 0.001f);
            smoother.setCurrentAndTargetValue(currentVal);
            smoother.setTargetValue(targetVal);
        }
        else
        {
            active = false;
        }
    }
}

// Audit Fix 1.1: Implementation
void MultiStageEnvelope::setCurrentValue(float val) noexcept
{
    // Force smoother target to value instantly
    // We must reset the smoother to snap it, otherwise it ramps from previous value
    smoother.reset(effectiveRate, 0.001); // Minimal time to avoid division by zero but effectively instant
    smoother.setCurrentAndTargetValue(val);
}

void MultiStageEnvelope::reset() noexcept
{
    active = false;
    currentStage = 0;
    smoother.setCurrentAndTargetValue(initialValue);
}

float MultiStageEnvelope::getNextValue() noexcept
{
    if (!active) return initialValue;
    
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
                float targetVal = getScaledLevel(currentStage);
                float delta = std::max(0.0001f, std::abs(targetVal - currentVal));
                float seconds = rateToSeconds(stages[currentStage].rate) * delta;
                smoother.reset(effectiveRate, seconds > 0.001f ? seconds : 0.001f);
                smoother.setCurrentAndTargetValue(currentVal);
                smoother.setTargetValue(targetVal);
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
    // Authentic 0-99 Step Mapping (Use std::round to avoid float truncation errors like 39.999 -> 39)
    int rate99 = static_cast<int>(std::round(rate * 99.0f));
    
    // Convert EnvType enum to the integer expected by the hardware timing function
    int envTypeInt = 0; // DCA
    if (envType == EnvType::DCW) envTypeInt = 1;
    else if (envType == EnvType::DCO) envTypeInt = 2;
    
    // Use AuthenticHardware utility
    float seconds = CZ101::Core::HardwareConstants::getRateInSeconds(rate99, envTypeInt, activeModel == Model::CZ5000);

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

float MultiStageEnvelope::getScaledLevel(int index) const noexcept
{
    if (index >= 0 && index < MAX_STAGES)
    {
        float rawLevel = stages[index].level;
        return initialValue + (rawLevel - initialValue) * levelScaler;
    }
    return initialValue;
}

} // namespace DSP
} // namespace CZ101
