#pragma once

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
        
        // Función interna: convertir milisegundos → coeficiente de decay
        auto msToRateCoeff = [sampleRate](float ms) -> float {
            if (ms < 1.0f) ms = 1.0f;
            if (ms > 8000.0f) ms = 8000.0f;
            
            // Convertir a segundos
            float sec = ms / 1000.0f;
            
            // Exponential decay: e^(-k*t)
            // Para 60dB en tiempo T: k = 5.5 / T
            float k = 5.5f / (sec * static_cast<float>(sampleRate));
            
            // Coeficiente per-sample: rate = e^(-k)
            float coeff = std::exp(-k);
            
            // Clamp para estabilidad numérica
            return std::clamp(coeff, 0.001f, 0.99f);
        };
        
        // ===== STAGE 0: ATTACK =====
        // Rampa desde 0 hacia 1.0
        outRates[0] = msToRateCoeff(attackMs);
        outLevels[0] = 1.0f;
        
        // ===== STAGE 1: DECAY =====
        // Rampa desde 1.0 hacia sustain
        outRates[1] = msToRateCoeff(decayMs);
        outLevels[1] = sustainLevel;
        
        // ===== STAGE 2: SUSTAIN HOLD =====
        // Mantiene nivel de sustain (sin decaimiento)
        outRates[2] = 0.99f;
        outLevels[2] = sustainLevel;
        
        // ===== STAGE 3: RELEASE =====
        // Rampa desde sustain hacia 0
        outRates[3] = msToRateCoeff(releaseMs);
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
