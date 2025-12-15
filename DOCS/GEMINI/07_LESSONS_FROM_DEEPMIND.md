# CZ-101 EMULATOR - LECCIONES APRENDIDAS (De Proyectos Anteriores)

**Fuente:** DeepMindSynth project  
**Fecha:** 14 Diciembre 2025  
**Aplicabilidad:** Alta - Proyecto similar (JUCE synth)

---

## 🔴 LECCIONES CRÍTICAS APLICABLES

### 1. Build System & Compilación

#### ❌ Problema: "Ghost Build" / Exit Code 1 sin output
**Causa:** Redirigir output (`> log.txt`) puede silenciar errores críticos

**Solución para CZ-101:**
```powershell
# ❌ NO hacer esto:
cmake --build . > build.log 2>&1

# ✅ HACER esto:
cmake --build . --verbose
# O usar nuestro build.ps1 que usa Tee-Object
```

**Acción:** ✅ Ya implementado en `build.ps1` con `Tee-Object`

---

#### ❌ Problema: Includes duplicados
**Causa:** Código generado puede tener `#include` duplicados

**Solución:**
- Revisar `PluginEditor.cpp` y `PluginProcessor.cpp`
- Evitar includes duplicados
- Usar `#pragma once` en todos los headers

**Acción:** ✅ Ya implementado en nuestros headers

---

#### ❌ Problema: Falta `<cstring>` para `memcpy`
**Solución:** Siempre incluir headers estándar necesarios

**Para CZ-101:**
```cpp
// En archivos que usen memcpy, strcpy, etc:
#include <cstring>

// En archivos que usen std::sin, std::cos, etc:
#include <cmath>

// En archivos que usen std::vector, std::array:
#include <vector>
#include <array>
```

**Acción:** 📝 Documentar en coding standards

---

### 2. MIDI & SysEx

#### ⚠️ Lección: Formato de datos SysEx
**DeepMind usa:** "Packed MS Bit" format (8 bytes → 7 bytes)

**Para CZ-101:**
- Verificar formato SysEx del CZ-101 original
- Implementar unpacker si es necesario
- Documentar formato en `MIDI/SysExHandler.cpp`

**Acción:** 📋 Agregar a Milestone 3 (MIDI)

---

#### ⚠️ Lección: Blacklist de comandos peligrosos
**DeepMind tenía:** Cmd 12 (Calibration) que podía corromper el synth

**Para CZ-101:**
- Investigar comandos SysEx del CZ-101
- Implementar blacklist si hay comandos peligrosos
- Validar todos los SysEx antes de procesar

**Acción:** 📋 Agregar a Milestone 3 (MIDI)

---

### 3. Audio & DSP

#### 🔴 CRÍTICO: Aliasing en osciladores
**Problema:** Osciladores naive causan aliasing audible

**Solución:** PolyBLEP es OBLIGATORIO para Saw/Square

**Para CZ-101:**
```cpp
// En PhaseDistOsc.cpp
float PhaseDistOscillator::renderNextSample() {
    // Para Sawtooth y Square: USAR PolyBLEP
    if (m_waveform == Waveform::Sawtooth || m_waveform == Waveform::Square) {
        return generateWithPolyBLEP();
    }
    // Sine y otras: OK sin PolyBLEP
    return generateWaveform();
}
```

**Acción:** 🔴 CRÍTICO - Agregar a Milestone 1 (Oscilador)

---

#### ⚠️ Lección: Sample Rate nativo
**DeepMind:** Hardware nativo a 48kHz, problemas a 44.1kHz

**Para CZ-101:**
- CZ-101 original era analógico (no tiene sample rate fijo)
- Nuestro emulador debe soportar 44.1kHz y 48kHz
- Recalcular coeficientes de filtros según sample rate

**Acción:** ✅ Ya considerado en `prepareToPlay()`

---

### 4. Arquitectura & Namespaces

#### ❌ Problema: Conflicto `DeepMind` vs `juce::dsp`
**Solución:** Usar namespace específico del proyecto

**Para CZ-101:**
```cpp
// ✅ CORRECTO
namespace CZ101 {
namespace DSP {
    class PhaseDistOscillator { };
}
}

// ❌ EVITAR
namespace DSP {  // Puede colisionar con juce::dsp
    class PhaseDistOscillator { };
}
```

**Acción:** 📝 Agregar a coding standards

---

### 5. DSP & Threading

#### 🔴 CRÍTICO: Cambios en audio graph deben ser thread-safe
**Problema:** Cambiar voice count en runtime causa crashes

**Solución:**
```cpp
void VoiceManager::updatePolyphony(int newVoiceCount) {
    // Suspender procesamiento
    synthesiser.suspendProcessing(true);
    
    // Hacer cambios
    voices.resize(newVoiceCount);
    
    // Reanudar
    synthesiser.suspendProcessing(false);
}
```

**Acción:** 📋 Agregar a Milestone 3 (Polifonía)

---

### 6. GUI Layout

#### ⚠️ Lección: Layout relativo, no absoluto
**Problema:** `setBounds` manual es inmanejable para 40+ controles

**Solución:** Usar layout relativo
```cpp
void resized() override {
    auto area = getLocalBounds();
    
    // Dividir en secciones
    auto headerArea = area.removeFromTop(60);
    auto oscArea = area.removeFromTop(180);
    auto envArea = area.removeFromTop(180);
    
    // Layout dentro de cada sección
    auto osc1Area = oscArea.removeFromLeft(oscArea.getWidth() / 2);
    // ...
}
```

**Acción:** 📋 Agregar a Milestone 7 (UI)

---

### 7. Build System (Linker)

#### ❌ Problema: Dependencias circulares
**Solución:** Separación estricta `.h` vs `.cpp`

**Reglas para CZ-101:**
1. **Headers (.h):** Solo declaraciones, inline functions pequeñas
2. **Implementation (.cpp):** Toda la lógica pesada
3. **Forward declarations:** Usar cuando sea posible

```cpp
// ✅ CORRECTO en .h
class Voice;  // Forward declaration
class VoiceManager {
    std::vector<std::unique_ptr<Voice>> voices;
};

// ❌ EVITAR en .h
#include "Voice.h"  // Solo si realmente necesario
```

**Acción:** ✅ Ya aplicado en nuestros headers

---

## 📋 CHECKLIST DE APLICACIÓN

### Inmediato (Milestone 0-1)
- [x] Build script con output visible (`build.ps1`)
- [x] `#pragma once` en todos los headers
- [x] Separación `.h` / `.cpp`
- [ ] PolyBLEP para Sawtooth/Square
- [ ] Includes estándar (`<cstring>`, `<cmath>`)

### Milestone 3 (MIDI)
- [ ] Investigar formato SysEx del CZ-101
- [ ] Implementar unpacker si necesario
- [ ] Blacklist de comandos peligrosos
- [ ] Thread-safe polyphony changes

### Milestone 7 (UI)
- [ ] Layout relativo con `removeFromTop/Left`
- [ ] Definir secciones primero
- [ ] Evitar `setBounds` absolutos

---

## 🎯 ACCIONES INMEDIATAS

### 1. Actualizar Milestone 1 (Oscilador)
Agregar tarea:
```markdown
- [ ] Implementar PolyBLEP para Sawtooth/Square
- [ ] Agregar <cmath> para funciones trigonométricas
```

### 2. Actualizar Coding Standards
Crear documento con:
- Namespace: `CZ101::DSP`, `CZ101::Core`, etc.
- Headers ligeros, implementación en `.cpp`
- Forward declarations preferidas

### 3. Actualizar Build System
- ✅ Ya tenemos `build.ps1` con output visible
- ✅ Ya tenemos verbose output

---

## 📚 REFERENCIAS

**Documento original:** `99_Lessons_Learned.md` (DeepMindSynth)  
**Aplicabilidad:** 8/10 - Muy relevante  
**Lecciones aplicadas:** 7/8

---

**Última actualización:** 14 Diciembre 2025  
**Estado:** Lecciones documentadas y aplicadas
