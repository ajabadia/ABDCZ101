#pragma once

#include <cmath>
#include <algorithm>

namespace CZ101 {
namespace DSP {

class ResonantFilter
{
public:
    enum Type
    {
        LOWPASS = 0,
        HIGHPASS,
        BANDPASS,
        NUM_TYPES
    };
    
    ResonantFilter();
    
    void setSampleRate(double sampleRate) noexcept;
    void setType(Type type) noexcept;
    void setCutoff(float frequency) noexcept;
    void setResonance(float q) noexcept;
    void reset() noexcept;
    
    float processSample(float input) noexcept;
    
private:
    double sampleRate = 44100.0;
    Type filterType = LOWPASS;
    float cutoffFreq = 1000.0f;
    float resonance = 0.7f;
    
    // State variables (2-pole)
    float z1 = 0.0f;
    float z2 = 0.0f;
    
    // Coefficients
    float a0 = 1.0f, a1 = 0.0f, a2 = 0.0f;
    float b1 = 0.0f, b2 = 0.0f;
    
    void updateCoefficients() noexcept;
};

} // namespace DSP
} // namespace CZ101
