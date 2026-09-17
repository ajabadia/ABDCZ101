#pragma once
#include <juce_core/juce_core.h>
#include <cmath>
#include <algorithm>

namespace CZ101 {
namespace Core {
namespace HardwareConstants {

    // --- ENVELOPE TIMING ---
    // Authentic hardware CZ (0-99) timing mapping
    // El chip µPD933 convierte el rate a formato punto flotante 7-bit (exponente y mantisa).
    inline float getRateInSeconds(int rate99, int envType, bool isCZ5000) {
        int idx = std::clamp(rate99, 0, 99);
        
        int hw_val = 0;
        if (envType == 1) { // DCW
            hw_val = (idx * 119) / 99 + 8;
        } else if (envType == 2) { // DCO (Pitch)
            hw_val = (idx * 127) / 99;
        } else { // DCA (Amp)
            hw_val = (idx * 119) / 99;
        }
        
        hw_val = std::clamp(hw_val, 0, 127);
        
        int exponent = hw_val >> 3;
        int mantissa = hw_val & 7;
        uint32_t rate_step = (8 + mantissa) << exponent;
        
        uint32_t target_max = 127 << 18; // DCA y DCW max target
        if (envType == 2) target_max = 63 << 16; // DCO max target (63 << (11 + 5))
        
        double samples = (double)target_max / (double)rate_step;
        double sr = 35714.28; // CZ-101 internal sample rate (112 clocks per sample @ 4MHz)
        float seconds = (float)(samples / sr);
        
        // Remove artificial division by 8; the hardware formula naturally gives the authentic punch.
        if (isCZ5000 && idx > 50) seconds *= 0.85f; 
        return seconds;
    }

    // --- DCW KEY FOLLOW ---
    // The authentic hardware DCW Key Follow / anti-aliasing limit is now computed
    // dynamically in PhaseDistOscillator::renderNextSample using the exact hardware formula:
    // limit = 1024 - (step >> 18)
    // However, the legacy heuristic curve is kept here for Mod Matrix Source 11.
    inline float getAuthenticDCWKeytrack(int midiNote, float dcwEnvValue, int mode = 2) {
        float noteFromC3 = (midiNote - 60) / 12.0f;
        
        if (mode == 1) {
            float tracking = std::pow(2.0f, noteFromC3 * 1.5f) - 1.0f;
            tracking = std::tanh(tracking * 2.0f) * 0.5f;
            return tracking * 0.15f; 
        } else {
            float exponent = 1.3f + (dcwEnvValue * 0.7f);
            float tracking = std::pow(2.0f, noteFromC3 * exponent) - 1.0f;
            tracking = std::tanh(tracking * 2.0f) * 0.5f;
            return tracking * (0.06f + dcwEnvValue * 0.09f); 
        }
    }

    // --- DAC COMPRESSION ---
    // Simula compresión de salida del DAC de 12 bits + Preamp analógico.
    inline float applyDACCompression(float digitalLevel) {
        if (digitalLevel > 0.85f) {
            float excess = digitalLevel - 0.85f;
            return 0.85f + excess * 0.3f;
        }
        return digitalLevel;
    }

    // --- NOISE CONSTANTS ---
    constexpr float DAC_NOISE_FLOOR = 0.0001f;
    constexpr float VOICE_MULTIPLEX_NOISE = 0.00005f;
    constexpr float KEY_CLICK_SPIKE = 0.0008f;

} // namespace HardwareConstants
} // namespace Core
} // namespace CZ101
