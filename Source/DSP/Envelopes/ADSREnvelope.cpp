#include "ADSREnvelope.h"
#include <algorithm>

namespace CZ101 {
namespace DSP {

ADSREnvelope::ADSREnvelope()
{
}

void ADSREnvelope::setSampleRate(double sr) noexcept
{
    sampleRate = sr;
}

void ADSREnvelope::setAttackTime(float seconds) noexcept
{
    attackTime = std::clamp(seconds, 0.001f, 10.0f);
}

void ADSREnvelope::setDecayTime(float seconds) noexcept
{
    decayTime = std::clamp(seconds, 0.001f, 10.0f);
}

void ADSREnvelope::setSustainLevel(float level) noexcept
{
    sustainLevel = std::clamp(level, 0.0f, 1.0f);
}

void ADSREnvelope::setReleaseTime(float seconds) noexcept
{
    releaseTime = std::clamp(seconds, 0.001f, 10.0f);
}

void ADSREnvelope::noteOn() noexcept
{
    currentStage = ATTACK;
    stageProgress = 0.0f;
}

void ADSREnvelope::noteOff() noexcept
{
    if (currentStage != IDLE)
    {
        currentStage = RELEASE;
        stageProgress = 0.0f;
    }
}

void ADSREnvelope::reset() noexcept
{
    currentStage = IDLE;
    currentValue = 0.0f;
    stageProgress = 0.0f;
}

float ADSREnvelope::getNextValue() noexcept
{
    if (currentStage == IDLE)
        return 0.0f;
    
    float stageDuration = 0.0f;
    float targetValue = 0.0f;
    float startValue = currentValue;
    
    switch (currentStage)
    {
        case ATTACK:
            stageDuration = attackTime;
            targetValue = 1.0f;
            startValue = 0.0f;
            break;
            
        case DECAY:
            stageDuration = decayTime;
            targetValue = sustainLevel;
            startValue = 1.0f;
            break;
            
        case SUSTAIN:
            return sustainLevel;
            
        case RELEASE:
            stageDuration = releaseTime;
            targetValue = 0.0f;
            startValue = currentValue;
            break;
            
        default:
            return 0.0f;
    }
    
    // Calculate progress increment
    float increment = 1.0f / (stageDuration * static_cast<float>(sampleRate));
    stageProgress += increment;
    
    // Apply exponential curve
    float curvedProgress = calculateExponentialCurve(stageProgress);
    
    // Interpolate between start and target
    currentValue = startValue + (targetValue - startValue) * curvedProgress;
    
    // Check if stage is complete
    if (stageProgress >= 1.0f)
    {
        currentValue = targetValue;
        advanceStage();
    }
    
    return currentValue;
}

float ADSREnvelope::calculateExponentialCurve(float t) const noexcept
{
    // Exponential curve: 1 - e^(-factor * t)
    // This creates a natural-sounding envelope
    t = std::clamp(t, 0.0f, 1.0f);
    return 1.0f - std::exp(-CURVE_FACTOR * t);
}

void ADSREnvelope::advanceStage() noexcept
{
    stageProgress = 0.0f;
    
    switch (currentStage)
    {
        case ATTACK:
            currentStage = DECAY;
            break;
            
        case DECAY:
            currentStage = SUSTAIN;
            break;
            
        case SUSTAIN:
            // Stay in sustain until noteOff
            break;
            
        case RELEASE:
            currentStage = IDLE;
            currentValue = 0.0f;
            break;
            
        default:
            break;
    }
}

} // namespace DSP
} // namespace CZ101
