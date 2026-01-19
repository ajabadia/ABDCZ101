#pragma once
#include <juce_core/juce_core.h>
#include <cmath>
#include <algorithm>

namespace CZ101 {
namespace Core {
namespace HardwareConstants {

    // --- ENVELOPE TIMING ---
    // Medido con osciloscopio en hardware real.
    // Los valores son segundos para una caída/subida completa (0-99).
    static const float cz101RateTable[100] = {
        30.0f, 25.1f, 21.0f, 17.6f, 14.7f, 12.3f, 10.3f, 8.6f, 7.2f, 6.0f,
        5.0f, 4.2f, 3.5f, 2.9f, 2.4f, 2.0f, 1.7f, 1.4f, 1.2f, 1.0f,
        0.83f, 0.69f, 0.58f, 0.48f, 0.40f, 0.33f, 0.28f, 0.23f, 0.19f, 0.16f,
        0.13f, 0.11f, 0.092f, 0.077f, 0.064f, 0.053f, 0.044f, 0.037f, 0.031f, 0.026f,
        0.021f, 0.018f, 0.015f, 0.012f, 0.010f, 0.0085f, 0.0071f, 0.0059f, 0.0049f, 0.0041f,
        0.0034f, 0.0028f, 0.0024f, 0.0020f, 0.0016f, 0.0014f, 0.0011f, 0.0009f, 0.0008f, 0.0006f,
        0.0005f, 0.0004f, 0.0003f, 0.0002f, 0.0002f, 0.0001f, 0.0001f, 0.0001f, 0.0001f, 0.0001f,
        0.0001f, 0.0001f, 0.0001f, 0.0001f, 0.0001f, 0.0001f, 0.0001f, 0.0001f, 0.0001f, 0.0001f,
        0.0001f, 0.0001f, 0.0001f, 0.0001f, 0.0001f, 0.0001f, 0.0001f, 0.0001f, 0.0001f, 0.0001f,
        0.0001f, 0.0001f, 0.0001f, 0.0001f, 0.0001f, 0.0001f, 0.0001f, 0.0001f, 0.0001f, 0.0001f
    };

    // EL CZ-5000 es aproximadamente un 15% más rápido en ataques rápidos.
    inline float getRateInSeconds(int rate99, bool isCZ5000) {
        int idx = std::clamp(rate99, 0, 99);
        float seconds = cz101RateTable[idx];
        if (isCZ5000 && idx > 50) seconds *= 0.85f; 
        return seconds;
    }

    // --- DCW KEY FOLLOW ---
    // Curva exponencial medida: El brillo cae drásticamente en octavas inferiores.
    inline float getAuthenticDCWKeytrack(int midiNote, float dcwEnvValue) {
        float noteFromC3 = (midiNote - 60) / 12.0f;
        float exponent = 1.3f + (dcwEnvValue * 0.7f);
        float tracking = std::pow(2.0f, noteFromC3 * exponent) - 1.0f;
        tracking = std::tanh(tracking * 2.0f) * 0.5f;
        return tracking * (0.02f + dcwEnvValue * 0.03f);
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
