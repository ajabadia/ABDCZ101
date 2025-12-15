#include "PhaseDistOsc.h"
#include <algorithm>

namespace CZ101 {
namespace DSP {

PhaseDistOscillator::PhaseDistOscillator()
{
    updatePhaseIncrement();
}

void PhaseDistOscillator::setSampleRate(double sr) noexcept
{
    sampleRate = sr;
    updatePhaseIncrement();
}

void PhaseDistOscillator::setFrequency(float freq) noexcept
{
    frequency = std::clamp(freq, 20.0f, 20000.0f);
    updatePhaseIncrement();
}

void PhaseDistOscillator::setWaveform(Waveform waveform) noexcept
{
    currentWaveform = waveform;
}

void PhaseDistOscillator::reset() noexcept
{
    phase = 0.0f;
}

void PhaseDistOscillator::updatePhaseIncrement() noexcept
{
    phaseIncrement = static_cast<float>(frequency / sampleRate);
}

float PhaseDistOscillator::renderNextSample(float dcwAmount) noexcept
{
    float sample = 0.0f;
    
    // Render based on current waveform
    switch (currentWaveform)
    {
        case SINE:
            sample = renderSine();
            break;
        case SAWTOOTH:
            sample = renderSawtooth();
            break;
        case SQUARE:
            sample = renderSquare();
            break;
        case TRIANGLE:
            sample = renderTriangle();
            break;
        default:
            sample = 0.0f;
    }

    // Phase Distortion Simulation (Timbre Modulation)
    // Mix between Pure Sine (fundamental) and Full Waveform based on DCW amount
    // This creates the characteristic "wah" or filter sweep sound of PD synthesis
    if (dcwAmount < 1.0f && currentWaveform != SINE)
    {
        float sineComp = renderSine();
        // Linear interpolation: sine -> full waveform
        sample = sineComp + (sample - sineComp) * dcwAmount;
    }
    
    // Advance phase
    phase += phaseIncrement;
    if (phase >= 1.0f)
        phase -= 1.0f;
    
    return sample;
}

float PhaseDistOscillator::renderSine() noexcept
{
    // Sine wave: Perfect, no aliasing, no PolyBLEP needed
    return waveTable.getSine(phase);
}

float PhaseDistOscillator::renderSawtooth() noexcept
{
    // Naive sawtooth
    float value = 2.0f * phase - 1.0f;
    
    // Apply PolyBLEP at discontinuity (phase wraps from 1.0 to 0.0)
    value -= polyBLEP(phase, phaseIncrement);
    
    return value;
}

float PhaseDistOscillator::renderSquare() noexcept
{
    // Naive square wave
    float value = (phase < 0.5f) ? 1.0f : -1.0f;
    
    // Apply PolyBLEP at rising edge (phase = 0.0)
    value += polyBLEP(phase, phaseIncrement);
    
    // Apply PolyBLEP at falling edge (phase = 0.5)
    float phaseShifted = phase + 0.5f;
    if (phaseShifted >= 1.0f)
        phaseShifted -= 1.0f;
    value -= polyBLEP(phaseShifted, phaseIncrement);
    
    return value;
}

float PhaseDistOscillator::renderTriangle() noexcept
{
    // Triangle: Continuous waveform, no PolyBLEP needed
    return waveTable.getTriangle(phase);
}

float PhaseDistOscillator::polyBLEP(float t, float dt) const noexcept
{
    // PolyBLEP: Polynomial Bandlimited Step
    // Reduces aliasing by smoothing discontinuities
    
    // t: normalized phase [0.0, 1.0]
    // dt: phase increment per sample (frequency/sampleRate)
    
    // Discontinuity at start of cycle (t near 0.0)
    if (t < dt)
    {
        t /= dt;
        // Polynomial: t^2 - 2t + 1
        return t + t - t * t - 1.0f;
    }
    // Discontinuity at end of cycle (t near 1.0)
    else if (t > 1.0f - dt)
    {
        t = (t - 1.0f) / dt;
        // Polynomial: t^2 + 2t + 1
        return t * t + t + t + 1.0f;
    }
    
    // No discontinuity, no correction needed
    return 0.0f;
}

} // namespace DSP
} // namespace CZ101
