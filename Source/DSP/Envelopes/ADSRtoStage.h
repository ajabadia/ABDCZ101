#pragma once

// Based on TinyADSR (Public Domain) and CZ-101 Logic
// Licensed under MIT

#include <cmath>
#include <array>
#include <algorithm>

namespace CZ101 {
namespace DSP {

/**
 * @brief Conversión ADSR → 8-stage envelope rates
 * 
 * Mapea parámetros ADSR (milisegundos) a coeficientes de stage
 * usando aproximación logarítmica basada en tiempo de caída 60dB
 */
struct ADSRtoStageConverter {
    
    /**
     * @brief Convertir ADSR a 8 stages
     * 
     * @param attackMs      Attack time (0-8000ms)
     * @param decayMs       Decay time (0-8000ms)
     * @param sustainLevel  Sustain level (0-1.0)
     * @param releaseMs     Release time (0-8000ms)
     * @param outRates      [OUT] array de 8 coeficientes de rate
     * @param outLevels     [OUT] array de 8 niveles objetivo
     * @param outSustainPoint [OUT] índice de sustain
     * @param outEndPoint   [OUT] índice de fin
     * @param sampleRate    Sample rate (default 44100)
     */
    static void convertADSR(
        float attackMs,
        float decayMs,
        float sustainLevel,
        float releaseMs,
        std::array<float, 8>& outRates,
        std::array<float, 8>& outLevels,
        int& outSustainPoint,
        int& outEndPoint,
        double sampleRate = 44100.0
    ) {
        // Validar y clampear inputs
        attackMs = std::clamp(attackMs, 0.0f, 8000.0f);
        decayMs = std::clamp(decayMs, 0.0f, 8000.0f);
        sustainLevel = std::clamp(sustainLevel, 0.0f, 1.0f);
        releaseMs = std::clamp(releaseMs, 0.0f, 8000.0f);
        
        // Internal helper: convert milliseconds -> MultiStageEnvelope Rate (0.0 - 1.0)
        // Inverse of getRateInSeconds: norm = ((sec - 0.003) / 25.0)^(1/4.5)
        auto msToRate = [](float ms) -> float {
            float sec = std::clamp(ms / 1000.0f, 0.003f, 25.0f);
            float norm = std::pow((sec - 0.003f) / 25.0f, 1.0f / 4.5f);
            return std::clamp(1.0f - norm, 0.0f, 1.0f);
        };
        
        // ===== STAGE 0: ATTACK =====
        // Rampa desde 0 hacia 1.0
        outRates[0] = msToRate(attackMs);
        outLevels[0] = 1.0f;
        
        // ===== STAGE 1: DECAY =====
        // Rampa desde 1.0 hacia sustain
        outRates[1] = msToRate(decayMs);
        outLevels[1] = sustainLevel;
        
        // ===== STAGE 2: SUSTAIN HOLD =====
        // Mantiene nivel de sustain (sin decaimiento)
        outRates[2] = 0.99f;
        outLevels[2] = sustainLevel;
        
        // ===== STAGE 3: RELEASE =====
        // Rampa desde sustain hacia 0
        outRates[3] = msToRate(releaseMs);
        outLevels[3] = 0.0f;
        
        // ===== STAGES 4-7: UNUSED =====
        for (int i = 4; i < 8; ++i) {
            outRates[i] = 0.99f;
            outLevels[i] = 0.0f;
        }
        
        // Puntos de control de sustain y fin
        outSustainPoint = 2;  // Sustain en stage 2
        outEndPoint = 3;      // Release en stage 3
    }
};

} // namespace DSP
} // namespace CZ101
