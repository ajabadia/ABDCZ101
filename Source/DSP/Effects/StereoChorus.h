#pragma once

#include <JuceHeader.h>
// Corregido para usar una ruta de inclusión directa desde la raíz de 'Source'
#include "Utils/CircularBuffer.h"
#include "../Modulation/LFO.h"

namespace CZ101 {
namespace DSP {
namespace Effects {

class StereoChorus {
public:
    StereoChorus();

    void prepare(double sampleRate);
    void reset(); // Audit Fix 1.2: Reset RNG/Phase
    void process(float* left, float* right, int numSamples);
    
    void setRate(float rateHz);
    void setDepth(float depth);
    void setMix(float mix);

private:
    double sr = 44100.0;
    LFO lfoL, lfoR;
    Utils::CircularBuffer<float> delayBufferL, delayBufferR;
    float rate = 1.0f;
    float depth = 0.5f;
    float mix = 0.5f;
};

} // namespace Effects
} // namespace DSP
} // namespace CZ101
