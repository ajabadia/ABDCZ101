#include "StereoChorus.h"

namespace CZ101 {
namespace DSP {
namespace Effects {

StereoChorus::StereoChorus() {
    // Set LFOs to be 90 degrees out of phase for stereo width
    lfoR.setPhaseOffset(0.25f); // 0.25 of a cycle is 90 degrees
}

void StereoChorus::prepare(double sampleRate) {
    sr = sampleRate;
    lfoL.setSampleRate(sr);
    lfoR.setSampleRate(sr);
    
    // Max delay of ~50ms should be plenty for a chorus
    int bufferSize = (int)(sr * 0.05);
    delayBufferL.setSize(bufferSize);
    delayBufferR.setSize(bufferSize);
    delayBufferL.clear();
    delayBufferR.clear();
}

void StereoChorus::reset() { // Audit Fix 1.2: Reset
    lfoL.reset();
    lfoR.reset();
    delayBufferL.clear();
    delayBufferR.clear();
}

void StereoChorus::process(float* left, float* right, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        // Get LFO values (range -1 to 1)
        float lfoValL = lfoL.getNextValue();
        float lfoValR = lfoR.getNextValue();

        // Map LFO to a delay time perturbation
        // Example: 10ms base delay, modulated by +/- 5ms
        float baseDelayMs = 15.0f;
        float modulationDepthMs = 10.0f * depth;

        float delayMsL = baseDelayMs + modulationDepthMs * lfoValL;
        float delayMsR = baseDelayMs + modulationDepthMs * lfoValR;
        
        float delaySamplesL = (float)(sr * delayMsL / 1000.0);
        float delaySamplesR = (float)(sr * delayMsR / 1000.0);

        // Get delayed samples
        float delayedL = delayBufferL.getInterpolated(delaySamplesL);
        float delayedR = delayBufferR.getInterpolated(delaySamplesR);
        
        // Write current dry sample to buffer for future reads
        delayBufferL.push(left[i]);
        delayBufferR.push(right[i]);

        // Mix dry and wet signals
        left[i]  = (1.0f - mix) * left[i]  + mix * delayedL;
        right[i] = (1.0f - mix) * right[i] + mix * delayedR;
    }
}

void StereoChorus::setRate(float rateHz) {
    rate = rateHz;
    lfoL.setFrequency(rate);
    lfoR.setFrequency(rate);
}

void StereoChorus::setDepth(float d) {
    depth = juce::jlimit(0.0f, 1.0f, d);
}

void StereoChorus::setMix(float m) {
    mix = juce::jlimit(0.0f, 1.0f, m);
}

} // namespace Effects
} // namespace DSP
} // namespace CZ101
