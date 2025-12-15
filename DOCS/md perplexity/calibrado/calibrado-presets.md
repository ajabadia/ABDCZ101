# 📋 REPORTE DETALLADO - CALIBRADO DE PRESETS CZ-101
## Problemas Identificados + Soluciones Completas

**Generado:** Dec 15, 2025 @ 19:06 CET  
**Versión del Plugin:** v0.9-rc3  
**Severidad:** CRÍTICA (afecta sonoridad de todos los presets)

---

## 📊 TABLA DE CONTENIDOS

1. [Problema 1: ADSR Desconectado](#problema-1-adsr-desconectado)
2. [Problema 2: Osciladores Overshooting](#problema-2-osciladores-overshooting)
3. [Problema 3: Rangos Indefinidos](#problema-3-rangos-indefinidos)
4. [Solución Integrada + Código](#solución-integrada--código)
5. [Plan de Testing](#plan-de-testing)

---

## PROBLEMA 1: ADSR DESCONECTADO

### Descripción

Los knobs ADSR en la UI están **completamente desconectados** del motor de síntesis. El código tiene dos problemas simultáneos:

**A) Métodos vacíos en Voice.cpp:**
```cpp
// Voice.cpp líneas 52-58 ❌ NO HACEN NADA
void Voice::setDCAAttack(float s) noexcept { }
void Voice::setDCADecay(float s) noexcept { }
void Voice::setDCASustain(float l) noexcept { }
void Voice::setDCARelease(float s) noexcept { }

void Voice::setDCWAttack(float s) noexcept { juce::ignoreUnused(s); }
void Voice::setDCWDecay(float s) noexcept { juce::ignoreUnused(s); }
void Voice::setDCWSustain(float l) noexcept { juce::ignoreUnused(l); }
void Voice::setDCWRelease(float s) noexcept { juce::ignoreUnused(s); }
```

**B) Sistema 8-stages con valores hardcodeados:**
```cpp
// PresetManager.cpp - valores fijos, sin conexión a ADSR
p.dcaEnv.rates[0] = 0.99f;    // Attack: coeficiente
p.dcaEnv.rates[1] = 0.6f;     // Decay: coeficiente
p.dcaEnv.levels[1] = 0.6f;    // Sustain: nivel fijo
p.dcaEnv.rates[3] = 0.6f;     // Release: coeficiente
```

### Impacto Sonoro

- **UI Knobs ADSR no funcionan** → Usuario mueve knobs sin efecto
- **Presets suenen siempre igual** → No se pueden editar en tiempo real
- **Tiempos de envelope imposibles de controlar** → Todos suenan largos

### Causa Raíz

El código fue implementado en **dos vías paralelas**:
1. **Ruta ADSR:** Parámetros creados, UI implementada, pero código vacío
2. **Ruta 8-stages:** MultiStageEnvelope funcional, pero hardcodeada

**Nunca se conectaron ambas.**

---

## PROBLEMA 2: OSCILADORES OVERSHOOTING

### Descripción

Los niveles de osciladores se suman sin normalización:

```cpp
// Voice::renderNextSample() línea 85
float mix = (osc1Sample * osc1Level) + (osc2Sample * osc2Level);
// ↑ Si osc1Level + osc2Level > 1.0 → CLIPPING
```

**Presets actuales:**
```
Bass:      osc1_level=1.0 + osc2_level=0.8 = 1.8 ❌ 80% OVERSHOOTING
Strings:   osc1_level=0.7 + osc2_level=0.7 = 1.4 ❌ 40% OVERSHOOTING
Brass:     osc1_level=0.75+ osc2_level=0.75= 1.5 ❌ 50% OVERSHOOTING
```

### Impacto Sonoro

- **Clipping no intencional** → Distorsión digital áspera
- **Osciladores con dinámica pobre** → Solapan frecuencias
- **Presets más ruidosos** → Necesitan más release para parecer "largos"

### Causa Raíz

Mezcla lineal sin guardrail. En CZ-101 real, cada oscillator tiene rango 0-100% pero el hardware normalizaba automáticamente.

---

## PROBLEMA 3: RANGOS INDEFINIDOS

### Descripción

Los parámetros se almacenan sin claridad sobre sus rangos reales:

```cpp
// PresetManager.cpp - ¿Unidades? ¿Rangos?
p.parameters["filter_cutoff"] = 2000.0f;        // Hz? 0-1.0? Midi?
p.parameters["delay_time"] = 0.4f;              // Segundos? 0-1.0?
p.parameters["reverb_size"] = 0.3f;             // 0-1.0? Pasos?
p.parameters["lfo_rate"] = 0.5f;                // Hz? 0-1.0?
p.parameters["dcw_decay"] = 0.3f;               // ms? Segundos? Coef?
```

### Tabla de Ambigüedades Críticas

| Parámetro | Valor Actual | Interpretación A | Interpretación B | Interpretación C |
|-----------|-------------|------------------|------------------|------------------|
| `filter_cutoff` | 2000.0 | Hz literal | Normalizado 0-20k | MIDI step |
| `delay_time` | 0.4 | 0.4 segundos | 0.4 de 2 segs | 400ms |
| `dcw_decay` | 0.3 | 300ms | 3000ms (×10) | Coef 0-1 |
| `lfo_rate` | 0.5 | 0.5 Hz | 5 Hz (×10) | Normalizado |

### Impacto

- **Presets suenan "casuales"** no científicos
- **Imposible comparar con CZ-101** original
- **Futuras ediciones de presets son aleatorias**

---

## SOLUCIÓN INTEGRADA + CÓDIGO

### PASO 1: Crear Función de Conversión ADSR → 8-Stages

Archivo: `Source/DSP/Envelopes/ADSRtoStage.h` **(NUEVO)**

```cpp
#pragma once

#include <cmath>
#include <array>

namespace CZ101 {
namespace DSP {

/**
 * @brief Convierte parámetros ADSR (milisegundos) a 8-stage envelope
 * 
 * Mapea:
 *   - Attack (0-8000ms)
 *   - Decay (0-8000ms)
 *   - Sustain (0-100%)
 *   - Release (0-8000ms)
 * 
 * A: stage rates + levels usando aproximación logarítmica
 */
struct ADSRtoStageConverter {
    
    /**
     * @brief Conversión ADSR → 8 stages
     * 
     * Stages:
     *   0: Attack ramp (rate 0-1, level 0→1)
     *   1: Decay ramp (rate 0-1, level 1→sustain)
     *   2: Sustain hold (rate 0.99, level sustain)
     *   3: Release ramp (rate 0-1, level sustain→0)
     *   4-7: Unused (hold at 0)
     */
    static void convertADSR(
        float attackMs,      // 0-8000
        float decayMs,       // 0-8000
        float sustainLevel,  // 0-1.0
        float releaseMs,     // 0-8000
        std::array<float, 8>& outRates,
        std::array<float, 8>& outLevels,
        int& outSustainPoint,
        int& outEndPoint,
        double sampleRate = 44100.0
    ) {
        // Función interna: ms → rate coefficient
        auto msToRateCoeff = [sampleRate](float ms) -> float {
            if (ms < 1.0f) ms = 1.0f;
            
            // Tiempo en segundos
            float sec = ms / 1000.0f;
            
            // Descenso logarítmico: e^(-k*t) con k basado en SR
            // Para 60dB en tiempo T: -60dB = 20*log10(e^(-k*T))
            // k = 5.5 / T (para 60dB atenuación)
            float k = 5.5f / (sec * static_cast<float>(sampleRate));
            
            // Convertir a coeficiente de decaimiento per-sample
            // rate = e^(-k) ≈ 1 - k para k pequeños
            float coeff = std::exp(-k);
            
            // Clamp 0.0-0.99 (0.99 = casi no decae)
            return std::clamp(coeff, 0.0f, 0.99f);
        };
        
        // ===== STAGE 0: ATTACK =====
        outRates[0] = msToRateCoeff(attackMs);
        outLevels[0] = 1.0f;  // Rampa hacia 1.0
        
        // ===== STAGE 1: DECAY =====
        outRates[1] = msToRateCoeff(decayMs);
        outLevels[1] = sustainLevel;  // Rampa hacia sustain
        
        // ===== STAGE 2: SUSTAIN HOLD =====
        outRates[2] = 0.99f;  // Mantiene (sin decaimiento)
        outLevels[2] = sustainLevel;
        
        // ===== STAGE 3: RELEASE =====
        outRates[3] = msToRateCoeff(releaseMs);
        outLevels[3] = 0.0f;  // Rampa hacia 0
        
        // Stages 4-7: Unused
        for (int i = 4; i < 8; ++i) {
            outRates[i] = 0.99f;
            outLevels[i] = 0.0f;
        }
        
        // Sustain point: stage 2 (hold)
        outSustainPoint = 2;
        
        // End point: stage 3 (release)
        outEndPoint = 3;
    }
};

} // namespace DSP
} // namespace CZ101
```

---

### PASO 2: Implementar Métodos ADSR en Voice

Archivo: `Source/Core/Voice.cpp` **(ACTUALIZAR)**

Reemplazar líneas 52-67 con:

```cpp
// ===== DCW ENVELOPE (8-Stage) =====

void Voice::setDCWAttack(float seconds) noexcept
{
    float ms = seconds * 1000.0f;
    float decayMs, sustainLevel, releaseMs;
    
    // Capturar valores actuales
    voiceManager->getDCWStage(1, decayMs, sustainLevel);
    decayMs *= 1000.0f; // Aproximado (ver nota abajo)
    voiceManager->getDCWStage(3, releaseMs, sustainLevel);
    releaseMs *= 1000.0f;
    
    // Reconvertir ADSR a stages
    std::array<float, 8> rates, levels;
    int sus, end;
    DSP::ADSRtoStageConverter::convertADSR(
        ms, decayMs, sustainLevel, releaseMs,
        rates, levels, sus, end, sampleRate
    );
    
    // Aplicar solo stage 0 (attack)
    dcwEnvelope.setStage(0, rates[0], levels[0]);
}

void Voice::setDCWDecay(float seconds) noexcept
{
    float ms = seconds * 1000.0f;
    float attackMs, sustainLevel, releaseMs;
    
    // Capturar valores actuales (aproximado)
    voiceManager->getDCWStage(0, attackMs, sustainLevel);
    voiceManager->getDCWStage(3, releaseMs, sustainLevel);
    
    std::array<float, 8> rates, levels;
    int sus, end;
    DSP::ADSRtoStageConverter::convertADSR(
        attackMs, ms, sustainLevel, releaseMs,
        rates, levels, sus, end, sampleRate
    );
    
    dcwEnvelope.setStage(1, rates[1], levels[1]);
}

void Voice::setDCWSustain(float level) noexcept
{
    float attackMs, decayMs, releaseMs;
    
    // Capturar otros parámetros
    voiceManager->getDCWStage(0, attackMs, sustainLevel);
    voiceManager->getDCWStage(1, decayMs, sustainLevel);
    voiceManager->getDCWStage(3, releaseMs, sustainLevel);
    
    std::array<float, 8> rates, levels;
    int sus, end;
    DSP::ADSRtoStageConverter::convertADSR(
        attackMs, decayMs, level, releaseMs,
        rates, levels, sus, end, sampleRate
    );
    
    // Aplicar sustain a stages 1 y 2
    dcwEnvelope.setStage(1, rates[1], levels[1]);
    dcwEnvelope.setStage(2, rates[2], levels[2]);
}

void Voice::setDCWRelease(float seconds) noexcept
{
    float ms = seconds * 1000.0f;
    float attackMs, decayMs, sustainLevel;
    
    voiceManager->getDCWStage(0, attackMs, sustainLevel);
    voiceManager->getDCWStage(1, decayMs, sustainLevel);
    
    std::array<float, 8> rates, levels;
    int sus, end;
    DSP::ADSRtoStageConverter::convertADSR(
        attackMs, decayMs, sustainLevel, ms,
        rates, levels, sus, end, sampleRate
    );
    
    dcwEnvelope.setStage(3, rates[3], levels[3]);
}

// ===== DCA ENVELOPE (8-Stage) - IDÉNTICO A DCW =====

void Voice::setDCAAttack(float seconds) noexcept
{
    float ms = seconds * 1000.0f;
    float decayMs, sustainLevel, releaseMs;
    
    voiceManager->getDCAStage(1, decayMs, sustainLevel);
    voiceManager->getDCAStage(3, releaseMs, sustainLevel);
    
    std::array<float, 8> rates, levels;
    int sus, end;
    DSP::ADSRtoStageConverter::convertADSR(
        ms, decayMs, sustainLevel, releaseMs,
        rates, levels, sus, end, sampleRate
    );
    
    dcaEnvelope.setStage(0, rates[0], levels[0]);
}

void Voice::setDCADecay(float seconds) noexcept
{
    float ms = seconds * 1000.0f;
    float attackMs, sustainLevel, releaseMs;
    
    voiceManager->getDCAStage(0, attackMs, sustainLevel);
    voiceManager->getDCAStage(3, releaseMs, sustainLevel);
    
    std::array<float, 8> rates, levels;
    int sus, end;
    DSP::ADSRtoStageConverter::convertADSR(
        attackMs, ms, sustainLevel, releaseMs,
        rates, levels, sus, end, sampleRate
    );
    
    dcaEnvelope.setStage(1, rates[1], levels[1]);
}

void Voice::setDCASustain(float level) noexcept
{
    float attackMs, decayMs, releaseMs;
    
    voiceManager->getDCAStage(0, attackMs, sustainLevel);
    voiceManager->getDCAStage(1, decayMs, sustainLevel);
    voiceManager->getDCAStage(3, releaseMs, sustainLevel);
    
    std::array<float, 8> rates, levels;
    int sus, end;
    DSP::ADSRtoStageConverter::convertADSR(
        attackMs, decayMs, level, releaseMs,
        rates, levels, sus, end, sampleRate
    );
    
    dcaEnvelope.setStage(1, rates[1], levels[1]);
    dcaEnvelope.setStage(2, rates[2], levels[2]);
}

void Voice::setDCARelease(float seconds) noexcept
{
    float ms = seconds * 1000.0f;
    float attackMs, decayMs, sustainLevel;
    
    voiceManager->getDCAStage(0, attackMs, sustainLevel);
    voiceManager->getDCAStage(1, decayMs, sustainLevel);
    
    std::array<float, 8> rates, levels;
    int sus, end;
    DSP::ADSRtoStageConverter::convertADSR(
        attackMs, decayMs, sustainLevel, ms,
        rates, levels, sus, end, sampleRate
    );
    
    dcaEnvelope.setStage(3, rates[3], levels[3]);
}
```

---

### PASO 3: Normalizar Osciladores

Archivo: `Source/Core/Voice.cpp` **(ACTUALIZAR renderNextSample)**

```cpp
float Voice::renderNextSample() noexcept
{
    if (!dcaEnvelope.isActive()) return 0.0f;
    
    // ... (código de modulación de pitch igual) ...
    
    float osc1Sample = osc1.renderNextSample(dcwValue, &osc1Wrapped);
    
    if (isHardSyncEnabled && osc1Wrapped) {
        osc2.reset();
    }
    
    float osc2Sample = osc2.renderNextSample(dcwValue);
    
    if (isRingModEnabled) {
        osc2Sample = osc1Sample * osc2Sample;
    }
    
    // ✅ NORMALIZACIÓN DE NIVELES
    // Calcular suma total para evitar overshooting
    float totalLevel = osc1Level + osc2Level;
    float normalizer = (totalLevel > 1.0f) ? 1.0f / totalLevel : 1.0f;
    
    float mix = (osc1Sample * osc1Level + osc2Sample * osc2Level) * normalizer;
    
    // ✅ APLICAR ENVELOPES CON HEADROOM
    return mix * dcaValue * currentVelocity * 0.9f;  // 0.9f = headroom
}
```

---

### PASO 4: Documentar Rangos de Parámetros

Crear: `Source/State/ParameterRanges.h` **(NUEVO)**

```cpp
#pragma once

/**
 * @brief Documentación oficial de rangos de parámetros
 * 
 * Cada parámetro tiene:
 * - Rango interno (usado en código)
 * - Mapeo a usuario (como se ve en UI)
 * - Unidades
 * - Valores típicos
 */

namespace CZ101 {
namespace State {

struct ParameterRange {
    const char* name;
    float minValue;
    float maxValue;
    const char* units;
    const char* description;
};

// ========== OSCILADORES ==========
const ParameterRange OSC1_LEVEL = {
    "OSC1 Level", 0.0f, 1.0f, "Linear",
    "Nivel de oscilador 1. Rango 0-100%. Normalización automática si OSC1+OSC2>1.0"
};

const ParameterRange OSC2_LEVEL = {
    "OSC2 Level", 0.0f, 1.0f, "Linear",
    "Nivel de oscilador 2. Rango 0-100%. Normalización automática si OSC1+OSC2>1.0"
};

const ParameterRange OSC2_DETUNE = {
    "OSC2 Detune", -100.0f, 100.0f, "cents",
    "Desafinación de OSC2. ±100 cents = ±1 semitono"
};

// ========== ENVELOPES ==========
const ParameterRange DCW_ATTACK = {
    "DCW Attack", 0.0f, 8.0f, "seconds",
    "Tiempo de ataque del envolvente DCW (timbre). 0-8000ms"
};

const ParameterRange DCW_DECAY = {
    "DCW Decay", 0.0f, 8.0f, "seconds",
    "Tiempo de caída del envolvente DCW. 0-8000ms"
};

const ParameterRange DCW_SUSTAIN = {
    "DCW Sustain", 0.0f, 1.0f, "Linear",
    "Nivel de sustain del envolvente DCW. 0-100%"
};

const ParameterRange DCW_RELEASE = {
    "DCW Release", 0.0f, 8.0f, "seconds",
    "Tiempo de liberación del envolvente DCW. 0-8000ms"
};

const ParameterRange DCA_ATTACK = {
    "DCA Attack", 0.0f, 8.0f, "seconds",
    "Tiempo de ataque del envolvente DCA (amplitud). 0-8000ms"
};

const ParameterRange DCA_DECAY = {
    "DCA Decay", 0.0f, 8.0f, "seconds",
    "Tiempo de caída del envolvente DCA. 0-8000ms"
};

const ParameterRange DCA_SUSTAIN = {
    "DCA Sustain", 0.0f, 1.0f, "Linear",
    "Nivel de sustain del envolvente DCA. 0-100%"
};

const ParameterRange DCA_RELEASE = {
    "DCA Release", 0.0f, 8.0f, "seconds",
    "Tiempo de liberación del envolvente DCA. 0-8000ms"
};

// ========== FILTRO ==========
const ParameterRange FILTER_CUTOFF = {
    "Filter Cutoff", 20.0f, 20000.0f, "Hz",
    "Frecuencia de corte. 20-20000 Hz. Escala logarítmica"
};

const ParameterRange FILTER_RESONANCE = {
    "Filter Resonance", 0.0f, 1.0f, "Linear",
    "Resonancia/Q del filtro. 0-100%"
};

// ========== EFECTOS ==========
const ParameterRange DELAY_TIME = {
    "Delay Time", 0.001f, 2.0f, "seconds",
    "Tiempo de delay. 1ms-2000ms"
};

const ParameterRange DELAY_FEEDBACK = {
    "Delay Feedback", 0.0f, 0.95f, "Linear",
    "Realimentación de delay. 0-95% (limitado para estabilidad)"
};

const ParameterRange DELAY_MIX = {
    "Delay Mix", 0.0f, 1.0f, "Linear",
    "Balance Dry/Wet. 0% = seco, 100% = solo delay"
};

const ParameterRange CHORUS_RATE = {
    "Chorus Rate", 0.1f, 10.0f, "Hz",
    "Frecuencia del LFO del chorus. 0.1-10 Hz"
};

const ParameterRange CHORUS_DEPTH = {
    "Chorus Depth", 0.0f, 20.0f, "ms",
    "Profundidad de modulación. 0-20ms"
};

const ParameterRange CHORUS_MIX = {
    "Chorus Mix", 0.0f, 1.0f, "Linear",
    "Balance Dry/Wet del chorus. 0-100%"
};

const ParameterRange REVERB_SIZE = {
    "Reverb Size", 0.0f, 1.0f, "Linear",
    "Tamaño de la sala. 0% = pequeña, 100% = catedral"
};

const ParameterRange REVERB_MIX = {
    "Reverb Mix", 0.0f, 1.0f, "Linear",
    "Balance Dry/Wet del reverb. 0-100%"
};

// ========== MODULACIÓN ==========
const ParameterRange LFO_RATE = {
    "LFO Rate", 0.1f, 20.0f, "Hz",
    "Frecuencia del LFO (vibrato). 0.1-20 Hz"
};

const ParameterRange LFO_DEPTH = {
    "LFO Depth", 0.0f, 2.0f, "semitones",
    "Profundidad del vibrato. 0-2 semitonos"
};

} // namespace State
} // namespace CZ101
```

---

### PASO 5: Ajustar Presets con Valores Calibrados

Archivo: `Source/State/PresetManager.cpp` **(ACTUALIZAR createBassPreset, etc.)**

```cpp
void PresetManager::createBassPreset()
{
    Preset p;
    p.name = "CZ Bass";
    initEnvelopes(p);
    
    // Oscillators: Saw + Square (NORMALIZADO)
    p.parameters["osc1_waveform"] = 1.0f;      // Saw
    p.parameters["osc1_level"] = 0.6f;         // 60% (antes 1.0 ❌)
    p.parameters["osc2_waveform"] = 2.0f;      // Square
    p.parameters["osc2_level"] = 0.4f;         // 40% (antes 0.8 ❌)
    // Total: 0.6 + 0.4 = 1.0 ✅ NORMALIZADO
    
    p.parameters["osc2_detune"] = -10.0f;
    
    // ADSR - Plucky Bass (EN SEGUNDOS)
    p.parameters["dcw_attack"] = 0.01f;        // 10ms
    p.parameters["dcw_decay"] = 0.2f;          // 200ms (antes 0.3 ❌)
    p.parameters["dcw_sustain"] = 0.2f;        // 20% nivel
    p.parameters["dcw_release"] = 0.1f;        // 100ms (antes 0.1 ✅)
    
    p.parameters["dca_attack"] = 0.001f;       // 1ms (crisp)
    p.parameters["dca_decay"] = 0.2f;          // 200ms (antes 0.3 ❌)
    p.parameters["dca_sustain"] = 0.5f;        // 50% nivel (antes 0.6 ❌)
    p.parameters["dca_release"] = 0.15f;       // 150ms (antes 0.2 ❌)
    
    // Filter
    p.parameters["filter_cutoff"] = 2000.0f;   // Hz (20-20000 rango)
    p.parameters["filter_resonance"] = 0.5f;   // 0-1.0
    
    // LFO
    p.parameters["lfo_rate"] = 0.5f;           // Hz
    p.parameters["lfo_depth"] = 0.0f;          // Sin vibrato
    
    // Effects (reducir un poco para que no suene "pastoso")
    p.parameters["delay_time"] = 0.3f;         // 300ms (antes 0.4 ❌)
    p.parameters["delay_feedback"] = 0.3f;     // 30%
    p.parameters["delay_mix"] = 0.08f;         // 8% (antes 0.1 ❌)
    
    p.parameters["chorus_rate"] = 0.5f;        // Hz
    p.parameters["chorus_depth"] = 2.0f;       // ms
    p.parameters["chorus_mix"] = 0.0f;         // Off
    
    p.parameters["reverb_size"] = 0.3f;
    p.parameters["reverb_mix"] = 0.08f;        // Mínimo reverb (antes 0.1 ❌)
    
    p.parameters["hard_sync"] = 0.0f;
    p.parameters["ring_mod"] = 0.0f;
    p.parameters["glide_time"] = 0.0f;
    
    presets.push_back(p);
}

void PresetManager::createStringPreset()
{
    Preset p;
    p.name = "Vintage Strings";
    initEnvelopes(p);
    
    // Oscillators (NORMALIZADO)
    p.parameters["osc1_waveform"] = 1.0f;      // Saw
    p.parameters["osc1_level"] = 0.5f;         // 50% (antes 0.7 ❌)
    p.parameters["osc2_waveform"] = 1.0f;      // Saw
    p.parameters["osc2_level"] = 0.5f;         // 50% (antes 0.7 ❌)
    // Total: 0.5 + 0.5 = 1.0 ✅ NORMALIZADO
    
    p.parameters["osc2_detune"] = 12.0f;       // 1 octava arriba
    
    // ADSR - Strings (tiempos REALISTAS, no "largos")
    // Strings reales: Attack lento (fricción de arco), Sustain largo
    p.parameters["dcw_attack"] = 0.3f;         // 300ms (antes 0.5 ❌)
    p.parameters["dcw_decay"] = 0.4f;          // 400ms (antes 0.5 ❌)
    p.parameters["dcw_sustain"] = 0.7f;        // 70% nivel
    p.parameters["dcw_release"] = 0.5f;        // 500ms (antes 0.8 ❌)
    
    p.parameters["dca_attack"] = 0.4f;         // 400ms (antes 0.6 ❌)
    p.parameters["dca_decay"] = 0.3f;          // 300ms (antes 0.5 ❌)
    p.parameters["dca_sustain"] = 0.8f;        // 80% nivel (antes 0.9 ❌)
    p.parameters["dca_release"] = 0.6f;        // 600ms (antes 0.8 ❌)
    
    // Filter
    p.parameters["filter_cutoff"] = 8000.0f;   // Hz
    p.parameters["filter_resonance"] = 0.3f;
    
    // LFO (Vibrato)
    p.parameters["lfo_rate"] = 4.5f;           // Hz (antes 5.0 ❌)
    p.parameters["lfo_depth"] = 0.08f;         // Vibrato sutil (antes 0.1 ❌)
    
    // Effects - Strings necesitan reverb lush
    p.parameters["delay_time"] = 0.25f;        // 250ms (antes 0.3 ❌)
    p.parameters["delay_feedback"] = 0.4f;     // 40%
    p.parameters["delay_mix"] = 0.3f;          // 30% (antes 0.4 ❌)
    
    p.parameters["chorus_rate"] = 0.6f;
    p.parameters["chorus_depth"] = 3.0f;
    p.parameters["chorus_mix"] = 0.15f;        // Light chorus
    
    p.parameters["reverb_size"] = 0.7f;
    p.parameters["reverb_mix"] = 0.4f;         // 40% reverb (antes 0.5 ❌)
    
    p.parameters["hard_sync"] = 0.0f;
    p.parameters["ring_mod"] = 0.0f;
    p.parameters["glide_time"] = 0.0f;
    
    presets.push_back(p);
}
```

---

## PLAN DE TESTING

### Fase 1: Compilación + Verificación Básica (15 min)

```bash
# 1. Incluir nuevo header
grep -n "ADSRtoStage" Source/Core/Voice.h
# → Debe estar incluido

# 2. Compilar
cmake --build . --config Release 2>&1 | grep -i error
# → Debe haber 0 errores

# 3. Cargar en DAW
# → UI debe renderizar sin crashes
```

### Fase 2: Testing de Envelopes (30 min)

**Herramientas necesarias:**
- Plugin cargado en Reaper/DAW
- Audacity para medir tiempos
- MIDI Controller (o teclado virtual)

**Test A: Verificar que ADSR knobs funcionan**

```
1. Cargar "CZ Bass"
2. Nota C3, sujetada 2 segundos
3. Mover DCW Attack knob:
   - Esperado: envelope ataque visible cambia
   - Si no cambia: ADSR no está conectado aún

4. Mover DCA Decay knob:
   - Esperado: nota se apaga más rápido/lento
   - Si no cambia: problema en implementación
```

**Test B: Medir tiempos reales**

```
1. Grabar Bass con DCA Decay = 0.2s (200ms)
2. En Audacity, medir desde nota-on a sustain
   - Esperado: ~200ms
   - Si >500ms: problema de conversión

3. Grabar con DCA Release = 0.15s (150ms)
4. Soltar nota, medir hasta silencio
   - Esperado: ~150ms
   - Si >400ms: problema de release
```

**Test C: Osciladores Normalized**

```
1. Cargar Bass (osc1_level=0.6, osc2_level=0.4)
2. Tocar nota fuerte sin reverb
   - Esperado: sonido limpio, sin clipping
   - Si suena "duro"/distorsionado: problema de normalización

3. Cambiar a osc1=1.0, osc2=0.0 (solo osc1)
4. Comparar nivel
   - Esperado: mismo nivel aproximadamente
   - Si osc1+osc2 suena más fuerte: normalización fallida
```

### Fase 3: Comparativa CZ-101 Original (20 min)

Si tienes acceso a CZ-101 original o emulador auténtico:

```
1. Grabar mismo preset en ambos
2. Superponer en Audacity
3. Comparar:
   - Tiempos de envelope (should be within ±50ms)
   - Timbres (DCW curve)
   - Niveles de osciladores
```

---

## CAMBIOS RESUMIDOS

| Archivo | Cambio | Línea(s) |
|---------|--------|---------|
| `Source/DSP/Envelopes/ADSRtoStage.h` | CREAR (nuevo) | - |
| `Source/Core/Voice.cpp` | Implementar ADSR | 52-120 |
| `Source/Core/Voice.cpp` | Normalizar OSC mix | 85-95 |
| `Source/State/ParameterRanges.h` | CREAR (documentación) | - |
| `Source/State/PresetManager.cpp` | Ajustar Bass/Strings | 150-250 |

---

## VERIFICACIÓN FINAL

Después de implementar, verificar:

✅ Compilación sin errores  
✅ Plugin carga en DAW  
✅ ADSR knobs afectan el sonido  
✅ Tiempos matches (±50ms vs CZ-101)  
✅ Osciladores no clip  
✅ Presets suenan "naturales" (no demasiado largos)  

---

**Generado:** Dec 15, 2025  
**Plugin:** CZ-101 Emulator v0.9-rc3  
**Estado:** READY FOR IMPLEMENTATION
