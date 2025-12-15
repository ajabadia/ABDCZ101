# CZ-101 EMULATOR - MASTER IMPLEMENTATION PLAN

**Fecha de inicio:** 14 Diciembre 2025  
**Versión objetivo:** 1.0.0  
**Tiempo estimado:** 8-10 semanas (desarrollo modular incremental)  
**Arquitectura:** C++17 + JUCE 7.x

---

## 🎯 OBJETIVOS DEL PROYECTO

### Objetivo Principal
Crear un emulador profesional del sintetizador Casio CZ-101 (1984) que:
- Replique fielmente la síntesis Phase Distortion original
- Funcione como plugin VST3/AU y aplicación standalone
- Sea multiplataforma (Windows, macOS, Linux)
- Tenga código modular, mantenible y bien documentado

### Objetivos Secundarios
- Aprender arquitectura profesional de audio plugins
- Aplicar mejores prácticas de DSP y desarrollo C++
- Crear una base reutilizable para futuros proyectos

---

## 📋 PRINCIPIOS DE DESARROLLO

### 1. Modularidad Extrema
- **Máximo 300 líneas por archivo** (ideal: 150-200)
- **Un concepto = un archivo**
- **Separación clara:** DSP / UI / MIDI / State Management
- **Interfaces bien definidas** entre módulos

### 2. Desarrollo Incremental
- **Milestone-driven:** Cada hito es funcional y testeable
- **Vertical slices:** Implementar features completas end-to-end
- **Test-first:** Tests unitarios antes de código complejo

### 3. Calidad del Código
- **C++ moderno:** C++17, smart pointers, RAII
- **Const-correctness:** `const` y `noexcept` donde corresponda
- **Zero allocations** en audio thread
- **Documentación inline:** Doxygen-style comments

### 4. Performance
- **Lock-free structures** para comunicación threads
- **SIMD optimizations** donde sea crítico
- **Profiling regular:** Valgrind, Instruments, Visual Studio Profiler
- **Target:** <5% CPU @ 8 voces + efectos (i5 2.5GHz)

---

## 🏗️ ARQUITECTURA DEL PROYECTO

### Estructura de Directorios

```
ABDZ101/
├── CMakeLists.txt                 # Build configuration
├── README.md                      # Documentación usuario
├── DOCS/
│   ├── GEMINI/                    # Documentación de desarrollo
│   │   ├── 00_MASTER_PLAN.md     # Este archivo
│   │   ├── 01_ARCHITECTURE.md    # Arquitectura detallada
│   │   ├── 02_MILESTONES.md      # Hitos y progreso
│   │   ├── 03_DSP_SPECS.md       # Especificaciones DSP
│   │   ├── 04_UI_DESIGN.md       # Diseño de interfaz
│   │   └── 05_TESTING.md         # Estrategia de testing
│   └── [documentación existente]
├── Source/
│   ├── Core/                      # Núcleo del sintetizador
│   │   ├── Voice.h/cpp           # Voz individual
│   │   ├── VoiceManager.h/cpp    # Gestión de voces
│   │   └── SynthEngine.h/cpp     # Motor principal
│   ├── DSP/                       # Procesamiento de señal
│   │   ├── Oscillators/
│   │   │   ├── PhaseDistOsc.h/cpp
│   │   │   ├── WaveTable.h/cpp
│   │   │   └── WaveShaper.h/cpp
│   │   ├── Envelopes/
│   │   │   ├── ADSREnvelope.h/cpp
│   │   │   └── MultiStageEnv.h/cpp
│   │   ├── Filters/
│   │   │   └── DCWFilter.h/cpp
│   │   ├── Effects/
│   │   │   ├── Reverb.h/cpp
│   │   │   ├── Chorus.h/cpp
│   │   │   └── Delay.h/cpp
│   │   └── Modulation/
│   │       ├── LFO.h/cpp
│   │       └── ModMatrix.h/cpp
│   ├── MIDI/
│   │   ├── MIDIProcessor.h/cpp
│   │   ├── SysExHandler.h/cpp
│   │   └── CCMapper.h/cpp
│   ├── State/
│   │   ├── Preset.h/cpp
│   │   ├── PresetManager.h/cpp
│   │   └── Parameters.h/cpp
│   ├── UI/
│   │   ├── PluginEditor.h/cpp
│   │   ├── Components/
│   │   │   ├── LCDDisplay.h/cpp
│   │   │   ├── Knob.h/cpp
│   │   │   ├── Button.h/cpp
│   │   │   └── Wheel.h/cpp
│   │   └── LookAndFeel/
│   │       └── CZ101LookAndFeel.h/cpp
│   ├── Utils/
│   │   ├── Constants.h
│   │   ├── MathUtils.h/cpp
│   │   └── Logger.h/cpp
│   └── PluginProcessor.h/cpp      # JUCE processor principal
├── Tests/
│   ├── DSP/
│   ├── MIDI/
│   └── Integration/
└── Resources/
    ├── Presets/
    ├── Fonts/
    └── Images/
```

### Separación de Responsabilidades

| Módulo | Responsabilidad | Dependencias |
|--------|----------------|--------------|
| **Core** | Lógica de síntesis, gestión de voces | DSP, State |
| **DSP** | Algoritmos de procesamiento de señal | Utils |
| **MIDI** | Entrada/salida MIDI, SysEx | Core, State |
| **State** | Gestión de parámetros y presets | Utils |
| **UI** | Interfaz gráfica | State, JUCE |
| **Utils** | Utilidades compartidas | Ninguna |

---

## 🎯 FASES DE DESARROLLO

### FASE 0: Infraestructura (Semana 1)
**Objetivo:** Proyecto compilable con arquitectura base

**Entregables:**
- ✅ CMake configurado (VST3, AU, Standalone)
- ✅ Estructura de directorios creada
- ✅ Plugin vacío que compila y carga en DAW
- ✅ Sistema de logging básico
- ✅ Framework de testing (GoogleTest)

**Criterio de éxito:** Plugin carga en DAW y genera silencio

---

### FASE 1: Core DSP - Oscilador (Semana 2)
**Objetivo:** Oscilador Phase Distortion funcional

**Entregables:**
- ✅ `WaveTable.h/cpp` - Tablas de ondas (Sine, Saw, Square, Triangle)
- ✅ `PhaseDistOsc.h/cpp` - Oscilador con phase distortion
- ✅ `WaveShaper.h/cpp` - Distorsión de fase
- ✅ Tests unitarios para cada waveform
- ✅ Validación de frecuencia (440Hz = 440Hz)

**Criterio de éxito:** Oscilador genera sine wave a 440Hz verificable con analizador

---

### FASE 2: Envelopes & Voice (Semana 2-3)
**Objetivo:** Voz completa con envelopes

**Entregables:**
- ✅ `ADSREnvelope.h/cpp` - Envelope básico
- ✅ `MultiStageEnv.h/cpp` - Envelope de 8 etapas
- ✅ `Voice.h/cpp` - Voz individual (DCO + DCW + DCA)
- ✅ Tests de timing de envelopes
- ✅ Validación de curvas exponenciales

**Criterio de éxito:** Nota MIDI genera sonido con attack/release correcto

---

### FASE 3: Polifonía & MIDI (Semana 3-4)
**Objetivo:** 8 voces polifónicas con MIDI completo

**Entregables:**
- ✅ `VoiceManager.h/cpp` - Gestión de 8 voces
- ✅ Voice stealing inteligente
- ✅ `MIDIProcessor.h/cpp` - Note on/off, velocity, pitch bend
- ✅ `CCMapper.h/cpp` - MIDI CC mappings
- ✅ Tests de polifonía

**Criterio de éxito:** Tocar acorde de 8 notas sin glitches

---

### FASE 4: Modulación (Semana 4)
**Objetivo:** LFOs y matriz de modulación

**Entregables:**
- ✅ `LFO.h/cpp` - 2 LFOs (Sine, Triangle, Saw, Square, Random)
- ✅ `ModMatrix.h/cpp` - Ruteo de modulación
- ✅ Aftertouch polifónico
- ✅ Tests de modulación

**Criterio de éxito:** LFO modula pitch con vibrato audible

---

### FASE 5: Efectos (Semana 5)
**Objetivo:** Reverb, Chorus, Delay

**Entregables:**
- ✅ `Reverb.h/cpp` - Reverb por convolución simple
- ✅ `Chorus.h/cpp` - Chorus con LFO
- ✅ `Delay.h/cpp` - Delay analógico
- ✅ FX chain configurable
- ✅ Tests de efectos

**Criterio de éxito:** Efectos audibles sin artefactos

---

### FASE 6: State Management (Semana 5-6)
**Objetivo:** Presets y parámetros

**Entregables:**
- ✅ `Parameters.h/cpp` - Sistema de parámetros JUCE
- ✅ `Preset.h/cpp` - Estructura de preset
- ✅ `PresetManager.h/cpp` - Carga/guardado JSON
- ✅ 16 presets iniciales
- ✅ Tests de serialización

**Criterio de éxito:** Guardar/cargar preset sin pérdida de datos

---

### FASE 7: UI Básica (Semana 6-7)
**Objetivo:** Interfaz funcional (1 tema)

**Entregables:**
- ✅ `PluginEditor.h/cpp` - Editor principal
- ✅ `LCDDisplay.h/cpp` - Pantalla LCD 16×2
- ✅ `Knob.h/cpp` - Knob rotatorio
- ✅ `CZ101LookAndFeel.h/cpp` - Tema Dark Mode
- ✅ Layout básico funcional

**Criterio de éxito:** UI responde a cambios de parámetros en tiempo real

---

### FASE 8: Características Avanzadas (Semana 7-8)
**Objetivo:** Features bonus

**Entregables:**
- ✅ Sustain pedal inteligente
- ✅ Portamento/Glide
- ✅ Arpeggiador básico
- ✅ Unison mode
- ✅ Tests de integración

**Criterio de éxito:** Todas las features funcionan sin bugs

---

### FASE 9: Optimización & Testing (Semana 8)
**Objetivo:** Performance y estabilidad

**Entregables:**
- ✅ Profiling y optimización
- ✅ Eliminación de allocations en audio thread
- ✅ Tests de stress (1000 notas/segundo)
- ✅ Validación de CPU usage (<5%)
- ✅ Memory leak detection

**Criterio de éxito:** Plugin estable 24h sin crashes

---

### FASE 10: Distribución (Semana 8+)
**Objetivo:** Empaquetado y release

**Entregables:**
- ✅ Instaladores (Windows, macOS, Linux)
- ✅ Documentación de usuario
- ✅ Manual PDF
- ✅ Video tutorial
- ✅ GitHub release v1.0.0

**Criterio de éxito:** Plugin instalable y funcional en 3 plataformas

---

## 📊 SISTEMA DE TRACKING

### Milestones
Cada fase tiene un archivo de milestone en `DOCS/GEMINI/MILESTONES/`:
- `MILESTONE_00_Infrastructure.md`
- `MILESTONE_01_Oscillator.md`
- `MILESTONE_02_Envelopes.md`
- etc.

### Formato de Milestone
```markdown
# MILESTONE X: [Nombre]

**Estado:** 🔴 No iniciado / 🟡 En progreso / 🟢 Completado  
**Fecha inicio:** DD/MM/YYYY  
**Fecha fin:** DD/MM/YYYY  
**Tiempo estimado:** X días  
**Tiempo real:** X días

## Objetivos
- [ ] Objetivo 1
- [ ] Objetivo 2

## Tareas
- [ ] Tarea 1 (archivo.cpp)
- [ ] Tarea 2 (archivo.h)

## Tests
- [ ] Test 1
- [ ] Test 2

## Notas de Desarrollo
[Apuntes, decisiones, problemas encontrados]

## Criterio de Éxito
[Cómo validar que está completado]
```

---

## 🔧 HERRAMIENTAS Y TECNOLOGÍAS

### Desarrollo
- **IDE:** Visual Studio 2022 / CLion / Xcode
- **Build:** CMake 3.21+
- **Compiler:** MSVC 19.3+ / GCC 11+ / Clang 14+
- **JUCE:** 7.0.12 (última estable)

### Testing
- **Framework:** GoogleTest 1.14+
- **Coverage:** gcov / llvm-cov
- **Profiling:** Visual Studio Profiler / Instruments / Valgrind

### Audio
- **Sample Rate:** 44.1kHz, 48kHz, 96kHz
- **Buffer Size:** 64-2048 samples
- **Latency Target:** <10ms (JACK), <20ms (ALSA)

### Control de Versiones
- **Git:** Commits atómicos, mensajes descriptivos
- **Branches:** `main`, `develop`, `feature/X`, `bugfix/X`
- **Tags:** `v0.1.0`, `v0.2.0`, etc.

---

## 📝 CONVENCIONES DE CÓDIGO

### Naming
```cpp
// Clases: PascalCase
class PhaseDistOscillator {};

// Funciones: camelCase
void processAudioBlock() {}

// Variables: camelCase
float sampleRate = 44100.0f;

// Constantes: UPPER_SNAKE_CASE
static constexpr int MAX_VOICES = 8;

// Miembros privados: camelCase con prefijo m_
float m_phase = 0.0f;
```

### Headers
```cpp
#pragma once

#include <juce_core/juce_core.h>  // JUCE primero
#include <vector>                  // STL después
#include "OtroHeader.h"            // Headers propios al final

namespace CZ101 {
namespace DSP {

class MiClase {
public:
    // Constructores
    MiClase();
    ~MiClase() = default;
    
    // Métodos públicos
    void metodoPublico() noexcept;
    
private:
    // Miembros privados
    float m_variable;
    
    // Métodos privados
    void metodoPrivado();
};

} // namespace DSP
} // namespace CZ101
```

---

## 🎯 PRÓXIMOS PASOS INMEDIATOS

### Acción 1: Crear Documentación Base
- [ ] `01_ARCHITECTURE.md` - Arquitectura detallada
- [ ] `02_MILESTONES.md` - Tracking de hitos
- [ ] `03_DSP_SPECS.md` - Especificaciones DSP
- [ ] `04_UI_DESIGN.md` - Diseño de interfaz
- [ ] `05_TESTING.md` - Estrategia de testing

### Acción 2: Setup Proyecto
- [ ] Crear estructura de directorios
- [ ] Configurar CMakeLists.txt
- [ ] Crear plugin JUCE vacío
- [ ] Configurar GoogleTest
- [ ] Primer commit

### Acción 3: Milestone 0 - Infraestructura
- [ ] Plugin compila en Windows
- [ ] Plugin carga en DAW (Reaper/Ableton)
- [ ] Sistema de logging funcional
- [ ] Primer test unitario pasa

---

**Última actualización:** 14 Diciembre 2025  
**Versión del plan:** 1.0
