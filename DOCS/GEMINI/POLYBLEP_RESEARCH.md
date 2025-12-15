# POLYBLEP: ¿POR QUÉ ES NECESARIO?

**Fecha:** 14 Diciembre 2025  
**Investigación:** CZ-101 y aliasing digital

---

## 🔍 CONCLUSIÓN

**PolyBLEP ES NECESARIO** para el emulador CZ-101, pero **NO** porque el CZ-101 original lo usara.

---

## 📊 HALLAZGOS

### 1. El CZ-101 Original (1984)

**Síntesis:** Phase Distortion (PD) digital  
**Problema:** El CZ-101 **SÍ tenía aliasing** inherente

**Citas clave:**
> "Aliasing is an inherent characteristic and a known issue within phase distortion synthesis"

> "While Casio's implementation incorporated techniques like synchronization and windowing to mitigate aliasing, some remnants can still be present"

**Técnicas de Casio:**
- Envelope per cycle (sinusoid starts/ends at zero)
- Windowing
- Synchronization
- Esto **reducía** aliasing, pero no lo eliminaba

### 2. Por Qué Necesitamos PolyBLEP

**Razón:** Estamos creando osciladores digitales **desde cero**

**El problema:**
- Waveforms con discontinuidades (Sawtooth, Square, Pulse)
- Generan armónicos infinitos
- Frecuencias > Nyquist frequency (22.05 kHz @ 44.1 kHz)
- **Foldover distortion** = aliasing audible

**Sin PolyBLEP:**
```
Sawtooth @ 1000 Hz:
- Armónico 1: 1000 Hz ✅
- Armónico 10: 10000 Hz ✅
- Armónico 30: 30000 Hz ❌ > Nyquist
  → Refleja a: 44100 - 30000 = 14100 Hz (inarmónico)
  → Resultado: Distorsión "buzzy" horrible
```

**Con PolyBLEP:**
```
Sawtooth @ 1000 Hz:
- Discontinuidades suavizadas
- Armónicos altos reducidos
- Sin foldover
- Sonido limpio y profesional
```

---

## 🎯 DECISIÓN FINAL

### Para el Emulador CZ-101

**SÍ usar PolyBLEP** porque:

1. **Calidad superior al original**
   - El CZ-101 original tenía aliasing
   - Podemos hacerlo mejor en 2025

2. **Estándar moderno**
   - Todos los sintetizadores digitales modernos usan anti-aliasing
   - PolyBLEP es el estándar de la industria

3. **Usuarios esperan calidad**
   - Nadie quiere aliasing audible
   - "Authentic" no significa "con defectos"

4. **Computacionalmente eficiente**
   - PolyBLEP es rápido
   - Buen balance calidad/CPU

### Dónde Aplicar

**Waveforms que NECESITAN PolyBLEP:**
- ✅ Sawtooth (discontinuidad en reset)
- ✅ Square (2 discontinuidades por ciclo)
- ✅ Pulse (2 discontinuidades por ciclo)

**Waveforms que NO necesitan PolyBLEP:**
- ❌ Sine (continua, sin discontinuidades)
- ❌ Triangle (continua, pero ver nota*)

**Nota Triangle:* Técnicamente continua, pero tiene discontinuidad en la derivada. PolyBLEP opcional pero recomendado.

---

## 💡 ANALOGÍA

**Pregunta:** "¿Debemos emular el aliasing del CZ-101 original?"

**Respuesta:** NO.

**Analogía:**
- El CZ-101 original tenía aliasing porque era 1984
- Era una limitación técnica, no una característica deseada
- Es como emular un Minimoog:
  - ✅ Emulamos el sonido cálido analógico
  - ❌ NO emulamos el ruido de fondo de 60Hz
  - ❌ NO emulamos los pots ruidosos

**Objetivo:** Capturar la **esencia** del CZ-101, no sus **defectos técnicos**

---

## 📝 IMPLEMENTACIÓN

### Código PolyBLEP

```cpp
namespace CZ101 {
namespace DSP {

class PhaseDistOscillator {
private:
    // PolyBLEP: Polynomial Bandlimited Step
    float polyBLEP(float t, float dt) const noexcept
    {
        // t: fase normalizada [0, 1]
        // dt: incremento de fase por sample
        
        // Discontinuidad al inicio del ciclo
        if (t < dt) {
            t /= dt;
            return t + t - t * t - 1.0f;
        }
        // Discontinuidad al final del ciclo
        else if (t > 1.0f - dt) {
            t = (t - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        }
        
        return 0.0f;
    }
    
    // Aplicar a Sawtooth
    float renderSawtooth() noexcept
    {
        float value = 2.0f * phase - 1.0f;  // Naive sawtooth
        value -= polyBLEP(phase, phaseIncrement);  // Anti-aliasing
        return value;
    }
    
    // Aplicar a Square
    float renderSquare() noexcept
    {
        float value = (phase < 0.5f) ? 1.0f : -1.0f;  // Naive square
        
        // Discontinuidad en rising edge (0.0)
        value += polyBLEP(phase, phaseIncrement);
        
        // Discontinuidad en falling edge (0.5)
        value -= polyBLEP(fmod(phase + 0.5f, 1.0f), phaseIncrement);
        
        return value;
    }
};

} // namespace DSP
} // namespace CZ101
```

---

## 🔬 VERIFICACIÓN

### Test de Aliasing

```cpp
TEST(PhaseDistOscTest, NoAliasing) {
    PhaseDistOscillator osc;
    osc.setSampleRate(44100.0);
    osc.setFrequency(5000.0f);  // Frecuencia alta
    osc.setWaveform(WAVEFORM_SAWTOOTH);
    
    // Generar 1 segundo
    std::vector<float> buffer(44100);
    for (int i = 0; i < 44100; ++i) {
        buffer[i] = osc.renderNextSample();
    }
    
    // FFT para analizar espectro
    auto spectrum = performFFT(buffer);
    
    // Verificar que no hay componentes > Nyquist
    for (int i = 22050; i < spectrum.size(); ++i) {
        EXPECT_LT(spectrum[i], 0.001f);  // Casi cero
    }
}
```

---

## 📚 REFERENCIAS

1. **Wikipedia - Phase Distortion Synthesis**
   - "Aliasing is an inherent characteristic"
   - Casio usó windowing para mitigar

2. **Native Instruments Forum**
   - "Casio's design reduces aliasing compared to basic methods"
   - Pero no lo elimina completamente

3. **PolyBLEP Research**
   - Estándar de la industria
   - Balance óptimo calidad/CPU

---

## ✅ DECISIÓN FINAL

**USAR PolyBLEP** en:
- Sawtooth
- Square
- Pulse
- (Opcional) Triangle

**RAZÓN:** Calidad profesional moderna, no emular defectos de 1984

---

**Conclusión:** PolyBLEP es necesario para crear un emulador de **calidad profesional**, no porque el CZ-101 original lo usara, sino porque podemos hacerlo **mejor** que en 1984.
