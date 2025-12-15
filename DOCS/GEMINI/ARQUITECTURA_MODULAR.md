# ARQUITECTURA MODULAR - CZ-101 EMULATOR

**Última actualización:** 14 Diciembre 2025, 22:12

---

## 🏗️ ESTRUCTURA ACTUAL

```
Source/
├── DSP/
│   ├── Oscillators/          ✅ Milestone 1 (100%)
│   │   ├── WaveTable.h/cpp           (240 líneas)
│   │   ├── PhaseDistOsc.h/cpp        (235 líneas)
│   │   └── WaveShaper.h/cpp          (98 líneas)
│   │
│   ├── Envelopes/            🟡 Milestone 2 (8%)
│   │   ├── ADSREnvelope.h/cpp        (220 líneas) ✅
│   │   └── MultiStageEnv.h/cpp       (pendiente)
│   │
│   ├── Filters/              🔴 Milestone 5
│   ├── Effects/              🔴 Milestone 5
│   └── Modulation/           🔴 Milestone 4
│
├── Core/                     🔴 Milestone 2 (Día 3-4)
│   ├── Voice.h/cpp           (pendiente)
│   └── VoiceManager.h/cpp    (Milestone 3)
│
├── MIDI/                     🔴 Milestone 3
│   ├── MIDIProcessor.h/cpp
│   └── SysExHandler.h/cpp
│
├── State/                    🔴 Milestone 6
│   ├── Parameters.h/cpp
│   ├── Preset.h/cpp
│   └── PresetManager.h/cpp
│
└── UI/                       🔴 Milestone 7
    ├── Components/
    └── LookAndFeel/
```

---

## 📐 PRINCIPIOS DE DISEÑO

### 1. Separación por Responsabilidad

**DSP/Oscillators:**
- Generación de waveforms
- Phase distortion
- Anti-aliasing (PolyBLEP)

**DSP/Envelopes:**
- Modulación temporal
- ADSR y multi-stage
- Curvas exponenciales

**Core:**
- Arquitectura de voces
- Integración DSP
- Voice stealing

**MIDI:**
- Procesamiento MIDI
- SysEx parsing
- CC mapping

**State:**
- Gestión de parámetros
- Presets
- Serialización

**UI:**
- Interfaz gráfica
- Componentes custom
- Look & Feel

### 2. Namespaces Jerárquicos

```cpp
namespace CZ101 {
    namespace DSP {
        class WaveTable { };
        class PhaseDistOscillator { };
        class ADSREnvelope { };
    }
    
    namespace Core {
        class Voice { };
        class VoiceManager { };
    }
    
    namespace MIDI {
        class MIDIProcessor { };
    }
    
    namespace State {
        class PresetManager { };
    }
}
```

### 3. Headers Ligeros

**Regla:** Solo declaraciones en .h

```cpp
// WaveTable.h
#pragma once
#include <array>
#include <cmath>

namespace CZ101::DSP {
    class WaveTable {
        // Declaraciones
    };
}
```

**Implementación en .cpp:**
```cpp
// WaveTable.cpp
#include "WaveTable.h"
#include <algorithm>

namespace CZ101::DSP {
    // Implementaciones
}
```

### 4. Bajo Acoplamiento

**Independencia de módulos:**
- WaveTable NO depende de PhaseDistOsc
- ADSREnvelope NO depende de osciladores
- Cada módulo es auto-contenido

**Integración en capas superiores:**
```cpp
// Voice.h (capa superior)
#include "DSP/Oscillators/PhaseDistOsc.h"
#include "DSP/Envelopes/ADSREnvelope.h"

class Voice {
    PhaseDistOscillator osc1, osc2;
    ADSREnvelope dcwEnv, dcaEnv;
};
```

### 5. Alta Cohesión

**Cada clase = Una responsabilidad:**
- WaveTable: Solo tablas de waveforms
- PhaseDistOsc: Solo generación de oscilador
- ADSREnvelope: Solo envelope ADSR
- Voice: Solo integración de componentes

**Tamaño de archivos:**
- Headers: <100 líneas
- Implementación: <300 líneas
- Si excede: dividir en sub-módulos

---

## 🔗 DEPENDENCIAS

### Actuales

```
PhaseDistOsc
    ↓
WaveTable (composición)

Voice (futuro)
    ↓
PhaseDistOsc + ADSREnvelope (composición)
```

### Futuras

```
VoiceManager
    ↓
Voice (array de 8)
    ↓
PhaseDistOsc + ADSREnvelope

MIDIProcessor
    ↓
VoiceManager (noteOn/noteOff)
```

---

## 📊 MÉTRICAS DE MODULARIDAD

| Módulo | Archivos | Líneas | Acoplamiento | Cohesión |
|--------|----------|--------|--------------|----------|
| Oscillators | 6 | 573 | Bajo | Alta |
| Envelopes | 2 | 220 | Bajo | Alta |
| Core | 0 | 0 | - | - |
| **Total** | **8** | **793** | **Bajo** | **Alta** |

---

## ✅ VENTAJAS DE LA MODULARIZACIÓN

1. **Testeable:** Cada módulo se puede testear independientemente
2. **Mantenible:** Cambios localizados
3. **Reutilizable:** Componentes pueden usarse en otros proyectos
4. **Escalable:** Fácil agregar nuevos módulos
5. **Comprensible:** Estructura clara y lógica

---

## 🎯 PRÓXIMOS MÓDULOS

### Milestone 2 (Continuación)
- MultiStageEnv.h/cpp (Envelopes)
- Voice.h/cpp (Core)

### Milestone 3
- VoiceManager.h/cpp (Core)
- MIDIProcessor.h/cpp (MIDI)

---

**Estado:** ✅ Arquitectura modular bien establecida  
**Calidad:** Alta cohesión, bajo acoplamiento
