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
    // Authentic CZ-101 Rate Table (Approximate Mapping of 0-99 values to Seconds)
    static const float cz101RateTable[100] = {
        60.000f, 47.931f, 38.309f, 30.635f, 24.512f, 19.620f, 15.711f, 12.589f, 10.096f, 8.106f,
        6.516f, 5.244f, 4.225f, 3.411f, 2.761f, 2.240f, 1.821f, 1.484f, 1.213f, 0.995f,
        0.819f, 0.676f, 0.560f, 0.466f, 0.389f, 0.325f, 0.273f, 0.230f, 0.194f, 0.165f,
        0.141f, 0.120f, 0.103f, 0.089f, 0.077f, 0.067f, 0.058f, 0.051f, 0.045f, 0.039f,
        0.035f, 0.031f, 0.027f, 0.024f, 0.022f, 0.020f, 0.018f, 0.016f, 0.014f, 0.013f,
        0.012f, 0.011f, 0.010f, 0.009f, 0.008f, 0.007f, 0.007f, 0.006f, 0.006f, 0.005f,
        0.005f, 0.005f, 0.004f, 0.004f, 0.004f, 0.004f, 0.003f, 0.003f, 0.003f, 0.003f,
        0.002f, 0.002f, 0.002f, 0.002f, 0.002f, 0.002f, 0.001f, 0.001f, 0.001f, 0.001f,
        0.001f, 0.001f, 0.001f, 0.001f, 0.001f, 0.001f, 0.001f, 0.001f, 0.001f, 0.001f,
        0.001f, 0.001f, 0.001f, 0.001f, 0.001f, 0.001f, 0.001f, 0.001f, 0.001f, 0.001f
    };

    // CZ-5000 Rate Table (Slightly faster/snappier curves based on community analysis)
    // Note: This is an approximation. CZ-5000 envelopes are notably faster at the attack phase.
    static const float cz5000RateTable[100] = {
        55.000f, 43.000f, 34.000f, 27.000f, 21.000f, 17.000f, 13.500f, 10.800f, 8.600f, 6.900f,
        5.500f, 4.400f, 3.500f, 2.800f, 2.200f, 1.800f, 1.450f, 1.150f, 0.920f, 0.740f,
        0.600f, 0.480f, 0.390f, 0.320f, 0.260f, 0.210f, 0.170f, 0.140f, 0.115f, 0.095f,
        0.080f, 0.068f, 0.057f, 0.048f, 0.040f, 0.034f, 0.029f, 0.025f, 0.021f, 0.018f,
        0.015f, 0.013f, 0.011f, 0.010f, 0.009f, 0.008f, 0.007f, 0.006f, 0.005f, 0.0045f,
        0.004f, 0.0035f, 0.003f, 0.0028f, 0.0025f, 0.0023f, 0.002f, 0.0018f, 0.0016f, 0.0014f,
        0.0012f, 0.0011f, 0.0010f, 0.0009f, 0.0008f, 0.0008f, 0.0007f, 0.0007f, 0.0006f, 0.0006f,
        0.0005f, 0.0005f, 0.0005f, 0.0005f, 0.0004f, 0.0004f, 0.0004f, 0.0004f, 0.0003f, 0.0003f,
        0.0003f, 0.0003f, 0.0002f, 0.0002f, 0.0002f, 0.0002f, 0.0002f, 0.0002f, 0.0001f, 0.0001f,
        0.0001f, 0.0001f, 0.0001f, 0.0001f, 0.0001f, 0.0001f, 0.0001f, 0.0001f, 0.0001f, 0.0001f
    };

    const float* table = (activeModel == Model::CZ5000) ? cz5000RateTable : cz101RateTable;

    // Interpolate between discrete hardware steps
    float scaledRate = rate * 99.0f;
    int index = std::clamp(static_cast<int>(scaledRate), 0, 98);
    float frac = scaledRate - static_cast<float>(index);
    
    float seconds = (1.0f - frac) * table[index] + frac * table[index + 1];
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
