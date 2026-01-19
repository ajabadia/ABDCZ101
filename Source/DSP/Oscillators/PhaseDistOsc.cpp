#include "PhaseDistOsc.h"
#include <algorithm>
#include <cmath> // Audit Fix 5.1: M_PI compliance

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

void PhaseDistOscillator::setWaveforms(CzWaveform first, CzWaveform second) noexcept
{
    // Audit Fix 11.1: Explicit logic
    // If second is NONE or invalid, it is disabled.
    secondWaveform = second;
    secondWaveformActive = (second != NONE && second < NUM_CZ_WAVEFORMS); 
}

void PhaseDistOscillator::reset() noexcept
{
    phase = 0.0f;
}

void PhaseDistOscillator::updatePhaseIncrement() noexcept
{
    phaseIncrement = static_cast<float>(frequency / sampleRate);
}

// Helper for applying PD
// Now static-like, takes waveform as arg
// Forced inline for performance in the hot path
inline float PhaseDistOscillator::applyPhaseDistortion(float linearPhase, float dcwValue, CzWaveform wave) noexcept
{
    float distortedPhase = linearPhase;

    switch (wave)
    {
        case SAWTOOTH:
        {
            float distorted = (linearPhase < Constants::HalfPhase) ? (linearPhase * 2.0f) : 1.0f;
            distortedPhase = linearPhase + (distorted - linearPhase) * dcwValue;
            break;
        }
        
        case SQUARE:
        {
            float distorted = (linearPhase < Constants::HalfPhase) ? Constants::QuarterPhase : Constants::ThreeQuarterPhase;
            distortedPhase = linearPhase + (distorted - linearPhase) * dcwValue;
            break;
        }

        case PULSE:
        {
            float distorted = (linearPhase < Constants::QuarterPhase) ? Constants::QuarterPhase : Constants::ThreeQuarterPhase;
            distortedPhase = linearPhase + (distorted - linearPhase) * dcwValue;
            break;
        }

        case DOUBLE_SINE:
        {
            float distorted = linearPhase * 2.0f;
            distortedPhase = linearPhase + (distorted - linearPhase) * dcwValue;
            break;
        }

        case SAW_PULSE:
        {
            float distorted = (linearPhase < Constants::HalfPhase) ? (linearPhase * 2.0f) : Constants::ThreeQuarterPhase;
            distortedPhase = linearPhase + (distorted - linearPhase) * dcwValue;
            break;
        }

        case RESONANCE_1:
        case RESONANCE_2:
        case RESONANCE_3:
        {
            float modFreq = (wave == RESONANCE_1) ? Constants::Resonance1Freq : (wave == RESONANCE_2 ? Constants::Resonance2Freq : Constants::Resonance3Freq);
            float maxMod = (wave == RESONANCE_1) ? Constants::Resonance1MaxMod : (wave == RESONANCE_2 ? Constants::Resonance2MaxMod : Constants::Resonance3MaxMod);
            
            // Optimization: Use WaveTable for modulation sine instead of sin()
            float sineMod = waveTable.getSine(linearPhase * modFreq);
            float distorted = linearPhase + sineMod * maxMod;
            distortedPhase = linearPhase + (distorted - linearPhase) * dcwValue;
            break;
        }

        default:
            break;
    }

    // Optimization: Fast wrapping (distortedPhase range is tight)
    if (distortedPhase >= 1.0f) distortedPhase -= 1.0f;
    else if (distortedPhase < 0.0f) distortedPhase += 1.0f;

    return distortedPhase;
}

// Helper for PolyBLEP
float PhaseDistOscillator::polyBLEP(float t, float dt) const noexcept
{
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    else if (t > 1.0f - dt) {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

float PhaseDistOscillator::renderNextSample(float dcwAmount, bool* outDidWrap) noexcept
{
    float sample = 0.0f;
    float stretchedPhase = phase;
    CzWaveform activeWave = firstWaveform;

    if (secondWaveformActive)
    {
        // Half-period switching: 0.0-0.5 is Wave 1, 0.5-1.0 is Wave 2
        if (phase < 0.5f)
        {
            stretchedPhase = phase * 2.0f;
            activeWave = firstWaveform;
        }
        else
        {
            stretchedPhase = (phase - 0.5f) * 2.0f;
            activeWave = secondWaveform;
        }
    }

    // Apply PD to the selected (and potentially stretched) phase
    float distPhase = applyPhaseDistortion(stretchedPhase, dcwAmount, activeWave);
    sample = waveTable.getSine(distPhase);

    // Apply BLEP (Anti-aliasing)
    // For stretched phase, we need adjusted dt
    float dt = phaseIncrement * (secondWaveformActive ? 2.0f : 1.0f);
    
    if (activeWave == SAWTOOTH)
    {
        sample -= polyBLEP(stretchedPhase, dt);
    }
    else if (activeWave == SQUARE)
    {
        sample += polyBLEP(stretchedPhase, dt);
        float ps = stretchedPhase + 0.5f; if(ps>=1.0f) ps-=1.0f;
        sample -= polyBLEP(ps, dt);
    }
    else if (activeWave == PULSE)
    {
        sample += polyBLEP(stretchedPhase, dt);
        float ps = stretchedPhase + 0.75f; if(ps>=1.0f) ps-=1.0f;
        sample -= polyBLEP(ps, dt);
    }
    else if (activeWave == SAW_PULSE)
    {
        sample += polyBLEP(stretchedPhase, dt);
        float ps = stretchedPhase + 0.5f; if(ps>=1.0f) ps-=1.0f;
        sample -= polyBLEP(ps, dt);
    }

    // Advance Phase
    phase += phaseIncrement;
    
    // Audit Fix 2.1: Robust Phase Wrapping for Hard Sync / High Pitch
    // Instead of if(phase >= 1.0f) phase -= 1.0f; we handle multiple wraps.
    // Ideally use std::fmod, but for performance in tight loop with known positive increment:
    if (phase >= 1.0f)
    {
        phase -= std::floor(phase); // Robust wrapping
        if (outDidWrap) *outDidWrap = true;
    }
    else
    {
        if (outDidWrap) *outDidWrap = false;
    }
    
    return sample;
}

} // namespace DSP
} // namespace CZ101
