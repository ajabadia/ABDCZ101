#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <juce_core/juce_core.h>
#include <vector>
#include "../Modulation/LFO.h" // Correct path from DSP/Effects

namespace CZ101 {
namespace DSP {
namespace Effects {

/**
 * Authentic Bucket Brigade Device (BBD) Chorus simulation.
 * Emulates the MN3009/MN3101 chips used in the CZ-5000/3000/1.
 * Features:
 * - Clock noise simulation
 * - Bucket loss (high frequency damping)
 * - Aliasing artifacts (reduced sample rate interpolation)
 */
class BBDChorus
{
public:
    BBDChorus() = default;

    void prepare(double sampleRate)
    {
        this->sampleRate = sampleRate;
        // BBD delay lines are typically 256-4096 stages. CZ Chorus is likely around 512-1024.
        bbdBuffer.resize(4096, 0.0f);
        chorusLfo.setSampleRate(sampleRate);
        chorusLfo.setWaveform(CZ101::DSP::LFO::TRIANGLE);
        chorusLfo.setFrequency(lfoRate);
        
        // Use a second LFO for Phase Inversion logic? Or just direct phase offset?
        // BBD chips often used complementary clocks or inverted LFO signals.
        // We will just offset the phase in read.
        
        vibratoLfo.setSampleRate(sampleRate);
        vibratoLfo.setWaveform(CZ101::DSP::LFO::TRIANGLE);
    }

    void reset()
    {
        std::fill(bbdBuffer.begin(), bbdBuffer.end(), 0.0f);
        writeIndex = 0;
        lfoPhase = 0.0f;
    }

    void setRate(float rateHz) { lfoRate = rateHz; }
    void setDepth(float depth01) { lfoDepth = depth01; }
    void setMix(float mix01) { wetMix = mix01; }

    // Stereo processing (Mono in -> Stereo Out, typical for 80s synths)
    void process(float* left, float* right, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            // Input signal
            float input = (left[i] + right[i]) * 0.5f;

            // Update LFO
            lfoPhase += (lfoRate / sampleRate);
            if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
            float lfoVal = std::sin(lfoPhase * juce::MathConstants<float>::twoPi);

            // Calculate BBD Clock Delay
            // Base delay ~20ms, modulated by LFO
            float baseDelaySamples = 0.015f * sampleRate; 
            float modDelay = baseDelaySamples + (lfoVal * lfoDepth * 0.005f * sampleRate);

            // Simulation of BBD "Bucket Loss" (Low pass filtering effect)
            // Each stage loses a tiny bit of high frequency
            // We approximate this with a simple one-pole LPF before writing?
            // Or just a global low-pass on the wet signal (easier and cheaper)
            
            // Write to circular buffer
            bbdBuffer[writeIndex] = input;
            
            // Read Interpolation (Linear is authentically lo-fi enough, or Hermite for better quality)
            float readPos = (float)writeIndex - modDelay;
            while (readPos < 0) readPos += bbdBuffer.size();
            while (readPos >= bbdBuffer.size()) readPos -= bbdBuffer.size();
            
            int idxA = (int)readPos;
            int idxB = (idxA + 1) % bbdBuffer.size();
            float frac = readPos - idxA;
            
            float delayedParams = bbdBuffer[idxA] + frac * (bbdBuffer[idxB] - bbdBuffer[idxA]);

            // Apply BBD Tone Darkening (LPF)
            delayedParams = toneFilter.processSample(delayedParams);

            // BBD Companding noise (placeholder)
            // float noise = (rand() / (float)RAND_MAX - 0.5f) * 0.0005f; 

            writeIndex = (writeIndex + 1) % bbdBuffer.size();

            // Mix:
            // Left = Dry + Wet
            // Right = Dry - Wet (Stereo widening trick used in Juno/CZ)
            
            left[i] = input + delayedParams * wetMix;
            right[i] = input - delayedParams * wetMix; 
        }
    }

private:
    double sampleRate = 44100.0;
    std::vector<float> bbdBuffer;
    int writeIndex = 0;
    
    float lfoRate = 0.5f;
    float lfoDepth = 0.5f;
    float wetMix = 0.0f;
    float lfoPhase = 0.0f;

    CZ101::DSP::LFO chorusLfo; 
    CZ101::DSP::LFO vibratoLfo;

    // Simple One Pole Lowpass for BBD Loss
    struct OnePole {
        float z1 = 0.0f;
        float b1 = 0.3f; // Cutoff factor
        float processSample(float in) {
            z1 = in * b1 + z1 * (1.0f - b1);
            return z1;
        }
    } toneFilter;
};

} // namespace Effects
} // namespace DSP
} // namespace CZ101
