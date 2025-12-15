#pragma once

#include <cmath>
#include <algorithm>

namespace CZ101 {
namespace Utils {

class DSPHelpers
{
public:
    static float dbToGain(float db)
    {
        return std::pow(10.0f, db / 20.0f);
    }
    
    static float gainToDb(float gain)
    {
        return 20.0f * std::log10(std::max(gain, 0.00001f));
    }
    
    static float midiNoteToFrequency(int midiNote)
    {
        return 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);
    }
    
    static int frequencyToMidiNote(float frequency)
    {
        return static_cast<int>(69 + 12 * std::log2(frequency / 440.0f));
    }
    
    static float lerp(float a, float b, float t)
    {
        return a + t * (b - a);
    }
    
    static float clamp(float value, float min, float max)
    {
        return std::clamp(value, min, max);
    }
    
    static float mapRange(float value, float inMin, float inMax, float outMin, float outMax)
    {
        return outMin + (value - inMin) * (outMax - outMin) / (inMax - inMin);
    }
};

} // namespace Utils
} // namespace CZ101
