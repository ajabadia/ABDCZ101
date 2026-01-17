#pragma once

#include <vector>
#include <cmath>
#include <JuceHeader.h>

namespace CZ101 {
namespace DSP {
namespace Effects {

class Chorus {
public:
    Chorus();
    
    void prepare(double sampleRate);
    void reset();
    
    void setRate(float rateHz);
    void setDepth(float depthMs);
    void setMix(float mix0to1);
    
    void process(float* leftChannel, float* rightChannel, int numSamples);
    
private:
    double sampleRate = 44100.0;
    
    // Parameters
    float rate = 0.5f;
    float depth = 2.0f; // ms
    float mix = 0.0f;
    
    // Delay lines
    std::vector<float> delayBufferL;
    std::vector<float> delayBufferR;
    size_t writeIndex = 0;
    size_t bufferSize = 0;
    
    // LFO state
    float lfoPhase = 0.0f;
    float lfoIncrement = 0.0f;
    
    // Helpers
    float getInterpolatedSample(const std::vector<float>& buffer, float readIndex) const;
};

} // namespace Effects
} // namespace DSP
} // namespace CZ101
