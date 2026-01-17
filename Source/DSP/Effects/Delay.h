#pragma once

#include <array>
#include <vector>
#include <algorithm>

namespace CZ101 {
namespace DSP {
namespace Effects {
    
class Delay
{
public:
    Delay();
    
    void setSampleRate(double sampleRate) noexcept;
    void setDelayTime(float seconds) noexcept;
    void setFeedback(float amount) noexcept;
    void setMix(float amount) noexcept;
    void reset() noexcept;
    
    float processSample(float input) noexcept;
    
private:
    static constexpr float MAX_DELAY_SECONDS = 2.0f;
    std::vector<float> buffer;
    double sampleRate = 44100.0;
    size_t delayInSamples = 22050;
    size_t writePos = 0;
    float feedback = 0.5f;
    float mix = 0.3f;
};

} // namespace Effects
} // namespace DSP
} // namespace CZ101
