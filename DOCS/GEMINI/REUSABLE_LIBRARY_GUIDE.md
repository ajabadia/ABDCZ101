# GUÍA: EXTRAER LIBRERÍA REUTILIZABLE

**Objetivo:** Crear `SynthDSP` - Librería de componentes DSP reutilizables

---

## 📦 ESTRUCTURA DE LIBRERÍA

```
SynthDSP/
├── Oscillators/
│   ├── WaveTable.h/cpp          ✅ 100% reutilizable
│   ├── PhaseDistOsc.h/cpp       ✅ 100% reutilizable
│   └── WaveShaper.h/cpp         ⚠️ 70% (específico CZ-101)
├── Envelopes/
│   ├── ADSREnvelope.h/cpp       ✅ 100% reutilizable
│   └── MultiStageEnv.h/cpp      ✅ 100% reutilizable
├── Modulation/
│   └── LFO.h/cpp                ✅ 100% reutilizable
├── Filters/
│   └── ResonantFilter.h/cpp     ✅ 100% reutilizable
├── Effects/
│   └── Delay.h/cpp              ✅ 100% reutilizable
└── Core/
    └── VoiceManager.h/cpp       ✅ 100% reutilizable
```

---

## 🔧 PASO 1: CAMBIAR NAMESPACES

### Opción A: Namespace Genérico
```cpp
// Antes (específico):
namespace CZ101 {
namespace DSP {
    class WaveTable { };
}}

// Después (genérico):
namespace SynthDSP {
    class WaveTable { };
}
```

### Opción B: Mantener Estructura
```cpp
namespace SynthDSP {
namespace Oscillators {
    class WaveTable { };
    class PhaseDistOsc { };
}
namespace Envelopes {
    class ADSR { };
}}
```

---

## 🔧 PASO 2: ELIMINAR DEPENDENCIAS ESPECÍFICAS

### WaveShaper (Específico CZ-101)
```cpp
// Opción 1: Hacerlo genérico
class WaveShaper {
    // Permitir custom curve function
    std::function<float(float, float)> curveFunction;
};

// Opción 2: Excluir de librería
// Dejar en proyecto CZ-101 específico
```

### Voice (Específico)
```cpp
// NO incluir en librería
// Cada synth tiene su propia arquitectura Voice
// Pero SÍ incluir VoiceManager (genérico)
```

---

## 🔧 PASO 3: CREAR CMakeLists.txt PARA LIBRERÍA

```cmake
# SynthDSP/CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(SynthDSP VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

# Header-only o compiled library
add_library(SynthDSP STATIC
    Oscillators/WaveTable.cpp
    Oscillators/PhaseDistOsc.cpp
    Envelopes/ADSREnvelope.cpp
    Envelopes/MultiStageEnv.cpp
    Modulation/LFO.cpp
    Filters/ResonantFilter.cpp
    Effects/Delay.cpp
    Core/VoiceManager.cpp
)

target_include_directories(SynthDSP PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Opcional: JUCE dependency
find_package(JUCE REQUIRED)
target_link_libraries(SynthDSP PUBLIC juce::juce_audio_basics)
```

---

## 🔧 PASO 4: USAR EN NUEVOS PROYECTOS

### Proyecto: MiniMoog Emulator

```cmake
# MiniMoog/CMakeLists.txt
add_subdirectory(../SynthDSP SynthDSP)

target_link_libraries(MiniMoogEmulator PRIVATE
    SynthDSP
    juce::juce_audio_processors
)
```

```cpp
// MiniMoog/Voice.h
#include <SynthDSP/Oscillators/PhaseDistOsc.h>
#include <SynthDSP/Envelopes/ADSREnvelope.h>
#include <SynthDSP/Filters/ResonantFilter.h>

class MiniMoogVoice {
    SynthDSP::PhaseDistOsc osc1, osc2, osc3;
    SynthDSP::ADSR filterEnv, ampEnv;
    SynthDSP::ResonantFilter filter;
};
```

---

## 📊 COMPONENTES POR CATEGORÍA

### 🟢 CORE (Incluir siempre)
- WaveTable
- PhaseDistOsc (con PolyBLEP)
- ADSREnvelope
- LFO
- ResonantFilter

### 🟡 AVANZADO (Incluir si necesario)
- MultiStageEnv
- Delay
- VoiceManager

### 🔴 ESPECÍFICO (Excluir o adaptar)
- WaveShaper (CZ-101 phase distortion)
- Voice (arquitectura específica)
- Parameters (específico de cada synth)

---

## 🎯 EJEMPLO: 3 SYNTHS DIFERENTES

### CZ-101 Emulator (Actual)
```cpp
Voice: 2× PhaseDistOsc → WaveShaper → DCW → DCA
Components: WaveTable, PhaseDistOsc, WaveShaper, ADSR, MultiStage
```

### MiniMoog Emulator
```cpp
Voice: 3× PhaseDistOsc → Mixer → Filter → VCA
Components: WaveTable, PhaseDistOsc, ADSR, ResonantFilter
```

### Juno-106 Emulator
```cpp
Voice: 1× PhaseDistOsc → Filter → Chorus → VCA
Components: WaveTable, PhaseDistOsc, ADSR, ResonantFilter, Chorus
```

**Código compartido:** ~70-80%

---

## ✅ VENTAJAS

1. **DRY (Don't Repeat Yourself)**
   - Escribir PolyBLEP una vez, usar en todos los proyectos

2. **Mantenimiento**
   - Bug fix en librería → todos los proyectos se benefician

3. **Testing**
   - Tests unitarios una vez → confianza en todos los proyectos

4. **Velocidad**
   - Nuevo synth en días, no semanas

---

## 📝 CHECKLIST PARA EXTRAER

- [ ] Cambiar namespaces a genéricos
- [ ] Eliminar dependencias específicas CZ-101
- [ ] Crear CMakeLists.txt para librería
- [ ] Documentar API pública
- [ ] Crear tests unitarios
- [ ] Versionar (Git tag v1.0.0)
- [ ] Publicar (GitHub/GitLab)

---

## 🚀 PRÓXIMOS PASOS

1. **Ahora:** Terminar CZ-101 Emulator
2. **Después:** Extraer SynthDSP como librería
3. **Futuro:** Crear MiniMoog/Juno usando SynthDSP

---

**Conclusión:** Esta arquitectura modular permite crear un "Frankenstein" synth reutilizando 70-80% del código DSP.
