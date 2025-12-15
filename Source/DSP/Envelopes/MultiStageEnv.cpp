#include "MultiStageEnv.h"
#include <algorithm>

namespace CZ101 {
namespace DSP {

MultiStageEnvelope::MultiStageEnvelope()
{
    // Initialize with default envelope
    setStage(0, 0.0f, 0.001f);   // Start at 0
    setStage(1, 1.0f, 0.01f);    // Fast attack
    setStage(2, 0.7f, 0.1f);     // Decay
    setStage(3, 0.7f, 0.1f, true); // Sustain
    setStage(4, 0.0f, 0.2f);     // Release
    setStage(5, 0.0f, 0.0f);     // Unused
    setStage(6, 0.0f, 0.0f);     // Unused
    setStage(7, 0.0f, 0.0f);     // Unused
}

void MultiStageEnvelope::setSampleRate(double sr) noexcept
{
    sampleRate = sr;
}

void MultiStageEnvelope::setStage(int index, float level, float time, bool sustain) noexcept
{
    if (index < 0 || index >= MAX_STAGES)
        return;
    
    stages[index].level = std::clamp(level, 0.0f, 1.0f);
    stages[index].time = std::max(0.001f, time);
    stages[index].sustain = sustain;
    
    if (sustain)
        sustainStage = index;
}

void MultiStageEnvelope::noteOn() noexcept
{
    currentStage = 0;
    currentValue = stages[0].level;
    stageProgress = 0.0f;
    released = false;
}

void MultiStageEnvelope::noteOff() noexcept
{
    if (currentStage < 0)
        return;
    
    released = true;
    
    // If we're at sustain stage, advance to next
    if (sustainStage >= 0 && currentStage == sustainStage)
    {
        advanceToNextStage();
    }
}

void MultiStageEnvelope::reset() noexcept
{
    currentStage = -1;
    currentValue = 0.0f;
    stageProgress = 0.0f;
    released = false;
}

float MultiStageEnvelope::getNextValue() noexcept
{
    if (currentStage < 0 || currentStage >= MAX_STAGES)
        return 0.0f;
    
    const Stage& stage = stages[currentStage];
    
    // If sustain stage and not released, hold value
    if (stage.sustain && !released)
        return currentValue;
    
    // Calculate progress
    float increment = 1.0f / (stage.time * static_cast<float>(sampleRate));
    stageProgress += increment;
    
    // Get start value (current or previous stage level)
    float startValue = (currentStage > 0) ? stages[currentStage - 1].level : 0.0f;
    float targetValue = stage.level;
    
    // Apply curve
    float curvedProgress = calculateCurve(stageProgress, startValue, targetValue);
    currentValue = curvedProgress;
    
    // Check if stage complete
    if (stageProgress >= 1.0f)
    {
        currentValue = targetValue;
        advanceToNextStage();
    }
    
    return currentValue;
}

float MultiStageEnvelope::calculateCurve(float t, float start, float end) const noexcept
{
    t = std::clamp(t, 0.0f, 1.0f);
    
    // Exponential curve
    float curved = 1.0f - std::exp(-CURVE_FACTOR * t);
    
    // Interpolate between start and end
    return start + (end - start) * curved;
}

void MultiStageEnvelope::advanceToNextStage() noexcept
{
    stageProgress = 0.0f;
    currentStage++;
    
    // Check if we've reached the end
    if (currentStage >= MAX_STAGES)
    {
        currentStage = -1;  // Idle
        currentValue = 0.0f;
        return;
    }
    
    // Skip unused stages (time = 0)
    while (currentStage < MAX_STAGES && stages[currentStage].time <= 0.001f)
    {
        currentStage++;
    }
    
    if (currentStage >= MAX_STAGES)
    {
        currentStage = -1;
        currentValue = 0.0f;
    }
}

} // namespace DSP
} // namespace CZ101
