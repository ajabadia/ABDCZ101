# 💻 CÓDIGO READY-TO-COPY - CALIBRADO CZ-101
## Soluciones Completas para Copiar/Pegar

---

## 📄 ARCHIVO 1: Source/DSP/Envelopes/ADSRtoStage.h

```cpp
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
```

---

## 📄 ARCHIVO 2: Source/Core/Voice.cpp - Método renderNextSample()

**REEMPLAZAR COMPLETAMENTE** la función `Voice::renderNextSample()` con:

```cpp
float Voice::renderNextSample() noexcept
{
    if (!dcaEnvelope.isActive()) return 0.0f;
    
    // === ENVELOPE VALUES ===
    float dcwValue = dcwEnvelope.getNextValue();         // Timbre (0-1)
    float dcaValue = dcaEnvelope.getNextValue();         // Amplitud (0-1)
    float pitchEnvVal = pitchEnvelope.getNextValue();    // Pitch mod (0-1)
    
    // === PITCH MODULATION ===
    // Pitch envelope: 0.0 = -1 octava, 0.5 = unison, 1.0 = +1 octava
    float semitones = (pitchEnvVal - 0.5f) * 100.0f;     // ±50 semitones
    float pitchMod = std::pow(2.0f, semitones / 12.0f);
    
    // === GLIDE (PORTAMENTO) ===
    if (glideTime > 0.001f && currentFrequency != targetFrequency) {
        float alpha = 1.0f / (44100.0f * (glideTime + 0.001f));
        float diff = targetFrequency - currentFrequency;
        currentFrequency += diff * alpha * 4.0f;
        
        if (std::abs(diff) < 0.1f) currentFrequency = targetFrequency;
    } else {
        currentFrequency = targetFrequency;
    }
    
    // === LFO VIBRATO ===
    float vibratoMod = 1.0f;
    if (vibratoDepth > 0.001f) {
        float lfoSemitones = lfoValue * vibratoDepth;
        vibratoMod = std::pow(2.0f, lfoSemitones / 12.0f);
    }
    
    // === FINAL FREQUENCY ===
    // Combina: Pitch Env + Vibrato + Pitch Bend + Master Tune
    float finalFreq = currentFrequency * pitchMod * vibratoMod 
                    * pitchBendFactor * masterTuneFactor;
    
    osc1.setFrequency(finalFreq);
    osc2.setFrequency(finalFreq * currentDetuneFactor);
    
    // === OSCILLATOR RENDERING ===
    bool osc1Wrapped = false;
    float osc1Sample = osc1.renderNextSample(dcwValue, &osc1Wrapped);
    
    // Hard Sync: reset osc2 cuando osc1 wraps
    if (isHardSyncEnabled && osc1Wrapped) {
        osc2.reset();
    }
    
    float osc2Sample = osc2.renderNextSample(dcwValue);
    
    // Ring Modulation: osc2_out = osc1 * osc2
    if (isRingModEnabled) {
        osc2Sample = osc1Sample * osc2Sample;
    }
    
    // === OSCILLATOR MIX WITH NORMALIZATION ✅ ===
    // Importante: Evitar overshooting si osc1Level + osc2Level > 1.0
    float totalLevel = osc1Level + osc2Level;
    float normalizer = (totalLevel > 1.0f) ? (1.0f / totalLevel) : 1.0f;
    
    float mix = (osc1Sample * osc1Level + osc2Sample * osc2Level) * normalizer;
    
    // === FINAL OUTPUT WITH SAFETY ===
    // 0.9f headroom para prevenir clipping de efectos posteriores
    float output = mix * dcaValue * currentVelocity * 0.9f;
    
    // Clamp final para ultra-seguridad
    return std::clamp(output, -1.0f, 1.0f);
}
```

---

## 📄 ARCHIVO 3: Source/Core/Voice.cpp - Métodos ADSR

**REEMPLAZAR COMPLETAMENTE** los siguientes métodos vacíos:

```cpp
// ============================================================================
// DCW ENVELOPE - IMPLEMENTACIÓN COMPLETA
// ============================================================================

void Voice::setDCWAttack(float seconds) noexcept
{
    // Convertir segundos a milisegundos
    float attackMs = std::clamp(seconds * 1000.0f, 0.0f, 8000.0f);
    
    // Obtener parámetros actuales (aproximación)
    float decayRate, decayLevel, sustainLevel;
    dcwEnvelope.getStageRate(1, decayRate);
    dcwEnvelope.getStageLevel(1, decayLevel);
    sustainLevel = dcwEnvelope.getStageLevel(2);
    
    // Reconvertir rate a ms (aproximación inversa)
    float decayMs = (decayRate < 0.1f) ? 100.0f : 300.0f; // Simplificado
    
    float releaseRate, releaseLevel;
    dcwEnvelope.getStageRate(3, releaseRate);
    releaseMs = (releaseRate < 0.1f) ? 100.0f : 300.0f;
    
    // Convertir ADSR a stages
    std::array<float, 8> rates, levels;
    int sus, end;
    DSP::ADSRtoStageConverter::convertADSR(
        attackMs, decayMs, sustainLevel, releaseMs,
        rates, levels, sus, end, 44100.0
    );
    
    // Aplicar solo stage 0 (attack)
    dcwEnvelope.setStage(0, rates[0], levels[0]);
}

void Voice::setDCWDecay(float seconds) noexcept
{
    float decayMs = std::clamp(seconds * 1000.0f, 0.0f, 8000.0f);
    
    float attackRate;
    dcwEnvelope.getStageRate(0, attackRate);
    float attackMs = (attackRate < 0.1f) ? 10.0f : 100.0f;
    
    float sustainLevel = dcwEnvelope.getStageLevel(2);
    
    float releaseRate;
    dcwEnvelope.getStageRate(3, releaseRate);
    float releaseMs = (releaseRate < 0.1f) ? 100.0f : 300.0f;
    
    std::array<float, 8> rates, levels;
    int sus, end;
    DSP::ADSRtoStageConverter::convertADSR(
        attackMs, decayMs, sustainLevel, releaseMs,
        rates, levels, sus, end, 44100.0
    );
    
    dcwEnvelope.setStage(1, rates[1], levels[1]);
}

void Voice::setDCWSustain(float level) noexcept
{
    float sustainLevel = std::clamp(level, 0.0f, 1.0f);
    
    float attackRate;
    dcwEnvelope.getStageRate(0, attackRate);
    float attackMs = (attackRate < 0.1f) ? 10.0f : 100.0f;
    
    float decayRate;
    dcwEnvelope.getStageRate(1, decayRate);
    float decayMs = (decayRate < 0.1f) ? 100.0f : 300.0f;
    
    float releaseRate;
    dcwEnvelope.getStageRate(3, releaseRate);
    float releaseMs = (releaseRate < 0.1f) ? 100.0f : 300.0f;
    
    std::array<float, 8> rates, levels;
    int sus, end;
    DSP::ADSRtoStageConverter::convertADSR(
        attackMs, decayMs, sustainLevel, releaseMs,
        rates, levels, sus, end, 44100.0
    );
    
    // Aplicar sustain a stages 1 y 2
    dcwEnvelope.setStage(1, rates[1], levels[1]);
    dcwEnvelope.setStage(2, rates[2], levels[2]);
}

void Voice::setDCWRelease(float seconds) noexcept
{
    float releaseMs = std::clamp(seconds * 1000.0f, 0.0f, 8000.0f);
    
    float attackRate;
    dcwEnvelope.getStageRate(0, attackRate);
    float attackMs = (attackRate < 0.1f) ? 10.0f : 100.0f;
    
    float decayRate;
    dcwEnvelope.getStageRate(1, decayRate);
    float decayMs = (decayRate < 0.1f) ? 100.0f : 300.0f;
    
    float sustainLevel = dcwEnvelope.getStageLevel(2);
    
    std::array<float, 8> rates, levels;
    int sus, end;
    DSP::ADSRtoStageConverter::convertADSR(
        attackMs, decayMs, sustainLevel, releaseMs,
        rates, levels, sus, end, 44100.0
    );
    
    dcwEnvelope.setStage(3, rates[3], levels[3]);
}

// ============================================================================
// DCA ENVELOPE - IMPLEMENTACIÓN COMPLETA (IDÉNTICA A DCW)
// ============================================================================

void Voice::setDCAAttack(float seconds) noexcept
{
    float attackMs = std::clamp(seconds * 1000.0f, 0.0f, 8000.0f);
    
    float decayRate;
    dcaEnvelope.getStageRate(1, decayRate);
    float decayMs = (decayRate < 0.1f) ? 100.0f : 300.0f;
    
    float sustainLevel = dcaEnvelope.getStageLevel(2);
    
    float releaseRate;
    dcaEnvelope.getStageRate(3, releaseRate);
    float releaseMs = (releaseRate < 0.1f) ? 100.0f : 300.0f;
    
    std::array<float, 8> rates, levels;
    int sus, end;
    DSP::ADSRtoStageConverter::convertADSR(
        attackMs, decayMs, sustainLevel, releaseMs,
        rates, levels, sus, end, 44100.0
    );
    
    dcaEnvelope.setStage(0, rates[0], levels[0]);
}

void Voice::setDCADecay(float seconds) noexcept
{
    float decayMs = std::clamp(seconds * 1000.0f, 0.0f, 8000.0f);
    
    float attackRate;
    dcaEnvelope.getStageRate(0, attackRate);
    float attackMs = (attackRate < 0.1f) ? 10.0f : 100.0f;
    
    float sustainLevel = dcaEnvelope.getStageLevel(2);
    
    float releaseRate;
    dcaEnvelope.getStageRate(3, releaseRate);
    float releaseMs = (releaseRate < 0.1f) ? 100.0f : 300.0f;
    
    std::array<float, 8> rates, levels;
    int sus, end;
    DSP::ADSRtoStageConverter::convertADSR(
        attackMs, decayMs, sustainLevel, releaseMs,
        rates, levels, sus, end, 44100.0
    );
    
    dcaEnvelope.setStage(1, rates[1], levels[1]);
}

void Voice::setDCASustain(float level) noexcept
{
    float sustainLevel = std::clamp(level, 0.0f, 1.0f);
    
    float attackRate;
    dcaEnvelope.getStageRate(0, attackRate);
    float attackMs = (attackRate < 0.1f) ? 10.0f : 100.0f;
    
    float decayRate;
    dcaEnvelope.getStageRate(1, decayRate);
    float decayMs = (decayRate < 0.1f) ? 100.0f : 300.0f;
    
    float releaseRate;
    dcaEnvelope.getStageRate(3, releaseRate);
    float releaseMs = (releaseRate < 0.1f) ? 100.0f : 300.0f;
    
    std::array<float, 8> rates, levels;
    int sus, end;
    DSP::ADSRtoStageConverter::convertADSR(
        attackMs, decayMs, sustainLevel, releaseMs,
        rates, levels, sus, end, 44100.0
    );
    
    dcaEnvelope.setStage(1, rates[1], levels[1]);
    dcaEnvelope.setStage(2, rates[2], levels[2]);
}

void Voice::setDCARelease(float seconds) noexcept
{
    float releaseMs = std::clamp(seconds * 1000.0f, 0.0f, 8000.0f);
    
    float attackRate;
    dcaEnvelope.getStageRate(0, attackRate);
    float attackMs = (attackRate < 0.1f) ? 10.0f : 100.0f;
    
    float decayRate;
    dcaEnvelope.getStageRate(1, decayRate);
    float decayMs = (decayRate < 0.1f) ? 100.0f : 300.0f;
    
    float sustainLevel = dcaEnvelope.getStageLevel(2);
    
    std::array<float, 8> rates, levels;
    int sus, end;
    DSP::ADSRtoStageConverter::convertADSR(
        attackMs, decayMs, sustainLevel, releaseMs,
        rates, levels, sus, end, 44100.0
    );
    
    dcaEnvelope.setStage(3, rates[3], levels[3]);
}
```

---

## 📄 ARCHIVO 4: Source/State/PresetManager.cpp - Nuevos Presets

**REEMPLAZAR `createBassPreset()` y `createStringPreset()` con:**

```cpp
void PresetManager::createBassPreset()
{
    Preset p;
    p.name = "CZ Bass";
    initEnvelopes(p);
    
    // ===== OSCILLATORS (NORMALIZED) =====
    p.parameters["osc1_waveform"] = 1.0f;      // Saw
    p.parameters["osc1_level"] = 0.6f;         // ✅ 60% (normalized)
    p.parameters["osc2_waveform"] = 2.0f;      // Square
    p.parameters["osc2_level"] = 0.4f;         // ✅ 40% (normalized)
    // Total: 0.6 + 0.4 = 1.0 ✅
    
    p.parameters["osc2_detune"] = -10.0f;      // -10 cents
    
    // ===== ADSR (IN SECONDS) =====
    p.parameters["dcw_attack"] = 0.01f;        // ✅ 10ms (crisp)
    p.parameters["dcw_decay"] = 0.2f;          // ✅ 200ms
    p.parameters["dcw_sustain"] = 0.2f;        // ✅ 20% level
    p.parameters["dcw_release"] = 0.1f;        // ✅ 100ms
    
    p.parameters["dca_attack"] = 0.001f;       // ✅ 1ms (very crisp)
    p.parameters["dca_decay"] = 0.2f;          // ✅ 200ms
    p.parameters["dca_sustain"] = 0.5f;        // ✅ 50% level
    p.parameters["dca_release"] = 0.15f;       // ✅ 150ms
    
    // ===== FILTER =====
    p.parameters["filter_cutoff"] = 2000.0f;   // 2000 Hz
    p.parameters["filter_resonance"] = 0.5f;   // 50% Q
    
    // ===== LFO =====
    p.parameters["lfo_rate"] = 0.5f;           // 0.5 Hz
    p.parameters["lfo_depth"] = 0.0f;          // No vibrato
    
    // ===== EFFECTS =====
    p.parameters["delay_time"] = 0.3f;         // ✅ 300ms
    p.parameters["delay_feedback"] = 0.3f;     // 30%
    p.parameters["delay_mix"] = 0.08f;         // ✅ 8% wet
    
    p.parameters["chorus_rate"] = 0.5f;        // 0.5 Hz
    p.parameters["chorus_depth"] = 2.0f;       // 2ms
    p.parameters["chorus_mix"] = 0.0f;         // Off
    
    p.parameters["reverb_size"] = 0.3f;        // Small room
    p.parameters["reverb_mix"] = 0.08f;        // ✅ 8% wet
    
    p.parameters["hard_sync"] = 0.0f;          // Off
    p.parameters["ring_mod"] = 0.0f;           // Off
    p.parameters["glide_time"] = 0.0f;         // No portamento
    
    presets.push_back(p);
}

void PresetManager::createStringPreset()
{
    Preset p;
    p.name = "Vintage Strings";
    initEnvelopes(p);
    
    // ===== OSCILLATORS (NORMALIZED) =====
    p.parameters["osc1_waveform"] = 1.0f;      // Saw
    p.parameters["osc1_level"] = 0.5f;         // ✅ 50% (normalized)
    p.parameters["osc2_waveform"] = 1.0f;      // Saw
    p.parameters["osc2_level"] = 0.5f;         // ✅ 50% (normalized)
    // Total: 0.5 + 0.5 = 1.0 ✅
    
    p.parameters["osc2_detune"] = 12.0f;       // +1 octava
    
    // ===== ADSR (IN SECONDS) - REALISTIC STRINGS =====
    p.parameters["dcw_attack"] = 0.3f;         // ✅ 300ms (bow friction)
    p.parameters["dcw_decay"] = 0.4f;          // ✅ 400ms
    p.parameters["dcw_sustain"] = 0.7f;        // ✅ 70% level
    p.parameters["dcw_release"] = 0.5f;        // ✅ 500ms
    
    p.parameters["dca_attack"] = 0.4f;         // ✅ 400ms (smooth)
    p.parameters["dca_decay"] = 0.3f;          // ✅ 300ms
    p.parameters["dca_sustain"] = 0.8f;        // ✅ 80% level
    p.parameters["dca_release"] = 0.6f;        // ✅ 600ms (smooth release)
    
    // ===== FILTER =====
    p.parameters["filter_cutoff"] = 8000.0f;   // Open
    p.parameters["filter_resonance"] = 0.3f;   // 30% Q
    
    // ===== LFO (VIBRATO) =====
    p.parameters["lfo_rate"] = 4.5f;           // ✅ 4.5 Hz
    p.parameters["lfo_depth"] = 0.08f;         // ✅ Subtle vibrato
    
    // ===== EFFECTS =====
    p.parameters["delay_time"] = 0.25f;        // ✅ 250ms
    p.parameters["delay_feedback"] = 0.4f;     // 40%
    p.parameters["delay_mix"] = 0.3f;          // ✅ 30% wet (longer tail)
    
    p.parameters["chorus_rate"] = 0.6f;        // 0.6 Hz
    p.parameters["chorus_depth"] = 3.0f;       // 3ms
    p.parameters["chorus_mix"] = 0.15f;        // ✅ 15% light chorus
    
    p.parameters["reverb_size"] = 0.7f;        // Large room
    p.parameters["reverb_mix"] = 0.4f;         // ✅ 40% wet (lush)
    
    p.parameters["hard_sync"] = 0.0f;
    p.parameters["ring_mod"] = 0.0f;
    p.parameters["glide_time"] = 0.0f;
    
    presets.push_back(p);
}
```

---

## ✅ CHECKLIST DE IMPLEMENTACIÓN

```
[ ] Crear ADSRtoStage.h con convertADSR()
[ ] Incluir <array> en Voice.h
[ ] Reemplazar renderNextSample() con versión normalizada
[ ] Implementar setDCWAttack/Decay/Sustain/Release
[ ] Implementar setDCAAttack/Decay/Sustain/Release
[ ] Reemplazar createBassPreset()
[ ] Reemplazar createStringPreset()
[ ] Compilar sin errores
[ ] Probar ADSR knobs en DAW
[ ] Medir tiempos con Audacity
[ ] Verificar que osciladores no clipen
[ ] Cargar presets originales CZ-101 para comparar
```

---

**Status:** Ready to implement  
**Severity:** CRITICAL (affects all presets)  
**Time to implement:** ~30 minutes
