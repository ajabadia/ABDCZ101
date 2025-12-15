#pragma once

#include <array>
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
    static constexpr int MAX_DELAY_SAMPLES = 88200;  // 2 seconds @ 44.1kHz
    
    std::array<float, MAX_DELAY_SAMPLES> buffer;
    double sampleRate = 44100.0;
    int delayInSamples = 22050;
    int writePos = 0;
    float feedback = 0.5f;
    float mix = 0.3f;
};

} // namespace Effects
} // namespace DSP
} // namespace CZ101
