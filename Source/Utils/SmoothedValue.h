#pragma once

#include <cmath>

namespace CZ101 {
namespace Utils {

template<typename FloatType>
class SmoothedValue
{
public:
    SmoothedValue() = default;
    
    void reset(FloatType initialValue) noexcept
    {
        currentValue = initialValue;
        target = initialValue;
    }
    
    void setTargetValue(FloatType newTarget) noexcept
    {
        target = newTarget;
    }
    
    void setSmoothingTime(FloatType timeInSeconds, double sampleRate) noexcept
    {
        auto numSamples = static_cast<int>(timeInSeconds * sampleRate);
        setNumSteps(numSamples);
    }
    
    void setNumSteps(int numSteps) noexcept
    {
        stepsToTarget = numSteps;
        countdown = stepsToTarget;
        
        if (countdown > 0)
            step = (target - currentValue) / static_cast<FloatType>(countdown);
        else
            step = 0;
    }
    
    FloatType getNextValue() noexcept
    {
        if (countdown <= 0)
        {
            currentValue = target;
            return currentValue;
        }
        
        --countdown;
        currentValue += step;
        return currentValue;
    }
    
    FloatType getCurrentValue() const noexcept
    {
        return currentValue;
    }
    
    FloatType getTargetValue() const noexcept
    {
        return target;
    }
    
    bool isSmoothing() const noexcept
    {
        return countdown > 0;
    }

private:
    FloatType currentValue = 0;
    FloatType target = 0;
    FloatType step = 0;
    int countdown = 0;
    int stepsToTarget = 0;
};

} // namespace Utils
} // namespace CZ101
