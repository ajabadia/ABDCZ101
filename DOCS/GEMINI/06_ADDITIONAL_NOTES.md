# CZ-101 EMULATOR - NOTAS ADICIONALES DE IMPLEMENTACIÓN

**Versión:** 1.0  
**Fecha:** 14 Diciembre 2025  
**Fuente:** CZ 101 - ia.docx (convertido)

---

## ⚠️ CONSEJOS CRÍTICOS DE IMPLEMENTACIÓN

### 1. Thread Safety (CRÍTICO)

**Problema:** Modificar parámetros en el audio thread causa crashes y glitches

**Solución:**
```cpp
// NUNCA hacer esto en processBlock():
// myParameter = newValue;  // ❌ INCORRECTO

// SIEMPRE usar AudioProcessorValueTreeState:
juce::AudioProcessorValueTreeState apvts;  // ✅ CORRECTO

// En processBlock():
float paramValue = apvts.getRawParameterValue("paramID")->load();
```

**Referencia:** Sección 10.1 del documento original

---

### 2. Denormalizados (Performance)

**Problema:** Números denormalizados causan caída dramática de performance en síntesis

**Solución:**
```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    // Agregar al inicio de CADA processBlock
    juce::ScopedNoDenormals noDenormals;
    
    // ... resto del código
}
```

**Qué son:** Números muy pequeños (cerca de 0) que causan operaciones lentas en CPU

**Referencia:** Sección 10.2 del documento original

---

### 3. Pitch Envelope (Diferencia Clave)

**Importante:** En el CZ-101 real, el pitch envelope modula la **frecuencia del oscilador**, no solo el timbre.

**Implementación correcta:**
```cpp
float Voice::processSample() {
    // Obtener modulación de pitch envelope
    float pitchMod = pitchEnv.process();  // 0.0 - 1.0
    
    // Convertir a semitonos (rango típico: ±12 semitonos)
    float pitchSemitones = (pitchMod - 0.5f) * 24.0f;  // -12 a +12
    
    // Aplicar a frecuencia del oscilador
    float pitchRatio = std::pow(2.0f, pitchSemitones / 12.0f);
    float modulatedFreq = baseFrequency * pitchRatio;
    
    osc1.setFrequency(modulatedFreq);
    
    // ... resto del procesamiento
}
```

**Referencia:** Sección 10.4 del documento original

---

### 4. Voice Stealing Eficiente

**Estrategia recomendada:**

```cpp
int VoiceManager::findVoiceToSteal() {
    // Prioridad 1: Buscar voz en release phase
    for (int i = 0; i < MAX_VOICES; ++i) {
        if (voices[i].isInReleasePhase()) {
            return i;
        }
    }
    
    // Prioridad 2: Buscar voz más antigua
    int oldestVoice = 0;
    uint32_t oldestTime = voices[0].getNoteOnTime();
    
    for (int i = 1; i < MAX_VOICES; ++i) {
        if (voices[i].getNoteOnTime() < oldestTime) {
            oldestTime = voices[i].getNoteOnTime();
            oldestVoice = i;
        }
    }
    
    return oldestVoice;
}
```

**Referencia:** Sección 10.3 del documento original

---

### 5. Testing para Artifacts

**Pruebas críticas:**

1. **Notas rápidas:** Tocar staccato rápido para detectar clicks
2. **Arpegios:** Secuencias rápidas para voice stealing
3. **Glissando:** Pitch bend continuo para detectar discontinuidades
4. **Sustain pedal:** Mantener 10+ notas y soltar pedal

**Código de test:**
```cpp
TEST(VoiceTest, NoClicksOnFastNotes) {
    Voice voice;
    voice.prepare(44100.0);
    
    std::vector<float> samples;
    
    // Tocar nota muy corta (10ms)
    voice.noteOn(60, 0.8f);
    for (int i = 0; i < 441; ++i) {  // 10ms @ 44.1kHz
        samples.push_back(voice.processSample());
    }
    voice.noteOff();
    
    // Procesar release
    for (int i = 0; i < 1000; ++i) {
        samples.push_back(voice.processSample());
    }
    
    // Buscar clicks (cambios bruscos)
    for (size_t i = 1; i < samples.size(); ++i) {
        float diff = std::abs(samples[i] - samples[i-1]);
        EXPECT_LT(diff, 0.5f);  // No debe haber saltos >0.5
    }
}
```

**Referencia:** Sección 10.5 del documento original

---

## 🏗️ ARQUITECTURA ESPECÍFICA DEL CZ-101

### Características Únicas

**1. Dos "Lines" (Líneas) por voz:**
- Line 1: Oscilador 1 + DCW Env 1 + DCA Env 1
- Line 2: Oscilador 2 + DCW Env 2 + DCA Env 2
- Cada línea es independiente

**2. Modos de mezcla:**
```cpp
enum class MixMode {
    Osc1Only,    // Solo oscilador 1
    Add,         // Osc1 + Osc2 (mezcla aditiva)
    Multiply     // Osc1 * Osc2 (ring modulation)
};
```

**3. Envelopes de 8 segmentos:**
- No es ADSR tradicional
- 8 puntos configurables (Rate + Level)
- Sustain point configurable (típicamente segmento 3 o 4)

**Referencia:** Secciones 1-5 del documento original

---

## 📚 RECURSOS ADICIONALES

### Libros Recomendados

1. **"Creating Synthesizer Plug-Ins with C++ and JUCE"**
   - URL: https://theaudioprogrammer.com/synth-plugin-book
   - Cubre: JUCE, DSP, UI, distribución

2. **"Designing Audio Effect Plugins in C++"** (Will Pirkle)
   - Cubre: DSP algorithms, optimization

### Comunidades

1. **JUCE Forum**
   - URL: forum.juce.com
   - Muy activo, respuestas rápidas

2. **TheAudioProgrammer (YouTube)**
   - Tutoriales de JUCE
   - Ejemplos prácticos

3. **WolfSound**
   - Audio DSP fundamentals
   - Matemáticas de síntesis

**Referencia:** Sección 9 del documento original

---

## 🔧 CMAKE CONFIGURATION

### CMakeLists.txt Recomendado

```cmake
cmake_minimum_required(VERSION 3.19)

project(CZ101Emulator VERSION 1.0.0)

# C++17 mínimo
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Agregar JUCE
add_subdirectory(JUCE)

# Crear plugin
juce_add_plugin(CZ101Emulator
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE Cz01
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "CZ-101 Emulator"
)

# Source files
target_sources(CZ101Emulator PRIVATE
    src/plugin/CZ101AudioProcessor.cpp
    src/plugin/CZ101Editor.cpp
    src/dsp/PhaseDist.cpp
    src/dsp/Envelope.cpp
    src/dsp/Voice.cpp
    # ... más archivos
)

# JUCE modules
target_link_libraries(CZ101Emulator PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
)

# Compile definitions
target_compile_definitions(CZ101Emulator PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)
```

**Referencia:** Sección 2 del documento original

---

## 🎯 PLAN DE FASES (del documento original)

### Comparación con nuestro plan

| Fase Original | Nuestro Milestone | Tiempo Original | Nuestro Tiempo |
|---------------|-------------------|-----------------|----------------|
| Fase 1: Estructura Base | Milestone 0-1 | 1-2 semanas | 5-7 días |
| Fase 2: Síntesis Completa | Milestone 1-2 | 2-3 semanas | 6-8 días |
| Fase 3: UI Funcional | Milestone 7 | 2 semanas | 4-5 días |
| Fase 4: Características Avanzadas | Milestone 4,8 | 2-3 semanas | 5-7 días |
| Fase 5: Optimización | Milestone 9 | 1-2 semanas | 3-4 días |
| **TOTAL** | **10 Milestones** | **8-12 semanas** | **~30 días** |

**Conclusión:** Nuestro plan es más granular y optimista. Considerar ajustar a 8-10 semanas para ser realistas.

---

## ⚡ OPTIMIZACIONES IMPORTANTES

### 1. Evitar Allocations en Audio Thread

```cpp
// ❌ INCORRECTO (alloca memoria)
void processBlock(...) {
    std::vector<float> tempBuffer;  // ❌ new/delete en cada bloque
    tempBuffer.resize(bufferSize);
}

// ✅ CORRECTO (pre-allocado)
class Processor {
private:
    std::vector<float> tempBuffer;  // Miembro de clase
    
public:
    void prepareToPlay(double sr, int maxBlockSize) {
        tempBuffer.resize(maxBlockSize);  // Allocar UNA VEZ
    }
    
    void processBlock(...) {
        // Usar tempBuffer sin allocar
    }
};
```

### 2. SIMD para Operaciones Vectoriales

```cpp
// Para operaciones en bloques grandes, usar JUCE DSP
juce::dsp::AudioBlock<float> block(buffer);
juce::dsp::ProcessContextReplacing<float> context(block);

// Procesar con SIMD automático
myProcessor.process(context);
```

---

## 📝 NOTAS FINALES

### Información Nueva Encontrada

1. ✅ **Consejos de thread safety** - Agregados arriba
2. ✅ **Denormalizados** - Crítico para performance
3. ✅ **Pitch envelope behavior** - Diferencia importante
4. ✅ **Voice stealing strategy** - Implementación específica
5. ✅ **Testing para artifacts** - Casos de prueba concretos
6. ✅ **CMake configuration** - Template completo
7. ✅ **Recursos y comunidades** - Links útiles

### Información Ya Cubierta

- ✅ Arquitectura general (en 01_ARCHITECTURE.md)
- ✅ Phase Distortion (en 03_DSP_SPECS.md)
- ✅ Envelopes de 8 segmentos (en 03_DSP_SPECS.md)
- ✅ UI Design (en 04_UI_DESIGN.md)
- ✅ Testing strategy (en 05_TESTING.md)

---

## 🎯 ACCIÓN RECOMENDADA

**Actualizar documentos existentes con:**

1. **02_MILESTONES.md:**
   - Ajustar tiempo estimado a 8-10 semanas (más realista)
   - Agregar nota sobre denormalizados en Milestone 1

2. **03_DSP_SPECS.md:**
   - Agregar sección sobre Pitch Envelope behavior
   - Incluir nota sobre ring modulation (Multiply mode)

3. **05_TESTING.md:**
   - Agregar test cases específicos para artifacts
   - Incluir test de voice stealing

4. **Crear nuevo:** `06_BEST_PRACTICES.md`
   - Thread safety
   - Performance optimization
   - Common pitfalls

---

**Última actualización:** 14 Diciembre 2025  
**Fuente:** CZ 101 - ia.docx (17,727 líneas)
