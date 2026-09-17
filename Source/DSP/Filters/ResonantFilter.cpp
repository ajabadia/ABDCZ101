#include "ResonantFilter.h"

namespace CZ101 {
namespace DSP {

ResonantFilter::ResonantFilter()
{
    updateCoefficients();
}

void ResonantFilter::setSampleRate(double sr) noexcept
{
    sampleRate = sr;
    updateCoefficients();
}

void ResonantFilter::setType(Type type) noexcept
{
    filterType = type;
    updateCoefficients();
}

void ResonantFilter::setCutoff(float frequency) noexcept
{
    float maxCutoff = static_cast<float>(sampleRate * 0.49);
    cutoffFreq = std::clamp(frequency, 20.0f, maxCutoff > 20.0f ? maxCutoff : 20000.0f);
    updateCoefficients();
}

void ResonantFilter::setResonance(float q) noexcept
{
    // Map normalized [0, 1] parameter to Q factor [0.7071 (flat Butterworth) .. 8.0]
    resonance = 0.7071f + std::clamp(q, 0.0f, 1.0f) * 6.5f;
    updateCoefficients();
}

void ResonantFilter::reset() noexcept
{
    z1 = 0.0f;
    z2 = 0.0f;
}

float ResonantFilter::processSample(float input) noexcept
{
    // Direct Form II (Transposed) - Canonical Biquad
    float output = a0 * input + z1;
    z1 = a1 * input - b1 * output + z2;
    z2 = a2 * input - b2 * output;
    return output;
}

void ResonantFilter::updateCoefficients() noexcept
{
    constexpr float PI = 3.14159265358979323846f;
    
    float omega = 2.0f * PI * cutoffFreq / static_cast<float>(sampleRate);
    float sinOmega = std::sin(omega);
    float cosOmega = std::cos(omega);
    float alpha = sinOmega / (2.0f * resonance);
    
    switch (filterType)
    {
        case LOWPASS:
        {
            float b0 = (1.0f - cosOmega) / 2.0f;
            float b1_coef = 1.0f - cosOmega;
            float b2_coef = (1.0f - cosOmega) / 2.0f;
            float a0_coef = 1.0f + alpha;
            float a1_coef = -2.0f * cosOmega;
            float a2_coef = 1.0f - alpha;
            
            a0 = b0 / a0_coef;
            a1 = b1_coef / a0_coef;
            a2 = b2_coef / a0_coef;
            b1 = a1_coef / a0_coef;
            b2 = a2_coef / a0_coef;
            break;
        }
        
        case HIGHPASS:
        {
            float b0 = (1.0f + cosOmega) / 2.0f;
            float b1_coef = -(1.0f + cosOmega);
            float b2_coef = (1.0f + cosOmega) / 2.0f;
            float a0_coef = 1.0f + alpha;
            float a1_coef = -2.0f * cosOmega;
            float a2_coef = 1.0f - alpha;
            
            a0 = b0 / a0_coef;
            a1 = b1_coef / a0_coef;
            a2 = b2_coef / a0_coef;
            b1 = a1_coef / a0_coef;
            b2 = a2_coef / a0_coef;
            break;
        }
        
        case BANDPASS:
        {
            float b0 = alpha;
            float b1_coef = 0.0f;
            float b2_coef = -alpha;
            float a0_coef = 1.0f + alpha;
            float a1_coef = -2.0f * cosOmega;
            float a2_coef = 1.0f - alpha;
            
            a0 = b0 / a0_coef;
            a1 = b1_coef / a0_coef;
            a2 = b2_coef / a0_coef;
            b1 = a1_coef / a0_coef;
            b2 = a2_coef / a0_coef;
            break;
        }
        
        default:
            break;
    }
}

} // namespace DSP
} // namespace CZ101
