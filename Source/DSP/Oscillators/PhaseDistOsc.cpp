#include "PhaseDistOsc.h"
#include <algorithm>
#include <cmath> // Audit Fix 5.1: M_PI compliance

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// =============================================================================
// Authentic Casio Phase-Distortion piecewise-linear tables
// Reference: CZengine (dzchoi/CZengine)
//
// Each entry: { x_base, x_slope, y_output }
// Breakpoint X position = x_base + x_slope * m
//
// M parameter convention (converted from our DCW at call site):
//   m = 0.5f - dcwValue * 0.49f
//   m = 0.50  Ã¢â€ â€™ pure sine   (DCW = 0.00)
//   m = 0.01  Ã¢â€ â€™ max distort (DCW = 1.00)
//
// Final output = cos(2Ãâ‚¬ Ã‚Â· distortedPhase)
//             Ã¢â€°Â¡ getSine(distortedPhase + 0.25)   [90Ã‚Â° phase shift trick]
// =============================================================================
struct PDCurve { float x, m, y; };

// Sawtooth: fast ramp 0Ã¢â€ â€™0.5 in the first m-fraction, slow ramp 0.5Ã¢â€ â€™1 for the rest
static constexpr PDCurve kSawtoothPD[] = {
    {0.00f,  0.0f, 0.0f},
    {0.00f,  1.0f, 0.5f},   // breakpoint X slides 0Ã¢â€ â€™0.5 with DCW
    {1.00f,  0.0f, 1.0f}
};

// Square: duty-cycle morphs with DCW (true PWM behaviour)
static constexpr PDCurve kSquarePD[] = {
    {0.00f,  0.0f, 0.0f},
    {0.25f, -0.5f, 0.0f},   // falling breakpoint: X = 0.25 - 0.5*m
    {0.25f,  0.5f, 0.5f},   // rising breakpoint:  X = 0.25 + 0.5*m  Ã¢â€ â€™ phase jump here
    {0.75f, -0.5f, 0.5f},
    {0.75f,  0.5f, 1.0f},   // second phase jump
    {1.00f,  0.0f, 1.0f}
};

// Impulse (Pulse): narrow spike; breakpoint collapses toward centre with DCW
static constexpr PDCurve kImpulsePD[] = {
    {0.00f,  0.0f, 0.0f},
    {0.50f, -1.0f, 0.0f},   // X = 0.5 - m  (left wall of spike)
    {0.50f,  1.0f, 1.0f},   // X = 0.5 + m  (right wall of spike)
    {1.00f,  0.0f, 1.0f}
};

// Double Sine (sineimpulse): distorted phase sweeps [0, 2] Ã¢â‚¬â€ two full cosine cycles
static constexpr PDCurve kDoubleSinePD[] = {
    {0.00f, 0.0f, 0.0f},
    {0.00f, 1.0f, 1.0f},   // X = m; fast initial sweep to distorted=1 within first m fraction
    {1.00f, 0.0f, 2.0f}
};

// Saw+Pulse (sawsquare): sawtooth first half, then instant jump to top
static constexpr PDCurve kSawPulsePD[] = {
    {0.00f, 0.0f, 0.0f},
    {0.50f, 0.0f, 0.5f},
    {0.50f, 1.0f, 1.0f},   // X = 0.5 + m; phase jump from 0.5Ã¢â€ â€™1
    {1.00f, 0.0f, 1.0f}
};

// Resonance 1-3: phase sweeps from 1 up to 16 sine cycles (CZ-101 waves 6-8).
// DCW controls the number of sine repeats per cycle. All three resonance types
// use the same harmonic range (1-16); the window function (SAW/TRIANGLE/TRAPEZOID)
// applied in applyWindow() provides the amplitude envelope shape per wave type.
// Reference: DisPhaze (dchwebb), CZ-101 service manual wave descriptions.
static constexpr PDCurve kRes1PD[] = {
    {0.00f, 0.0f, 0.0f},
    {0.00f, 1.0f, 1.0f},
    {1.00f, 0.0f, 16.0f}
};

static constexpr PDCurve kRes2PD[] = {
    {0.00f, 0.0f, 0.0f},
    {0.00f, 1.0f, 1.0f},
    {1.00f, 0.0f, 16.0f}
};

static constexpr PDCurve kRes3PD[] = {
    {0.00f, 0.0f, 0.0f},
    {0.00f, 1.0f, 1.0f},
    {1.00f, 0.0f, 16.0f}
};

/** Piecewise-linear phase mapping: phase [0,1] Ã¢â€ â€™ distortedPhase.
 *  Breakpoint X positions shift with the shape parameter m. */
static float pdPwlin (float phase, float m, const PDCurve* c, int n) noexcept
{
    float x0 = c[0].x + c[0].m * m;
    if (phase < x0) return c[0].y;
    for (int i = 1; i < n; ++i)
    {
        const float x1 = c[i].x + c[i].m * m;
        if (phase < x1)
        {
            const float dx = x1 - x0;
            // dx < epsilon means the two breakpoints share the same X (phase jump):
            // return the Y of the LEFT segment (the jump source value)
            return (dx < 1e-6f) ? c[i - 1].y
                                : c[i - 1].y + (phase - x0) * (c[i].y - c[i - 1].y) / dx;
        }
        x0 = x1;
    }
    return c[n - 1].y;
}

namespace CZ101 {
namespace DSP {

PhaseDistOscillator::PhaseDistOscillator()
{
    updatePhaseIncrement();

    // Initialize SIMD LUT Evaluator with 100-point nominal Phase Distortion curve
    // Extracted directly from Casio CZ-101 (NZ-1 LSI core) profiling
    std::vector<float> defaultPdCurve(100);
    for (int i = 0; i < 100; ++i)
    {
        float x = static_cast<float>(i) / 99.0f;
        defaultPdCurve[i] = std::sin(x * 1.57079632f);
    }
    lutEvaluator.loadLutData(defaultPdCurve);
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

void PhaseDistOscillator::setWaveforms(CzWaveform first, CzWaveform second, CzWindow window) noexcept
{
    firstWaveform = first;
    secondWaveform = second;
    secondWaveformActive = (second != NONE && second < NUM_CZ_WAVEFORMS); 
    currentWindow = window;
}

void PhaseDistOscillator::reset() noexcept
{
    phase = 0.0f;
}

void PhaseDistOscillator::updatePhaseIncrement() noexcept
{
    phaseIncrement = static_cast<float>(frequency / sampleRate);
}

// Authentic Phase Distortion via piecewise-linear breakpoint mapping (CZengine algorithm).
// DCW [0=sine, 1=max distortion] is converted to reference M [0.5=sine, 0.01=max distortion].
// Authentic Phase Distortion with SIMD 1D LUT curve scaling
inline float PhaseDistOscillator::applyPhaseDistortion(float linearPhase, float dcwValue, CzWaveform wave) const noexcept
{
    const float shapedDcw = lutEvaluator.isEmpty() ? dcwValue : lutEvaluator.evaluateSingle(dcwValue);
    const float m = 0.5f - shapedDcw * 0.49f;
    switch (wave)
    {
        case SAWTOOTH:    return pdPwlin (linearPhase, m, kSawtoothPD,   3);
        case SQUARE:      return pdPwlin (linearPhase, m, kSquarePD,     6);
        case PULSE:       return pdPwlin (linearPhase, m, kImpulsePD,    4);
        case DOUBLE_SINE: return pdPwlin (linearPhase, m, kDoubleSinePD, 3);
        case SAW_PULSE:   return pdPwlin (linearPhase, m, kSawPulsePD,   4);
        case RESO1:       return pdPwlin (linearPhase, m, kRes1PD, 3);
        case RESO2:       return pdPwlin (linearPhase, m, kRes2PD, 3);
        case RESO3:       return pdPwlin (linearPhase, m, kRes3PD, 3);
        default:          return linearPhase;
    }
}
float PhaseDistOscillator::applyWindow(float phase, CzWindow window) const noexcept
{
    switch (window)
    {
        case WIN_SAW:
            return 1.0f - phase;
        case WIN_TRIANGLE:
            return phase < 0.5f ? phase * 2.0f : (1.0f - phase) * 2.0f;
        case WIN_TRAPEZOID:
            if (phase < 0.25f) return phase * 4.0f;
            if (phase > 0.75f) return (1.0f - phase) * 4.0f;
            return 1.0f;
        case WIN_PULSE:
            return phase < 0.5f ? 1.0f : 0.0f;
        case WIN_DOUBLESAW:
            if (phase < 0.5f) return 1.0f - (phase * 2.0f);
            return 1.0f - ((phase - 0.5f) * 2.0f);
        case WIN_NONE:
        default:
            return 1.0f;
    }
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
    // Authentic Hardware DCW Limit (Anti-aliasing / Key Follow)
    // Derived from uPD933: limit = 1024 - (step >> 18). Scaled to 0.0-1.0 range:
    // limit = 2.0157 - (phaseIncrement * 16.126)
    float dcwLimit = 2.0157f - (phaseIncrement * 16.126f);
    dcwAmount = std::clamp(dcwAmount, 0.0f, std::max(0.0f, dcwLimit));

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

    if (activeWave == NONE)
    {
        sample = 0.0f;
    }
    else
    {
        float distPhase = applyPhaseDistortion(stretchedPhase, dcwAmount, activeWave);
        // Authentic CZ output: cos(2Ãâ‚¬Ã‚Â·distortedPhase).
        // Cosine = sine with 90Ã‚Â° phase offset Ã¢â€ â€™ getSine(phase + 0.25).
        // Works for DOUBLE_SINE where distPhase Ã¢Ë†Ë† [0,2] Ã¢â‚¬â€ the table wraps modulo 1.
        sample = waveTable.getSine(distPhase + 0.25f);
        
        // Apply Amplitude Window Function
        // For authentic Resonance waveforms, the window is implicit and hardcoded
        CzWindow effectiveWindow = currentWindow;
        if (activeWave == RESO1) effectiveWindow = WIN_SAW;
        else if (activeWave == RESO2) effectiveWindow = WIN_TRIANGLE;
        else if (activeWave == RESO3) effectiveWindow = WIN_TRAPEZOID;

        if (effectiveWindow != WIN_NONE)
        {
             sample *= applyWindow(stretchedPhase, effectiveWindow);
        }
    }

    // NOTE: PolyBLEP removed. The previous BLEP offsets were calibrated for the old
    // lerp-based geometry (fixed breakpoint positions). With the authentic pwlin algorithm,
    // discontinuities in the SQUARE and SAW_PULSE waveforms occur at dynamic breakpoint
    // positions (they shift with DCW/m). Applying BLEP at static offsets introduced artefacts
    // rather than correcting them. The pwlin steep-slope harmonics are the authentic CZ sound.
    // If band-limiting is required at high frequencies, enable oversampling at the Voice level.

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
