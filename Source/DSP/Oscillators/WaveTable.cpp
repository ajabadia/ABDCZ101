#include "WaveTable.h"
#include <algorithm>

namespace CZ101 {
namespace DSP {

WaveTable::WaveTable()
{
    generateTables();
}

void WaveTable::generateTables()
{
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 2.0f * PI;
    
    for (int i = 0; i < TABLE_SIZE; ++i)
    {
        const float phase = static_cast<float>(i) / static_cast<float>(TABLE_SIZE);
        
        // Sine wave: Perfect, no aliasing
        sineTable[i] = std::sin(TWO_PI * phase);
        
        // Sawtooth: Naive version (PolyBLEP applied at render time)
        sawtoothTable[i] = 2.0f * phase - 1.0f;
        
        // Square: Naive version (PolyBLEP applied at render time)
        squareTable[i] = (phase < 0.5f) ? 1.0f : -1.0f;
        
        // Triangle: Continuous waveform
        if (phase < 0.25f)
            triangleTable[i] = 4.0f * phase;
        else if (phase < 0.75f)
            triangleTable[i] = 2.0f - 4.0f * phase;
        else
            triangleTable[i] = 4.0f * phase - 4.0f;
    }
}

float WaveTable::getSine(float phase) const noexcept
{
    return interpolate(sineTable, phase);
}

float WaveTable::getSawtooth(float phase) const noexcept
{
    return interpolate(sawtoothTable, phase);
}

float WaveTable::getSquare(float phase) const noexcept
{
    return interpolate(squareTable, phase);
}

float WaveTable::getTriangle(float phase) const noexcept
{
    return interpolate(triangleTable, phase);
}

float WaveTable::getPulse(float phase, float width) const noexcept
{
    // Pulse wave with variable width
    // width = 0.5 is square wave
    phase = phase - std::floor(phase);
    width = std::clamp(width, 0.1f, 0.9f);
    return (phase < width) ? 1.0f : -1.0f;
}

float WaveTable::getDoubleSine(float phase) const noexcept
{
    // Two sine waves, one octave apart
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 2.0f * PI;
    
    phase = phase - std::floor(phase);
    float fundamental = std::sin(TWO_PI * phase);
    float octave = std::sin(TWO_PI * phase * 2.0f);
    
    return (fundamental + octave * 0.5f) / 1.5f;  // Normalize
}

float WaveTable::getHalfSine(float phase) const noexcept
{
    // Sine wave rectified (only positive half)
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 2.0f * PI;
    
    phase = phase - std::floor(phase);
    float sine = std::sin(TWO_PI * phase);
    
    return (sine > 0.0f) ? sine : 0.0f;
}

float WaveTable::getResonantSaw(float phase) const noexcept
{
    // Sawtooth with emphasized harmonics (resonant character)
    phase = phase - std::floor(phase);
    
    float saw = 2.0f * phase - 1.0f;
    
    // Add harmonic emphasis
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 2.0f * PI;
    float harmonic = std::sin(TWO_PI * phase * 3.0f) * 0.3f;
    
    return std::clamp(saw + harmonic, -1.0f, 1.0f);
}

float WaveTable::getResonantTriangle(float phase) const noexcept
{
    // Triangle with emphasized harmonics
    phase = phase - std::floor(phase);
    
    float tri;
    if (phase < 0.25f)
        tri = 4.0f * phase;
    else if (phase < 0.75f)
        tri = 2.0f - 4.0f * phase;
    else
        tri = 4.0f * phase - 4.0f;
    
    // Add harmonic emphasis
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 2.0f * PI;
    float harmonic = std::sin(TWO_PI * phase * 5.0f) * 0.2f;
    
    return std::clamp(tri + harmonic, -1.0f, 1.0f);
}

float WaveTable::getTrapezoid(float phase) const noexcept
{
    // Trapezoid wave (between square and triangle)
    phase = phase - std::floor(phase);
    
    constexpr float riseTime = 0.15f;   // 15% rise
    constexpr float fallTime = 0.15f;   // 15% fall
    constexpr float highTime = 0.35f;   // 35% high
    // lowTime = 0.35f (35% low) - implicit in else branch
    
    if (phase < riseTime)
        return (phase / riseTime) * 2.0f - 1.0f;  // Rising
    else if (phase < riseTime + highTime)
        return 1.0f;  // High
    else if (phase < riseTime + highTime + fallTime)
        return 1.0f - ((phase - riseTime - highTime) / fallTime) * 2.0f;  // Falling
    else
        return -1.0f;  // Low
}

float WaveTable::interpolate(const std::array<float, TABLE_SIZE>& table, float phase) const noexcept
{
    // Fast wrap phase to [0.0, 1.0)
    if (phase >= 1.0f) phase -= std::floor(phase);
    else if (phase < 0.0f) phase += 1.0f - std::floor(phase);
    
    // Ensure strictly in range [0, 1)
    if (phase >= 1.0f) phase = 0.0f;
    if (phase < 0.0f) phase = 0.0f;

    // Convert to table index
    const float indexFloat = phase * static_cast<float>(TABLE_SIZE);
    const int index0 = static_cast<int>(indexFloat) & (TABLE_SIZE - 1);
    const int index1 = (index0 + 1) & (TABLE_SIZE - 1); // Power of 2 mask
    
    // Linear interpolation
    const float frac = indexFloat - static_cast<float>(static_cast<int>(indexFloat));
    return table[index0] + frac * (table[index1] - table[index0]);
}

} // namespace DSP
} // namespace CZ101
