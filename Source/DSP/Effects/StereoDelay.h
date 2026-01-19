#pragma once

#include <vector>
#include <algorithm>
#include <cmath>

namespace CZ101 {
namespace DSP {
namespace Effects {

class StereoDelay
{
public:
    StereoDelay()
    {
        setSampleRate(44100.0);
    }

    void setSampleRate(double sr) noexcept
    {
        sampleRate = sr;
        resizeBuffers();
        reset();
    }

    void prepare(double sr) { setSampleRate(sr); }

    void setParameters(float timeSec, float fb, float mixAmt, bool pingPong = false, float spread = 0.0f)
    {
        // Time
        float t = std::clamp(timeSec, 0.001f, 2.0f);
        
        // Spread logic: Offset Right channel slightly
        delayTimeSamplesL = static_cast<float>(t * sampleRate);
        
        if (spread > 0.01f) {
            // "Professional" spread often means slight timing offset or ratio
            // Let's use a ratio for musical spread (e.g. dotted 8th vs 8th) or just simple offset
            // For CZ, simple offset is safer but let's do a width spread
            float tR = t * (1.0f + spread * 0.5f); 
            delayTimeSamplesR = static_cast<float>(tR * sampleRate);
        } else {
             delayTimeSamplesR = delayTimeSamplesL;
        }

        // Clamp
        float maxDelay = (float)bufferL.size() - 2.0f;
        delayTimeSamplesL = std::min(delayTimeSamplesL, maxDelay);
        delayTimeSamplesR = std::min(delayTimeSamplesR, maxDelay);

        feedback = std::clamp(fb, 0.0f, 0.99f); // Limit to prevent explosion
        wetMix = std::clamp(mixAmt, 0.0f, 1.0f);
        dryMix = 1.0f - wetMix; // Equal gain or linear? Linear is fine for delay mix usually.
        
        isPingPong = pingPong;
    }

    void reset() noexcept
    {
        std::fill(bufferL.begin(), bufferL.end(), 0.0f);
        std::fill(bufferR.begin(), bufferR.end(), 0.0f);
        writePos = 0;
    }

    void process(float* left, float* right, int numSamples) noexcept
    {
        const int size = (int)bufferL.size();
        const int mask = size - 1; // Power of 2 optimization if we enforced it, but explicit modulo is safer for arbitrary sizes
        
        for (int i = 0; i < numSamples; ++i)
        {
            float inL = left[i];
            float inR = right[i];

            // Read from Delay Lines (Linear Interpolation)
            float dL = readBuffer(bufferL, delayTimeSamplesL, size);
            float dR = readBuffer(bufferR, delayTimeSamplesR, size);

            // Feedback Path
            float fbL, fbR;
            
            if (isPingPong)
            {
                // Cross-feed: Left out goes to Right in, Right out goes to Left in
                fbL = dR * feedback;
                fbR = dL * feedback;
            }
            else
            {
                // Normal Stereo: L->L, R->R
                fbL = dL * feedback;
                fbR = dR * feedback;
            }
            
            // Soft Saturation in loop (Tape-like safety)
            // fbL = std::tanh(fbL); // Too expensive per sample? 
            // Fast clamp is better for efficiency
            fbL = (fbL > 1.5f) ? 1.5f : (fbL < -1.5f) ? -1.5f : fbL;
            fbR = (fbR > 1.5f) ? 1.5f : (fbR < -1.5f) ? -1.5f : fbR;

            // Write to Buffers
            bufferL[writePos] = inL + fbL;
            bufferR[writePos] = inR + fbR;

            // Output Mix
            left[i] = (inL * dryMix) + (dL * wetMix);
            right[i] = (inR * dryMix) + (dR * wetMix);

            // Increment
            writePos++;
            if (writePos >= size) writePos = 0;
        }
    }

private:
    double sampleRate = 44100.0;
    std::vector<float> bufferL;
    std::vector<float> bufferR;
    int writePos = 0;
    
    float delayTimeSamplesL = 10000.0f;
    float delayTimeSamplesR = 10000.0f;
    float feedback = 0.0f;
    float wetMix = 0.0f;
    float dryMix = 1.0f;
    bool isPingPong = false;

    void resizeBuffers()
    {
        // 2.0 seconds max delay
        size_t size = static_cast<size_t>(sampleRate * 2.0) + 1024;
        if (bufferL.size() != size)
        {
            bufferL.resize(size, 0.0f);
            bufferR.resize(size, 0.0f);
        }
    }
    
    // Linear Interpolation Read
    inline float readBuffer(const std::vector<float>& buf, float delaySamples, int size)
    {
        float readIdx = (float)writePos - delaySamples;
        while (readIdx < 0.0f) readIdx += (float)size;
        
        int i1 = (int)readIdx;
        int i2 = i1 + 1;
        if (i2 >= size) i2 = 0;
        
        float frac = readIdx - (float)i1;
        return buf[i1] + frac * (buf[i2] - buf[i1]);
    }
};

} // namespace Effects
} // namespace DSP
} // namespace CZ101
