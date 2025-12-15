# CZ-101 EMULATOR - CODING STANDARDS

**Versión:** 1.0  
**Fecha:** 14 Diciembre 2025  
**Basado en:** Lecciones de DeepMindSynth + JUCE best practices

---

## 📋 REGLAS OBLIGATORIAS

### 1. Namespaces

**✅ USAR:**
```cpp
namespace CZ101 {
namespace DSP {
    class PhaseDistOscillator { };
}
namespace Core {
    class Voice { };
}
namespace MIDI {
    class MIDIProcessor { };
}
}
```

**❌ EVITAR:**
```cpp
namespace DSP {  // Colisiona con juce::dsp
    class PhaseDistOscillator { };
}
```

**Razón:** Evitar conflictos con `juce::dsp` y otros namespaces de JUCE

---

### 2. Headers (.h)

**Reglas:**
1. Siempre usar `#pragma once`
2. Headers deben ser LIGEROS
3. Solo declaraciones, no implementaciones pesadas
4. Usar forward declarations cuando sea posible

**✅ CORRECTO:**
```cpp
#pragma once

#include <juce_core/juce_core.h>  // JUCE primero
#include <vector>                  // STL después

namespace CZ101 {
namespace DSP {

// Forward declaration
class WaveTable;

class PhaseDistOscillator {
public:
    PhaseDistOscillator();
    ~PhaseDistOscillator() = default;
    
    void setFrequency(float freq) noexcept;
    float renderNextSample() noexcept;
    
private:
    float m_phase = 0.0f;
    float m_frequency = 440.0f;
    
    // Helper privado (inline OK si es pequeño)
    inline float incrementPhase() noexcept {
        m_phase += m_phaseIncrement;
        if (m_phase >= 1.0f) m_phase -= 1.0f;
        return m_phase;
    }
};

} // namespace DSP
} // namespace CZ101
```

**❌ EVITAR:**
```cpp
// Sin #pragma once
#ifndef PHASE_DIST_OSC_H  // Usar #pragma once en su lugar
#define PHASE_DIST_OSC_H

// Implementación pesada en header
class PhaseDistOscillator {
    float renderNextSample() {
        // 50 líneas de código aquí  // ❌ Mover a .cpp
    }
};

#endif
```

---

### 3. Implementation (.cpp)

**Reglas:**
1. Toda la lógica pesada va aquí
2. Incluir headers estándar necesarios
3. Usar `noexcept` donde corresponda

**✅ CORRECTO:**
```cpp
#include "PhaseDistOsc.h"

#include <cmath>    // Para std::sin, std::cos
#include <array>    // Para std::array
#include <cstring>  // Para memcpy (si se usa)

namespace CZ101 {
namespace DSP {

PhaseDistOscillator::PhaseDistOscillator()
    : m_phase(0.0f)
    , m_frequency(440.0f)
{
}

void PhaseDistOscillator::setFrequency(float freq) noexcept
{
    m_frequency = juce::jlimit(20.0f, 20000.0f, freq);
    m_phaseIncrement = m_frequency / m_sampleRate;
}

float PhaseDistOscillator::renderNextSample() noexcept
{
    // Implementación completa aquí
    float sample = std::sin(m_phase * juce::MathConstants<float>::twoPi);
    
    incrementPhase();
    
    return sample;
}

} // namespace DSP
} // namespace CZ101
```

---

### 4. Includes Estándar Necesarios

**Tabla de referencia:**

| Header | Cuándo usar |
|--------|-------------|
| `<cmath>` | std::sin, std::cos, std::tan, std::exp, std::log, std::pow |
| `<cstring>` | memcpy, memset, strcpy |
| `<array>` | std::array |
| `<vector>` | std::vector |
| `<memory>` | std::unique_ptr, std::shared_ptr |
| `<algorithm>` | std::min, std::max, std::clamp |
| `<cstdint>` | int32_t, uint8_t, etc. |

**Ejemplo completo:**
```cpp
#include "MyClass.h"

#include <juce_core/juce_core.h>
#include <cmath>      // std::sin
#include <array>      // std::array
#include <memory>     // std::unique_ptr
#include <algorithm>  // std::clamp
```

---

### 5. Anti-Aliasing (PolyBLEP)

**OBLIGATORIO para:** Sawtooth, Square, Pulse

**Implementación:**
```cpp
namespace CZ101 {
namespace DSP {

class PhaseDistOscillator {
private:
    // PolyBLEP para anti-aliasing
    float polyBLEP(float t, float dt) const noexcept
    {
        // t: fase normalizada [0, 1]
        // dt: incremento de fase por sample
        if (t < dt) {
            t /= dt;
            return t + t - t * t - 1.0f;
        } else if (t > 1.0f - dt) {
            t = (t - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        }
        return 0.0f;
    }
    
public:
    float renderSawtooth() noexcept
    {
        float sample = 2.0f * m_phase - 1.0f;
        
        // Aplicar PolyBLEP en discontinuidades
        sample -= polyBLEP(m_phase, m_phaseIncrement);
        
        incrementPhase();
        return sample;
    }
    
    float renderSquare() noexcept
    {
        float sample = (m_phase < 0.5f) ? 1.0f : -1.0f;
        
        // Aplicar PolyBLEP en ambas discontinuidades
        sample += polyBLEP(m_phase, m_phaseIncrement);
        sample -= polyBLEP(fmod(m_phase + 0.5f, 1.0f), m_phaseIncrement);
        
        incrementPhase();
        return sample;
    }
};

} // namespace DSP
} // namespace CZ101
```

---

### 6. Thread Safety

**Reglas:**
1. NUNCA modificar parámetros en audio thread
2. Usar `AudioProcessorValueTreeState` para parámetros
3. Usar `suspendProcessing()` para cambios estructurales

**✅ CORRECTO:**
```cpp
// En PluginProcessor.h
class CZ101AudioProcessor : public juce::AudioProcessor {
private:
    juce::AudioProcessorValueTreeState parameters;
};

// En processBlock()
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;  // SIEMPRE
    
    // Leer parámetros (thread-safe)
    float freq = parameters.getRawParameterValue("frequency")->load();
    
    // NUNCA hacer esto:
    // myParameter = newValue;  // ❌ NO THREAD-SAFE
}

// Para cambios estructurales
void updatePolyphony(int newVoiceCount)
{
    suspendProcessing(true);
    
    // Hacer cambios
    voices.resize(newVoiceCount);
    
    suspendProcessing(false);
}
```

---

### 7. Naming Conventions

**Clases:** PascalCase
```cpp
class PhaseDistOscillator { };
```

**Funciones:** camelCase
```cpp
void setFrequency(float freq);
float renderNextSample();
```

**Variables:** camelCase
```cpp
float sampleRate = 44100.0f;
int voiceCount = 8;
```

**Miembros privados:** camelCase con prefijo `m_`
```cpp
class MyClass {
private:
    float m_phase;
    int m_voiceCount;
};
```

**Constantes:** UPPER_SNAKE_CASE
```cpp
static constexpr int MAX_VOICES = 8;
static constexpr float TWO_PI = 6.28318530718f;
```

---

### 8. Tamaño de Archivos

**Límites:**
- **Máximo:** 300 líneas por archivo
- **Ideal:** 150-200 líneas
- **Si excede:** Dividir en archivos más pequeños

**Razón:** Archivos grandes son propensos a erromperse al editar

---

### 9. Comentarios

**Usar Doxygen style:**
```cpp
/**
 * Oscilador de síntesis Phase Distortion
 * 
 * Implementa el algoritmo de síntesis del Casio CZ-101
 * con anti-aliasing mediante PolyBLEP.
 * 
 * @see WaveTable para las tablas de ondas
 */
class PhaseDistOscillator {
public:
    /**
     * Configura la frecuencia del oscilador
     * 
     * @param freq Frecuencia en Hz (20-20000)
     */
    void setFrequency(float freq) noexcept;
};
```

---

### 10. Build System

**Reglas:**
1. Usar `build.ps1` para compilar (output visible)
2. NUNCA redirigir output a archivo (`> log.txt`)
3. Siempre compilar con `--verbose` si hay problemas

**✅ CORRECTO:**
```powershell
# Usar script
.\build.ps1

# O manual con verbose
cmake --build . --verbose
```

**❌ EVITAR:**
```powershell
# Silencia errores críticos
cmake --build . > build.log 2>&1
```

---

## 📋 CHECKLIST PRE-COMMIT

Antes de cada commit, verificar:

- [ ] Todos los headers tienen `#pragma once`
- [ ] Namespaces correctos (`CZ101::DSP`, etc.)
- [ ] Includes estándar necesarios (`<cmath>`, etc.)
- [ ] PolyBLEP en Sawtooth/Square
- [ ] `ScopedNoDenormals` en processBlock
- [ ] No hay modificaciones de parámetros en audio thread
- [ ] Archivos <300 líneas
- [ ] Comentarios Doxygen en clases públicas
- [ ] Forward declarations donde sea posible
- [ ] Código compila sin warnings

---

## 🔗 REFERENCIAS

- **Lecciones aprendidas:** `07_LESSONS_FROM_DEEPMIND.md`
- **Arquitectura:** `01_ARCHITECTURE.md`
- **JUCE Coding Standards:** https://juce.com/learn/documentation

---

**Última actualización:** 14 Diciembre 2025  
**Versión:** 1.0
