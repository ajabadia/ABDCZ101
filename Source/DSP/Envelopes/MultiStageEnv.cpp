#include "MultiStageEnv.h"
#include <algorithm>

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
    
    // Start from 0
    float startVal = 0.0f;
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
    
    // If currently holding at sustain point, move to next stage immediately
    if (active && currentStage == sustainPoint)
    {
        // Force transition to next stage
        currentStage++;
        if (currentStage > endPoint)
        {
            active = false;
            return;
        }
        
        // Setup next stage from current value
        float currentVal = smoother.getCurrentValue();
        float seconds = rateToSeconds(stages[currentStage].rate);
        
        smoother.reset(sampleRate, seconds > 0.001f ? seconds : 0.001f);
        smoother.setCurrentAndTargetValue(currentVal);
        smoother.setTargetValue(stages[currentStage].level);
    }
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

float MultiStageEnvelope::rateToSeconds(float rate) const noexcept
{
    // CZ-101 Rate approximation
    // Rate 0.0 (slow) -> ~3 seconds (can be longer on real hardware)
    // Rate 1.0 (fast) -> ~1 ms
    
    // Using exponential curve
    // Invert rate: 1.0 is slow, 0.0 is fast for calculation
    float r = 1.0f - rate;
    
    // Base 30s max time
    return 0.001f + (std::pow(r, 4.0f) * 30.0f);
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
