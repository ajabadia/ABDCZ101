# CZ-101 EMULATOR - ARQUITECTURA DETALLADA

**Versión:** 1.0  
**Fecha:** 14 Diciembre 2025  
**Autor:** Equipo de Desarrollo

---

## 🏛️ VISIÓN GENERAL

El CZ-101 Emulator sigue una **arquitectura modular en capas** con separación estricta de responsabilidades:

```
┌─────────────────────────────────────────────────────────┐
│                    JUCE Framework                        │
│                  (Audio, MIDI, UI)                       │
└─────────────────────────────────────────────────────────┘
                            ▲
                            │
┌─────────────────────────────────────────────────────────┐
│                  PluginProcessor                         │
│              (Orquestador principal)                     │
└─────────────────────────────────────────────────────────┘
         │              │              │              │
         ▼              ▼              ▼              ▼
┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│     Core     │ │     MIDI     │ │    State     │ │      UI      │
│   (Synth)    │ │  (Processor) │ │  (Presets)   │ │   (Editor)   │
└──────────────┘ └──────────────┘ └──────────────┘ └──────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────┐
│                      DSP Layer                           │
│  (Oscillators, Envelopes, Filters, Effects, Modulation) │
└─────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────┐
│                    Utils Layer                           │
│         (Constants, Math, Logging, Helpers)              │
└─────────────────────────────────────────────────────────┘
```

---

## 📦 MÓDULOS PRINCIPALES

### 1. PluginProcessor (Orquestador)

**Responsabilidad:** Coordinar todos los módulos y gestionar el ciclo de vida del plugin

**Archivo:** `Source/PluginProcessor.h/cpp`

**Funciones clave:**
```cpp
class CZ101AudioProcessor : public juce::AudioProcessor {
public:
    // Ciclo de vida JUCE
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void releaseResources() override;
    
    // State management
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    
    // Editor
    juce::AudioProcessorEditor* createEditor() override;
    
private:
    std::unique_ptr<Core::SynthEngine> m_synthEngine;
    std::unique_ptr<MIDI::MIDIProcessor> m_midiProcessor;
    std::unique_ptr<State::PresetManager> m_presetManager;
    juce::AudioProcessorValueTreeState m_parameters;
};
```

**Dependencias:** Core, MIDI, State

---

### 2. Core (Motor de Síntesis)

**Responsabilidad:** Lógica principal del sintetizador, gestión de voces

**Archivos:**
- `Source/Core/SynthEngine.h/cpp` - Motor principal
- `Source/Core/VoiceManager.h/cpp` - Gestión de 8 voces
- `Source/Core/Voice.h/cpp` - Voz individual

#### 2.1 SynthEngine

```cpp
namespace Core {

class SynthEngine {
public:
    SynthEngine();
    
    // Inicialización
    void prepare(double sampleRate, int maxBlockSize);
    
    // Procesamiento principal
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                        const juce::MidiBuffer& midiMessages);
    
    // Control
    void noteOn(int midiNote, float velocity);
    void noteOff(int midiNote);
    void allNotesOff();
    
    // Parámetros
    void setParameter(const juce::String& paramID, float value);
    
private:
    std::unique_ptr<VoiceManager> m_voiceManager;
    std::unique_ptr<DSP::Effects::EffectsChain> m_effectsChain;
    
    double m_sampleRate = 44100.0;
    int m_maxBlockSize = 512;
};

} // namespace Core
```

#### 2.2 VoiceManager

```cpp
namespace Core {

class VoiceManager {
public:
    static constexpr int MAX_VOICES = 8;
    
    VoiceManager();
    
    // Gestión de voces
    void noteOn(int midiNote, float velocity);
    void noteOff(int midiNote);
    void allNotesOff();
    
    // Renderizado
    void renderNextBlock(juce::AudioBuffer<float>& buffer);
    
    // Voice stealing
    int findFreeVoice();
    int findVoiceToSteal();
    
private:
    std::array<std::unique_ptr<Voice>, MAX_VOICES> m_voices;
    
    // Voice stealing strategy
    enum class StealingMode {
        Oldest,
        Quietest,
        ReleasePhase
    };
    StealingMode m_stealingMode = StealingMode::ReleasePhase;
};

} // namespace Core
```

#### 2.3 Voice

```cpp
namespace Core {

class Voice {
public:
    Voice();
    
    // Control
    void noteOn(int midiNote, float velocity);
    void noteOff();
    void kill(); // Immediate stop
    
    // Estado
    bool isActive() const noexcept { return m_isActive; }
    bool isInReleasePhase() const noexcept;
    int getCurrentNote() const noexcept { return m_currentNote; }
    float getCurrentLevel() const noexcept;
    
    // Procesamiento
    float renderNextSample();
    
    // Configuración
    void prepare(double sampleRate);
    void setOscillatorWaveform(int oscIndex, DSP::Waveform waveform);
    void setEnvelopeParams(/* ... */);
    
private:
    // Componentes DSP
    std::array<DSP::PhaseDistOscillator, 2> m_oscillators; // DCO1, DCO2
    DSP::MultiStageEnvelope m_dcwEnvelope;
    DSP::MultiStageEnvelope m_dcaEnvelope;
    DSP::DCWFilter m_dcwFilter;
    
    // Estado
    bool m_isActive = false;
    int m_currentNote = -1;
    float m_velocity = 0.0f;
    double m_sampleRate = 44100.0;
};

} // namespace Core
```

---

### 3. DSP (Procesamiento de Señal)

**Responsabilidad:** Algoritmos de síntesis, filtros, efectos, modulación

**Estructura:**
```
DSP/
├── Oscillators/
│   ├── PhaseDistOsc.h/cpp      # Oscilador principal
│   ├── WaveTable.h/cpp         # Tablas de ondas
│   └── WaveShaper.h/cpp        # Distorsión de fase
├── Envelopes/
│   ├── ADSREnvelope.h/cpp      # Envelope básico
│   └── MultiStageEnv.h/cpp     # Envelope 8 etapas
├── Filters/
│   └── DCWFilter.h/cpp         # Filtro DCW (Digital Controlled Wave)
├── Effects/
│   ├── EffectsChain.h/cpp      # Cadena de efectos
│   ├── Reverb.h/cpp            # Reverb
│   ├── Chorus.h/cpp            # Chorus
│   └── Delay.h/cpp             # Delay
└── Modulation/
    ├── LFO.h/cpp               # Low Frequency Oscillator
    └── ModMatrix.h/cpp         # Matriz de modulación
```

#### 3.1 PhaseDistOscillator

```cpp
namespace DSP {

enum class Waveform {
    Sine,
    Sawtooth,
    Square,
    Triangle,
    Pulse,
    DoubleSine,
    HalfSine,
    ResonantSaw,
    ResonantTriangle,
    Trapezoid
};

class PhaseDistOscillator {
public:
    PhaseDistOscillator();
    
    // Configuración
    void prepare(double sampleRate);
    void setFrequency(float frequencyHz) noexcept;
    void setWaveform(Waveform waveform) noexcept;
    void setPhaseDistortion(float amount) noexcept; // 0.0 - 1.0
    
    // Procesamiento
    float renderNextSample() noexcept;
    
    // Reset
    void reset() noexcept;
    
private:
    double m_sampleRate = 44100.0;
    float m_frequency = 440.0f;
    float m_phase = 0.0f;
    float m_phaseIncrement = 0.0f;
    Waveform m_waveform = Waveform::Sine;
    float m_phaseDistAmount = 0.0f;
    
    // Helpers
    float applyPhaseDistortion(float phase) const noexcept;
    float lookupWaveTable(float phase) const noexcept;
};

} // namespace DSP
```

#### 3.2 MultiStageEnvelope

```cpp
namespace DSP {

class MultiStageEnvelope {
public:
    enum class Stage {
        Idle,
        Attack,
        Decay1,
        Decay2,
        Sustain,
        Release
    };
    
    MultiStageEnvelope();
    
    // Control
    void noteOn() noexcept;
    void noteOff() noexcept;
    void reset() noexcept;
    
    // Configuración
    void setAttackTime(float timeMs) noexcept;
    void setDecayTime(float timeMs) noexcept;
    void setSustainLevel(float level) noexcept; // 0.0 - 1.0
    void setReleaseTime(float timeMs) noexcept;
    
    // Procesamiento
    float getNextValue() noexcept;
    
    // Estado
    bool isActive() const noexcept { return m_currentStage != Stage::Idle; }
    Stage getCurrentStage() const noexcept { return m_currentStage; }
    
private:
    Stage m_currentStage = Stage::Idle;
    float m_currentLevel = 0.0f;
    float m_targetLevel = 0.0f;
    float m_attackTime = 100.0f;
    float m_decayTime = 500.0f;
    float m_sustainLevel = 0.8f;
    float m_releaseTime = 300.0f;
    
    double m_sampleRate = 44100.0;
    int m_sampleCounter = 0;
    
    // Helpers
    float calculateExponentialCurve(float current, float target, float time) const noexcept;
};

} // namespace DSP
```

---

### 4. MIDI (Procesamiento MIDI)

**Responsabilidad:** Manejo de eventos MIDI, SysEx, CC mapping

**Archivos:**
- `Source/MIDI/MIDIProcessor.h/cpp` - Procesador principal
- `Source/MIDI/SysExHandler.h/cpp` - SysEx bidireccional
- `Source/MIDI/CCMapper.h/cpp` - MIDI CC a parámetros

```cpp
namespace MIDI {

class MIDIProcessor {
public:
    MIDIProcessor(Core::SynthEngine& synthEngine);
    
    // Procesamiento
    void processMidiBuffer(const juce::MidiBuffer& midiMessages);
    
    // Configuración
    void setMidiChannel(int channel); // 1-16, 0 = omni
    void setPitchBendRange(int semitones);
    
private:
    Core::SynthEngine& m_synthEngine;
    
    void handleNoteOn(const juce::MidiMessage& msg);
    void handleNoteOff(const juce::MidiMessage& msg);
    void handlePitchBend(const juce::MidiMessage& msg);
    void handleControlChange(const juce::MidiMessage& msg);
    void handleSysEx(const juce::MidiMessage& msg);
    
    int m_midiChannel = 0; // 0 = omni
    int m_pitchBendRange = 2; // semitones
};

} // namespace MIDI
```

---

### 5. State (Gestión de Estado)

**Responsabilidad:** Parámetros, presets, serialización

**Archivos:**
- `Source/State/Parameters.h/cpp` - Definición de parámetros
- `Source/State/Preset.h/cpp` - Estructura de preset
- `Source/State/PresetManager.h/cpp` - Carga/guardado

```cpp
namespace State {

struct Preset {
    juce::String name;
    juce::String category;
    
    // Osciladores
    struct OscillatorParams {
        int waveform;      // 0-9
        int pitch;         // -48 a +48 semitonos
        float detune;      // -100 a +100 cents
        float volume;      // 0.0 - 1.0
    };
    OscillatorParams dco1;
    OscillatorParams dco2;
    
    // Envelopes
    struct EnvelopeParams {
        float attackMs;
        float decayMs;
        float sustainLevel;
        float releaseMs;
    };
    EnvelopeParams dcwEnvelope;
    EnvelopeParams dcaEnvelope;
    
    // Efectos
    struct EffectsParams {
        float reverbMix;
        float chorusRate;
        float chorusDepth;
        float delayTime;
        float delayFeedback;
    };
    EffectsParams effects;
    
    // Serialización
    juce::var toJSON() const;
    static Preset fromJSON(const juce::var& json);
};

} // namespace State
```

---

## 🔄 FLUJO DE DATOS

### Audio Thread (Real-time)

```
MIDI Input
    │
    ▼
MIDIProcessor
    │
    ▼
SynthEngine::renderNextBlock()
    │
    ├─► VoiceManager::renderNextBlock()
    │       │
    │       ├─► Voice[0]::renderNextSample()
    │       ├─► Voice[1]::renderNextSample()
    │       └─► ... (8 voces)
    │
    └─► EffectsChain::process()
            │
            ├─► Reverb::process()
            ├─► Chorus::process()
            └─► Delay::process()
    │
    ▼
Audio Output
```

### UI Thread (Non-realtime)

```
User Interaction
    │
    ▼
UI Component (Knob, Button, etc)
    │
    ▼
AudioProcessorValueTreeState
    │
    ▼
PluginProcessor::parameterChanged()
    │
    ▼
SynthEngine::setParameter()
    │
    ▼
Lock-free FIFO
    │
    ▼
Audio Thread reads parameter
```

---

## 🔒 THREAD SAFETY

### Reglas Estrictas

1. **Audio Thread:**
   - ❌ NO allocations (new, malloc, vector::push_back)
   - ❌ NO locks (mutex, critical section)
   - ❌ NO file I/O
   - ✅ SÍ lock-free structures
   - ✅ SÍ atomic operations

2. **UI Thread:**
   - ✅ Puede hacer allocations
   - ✅ Puede usar locks
   - ✅ Puede hacer file I/O
   - ❌ NO acceso directo a DSP state

### Comunicación Thread-Safe

```cpp
// Ejemplo: Cambio de parámetro desde UI
class PluginProcessor {
private:
    // Lock-free FIFO para parámetros
    juce::AbstractFifo m_parameterFifo{32};
    std::array<ParameterChange, 32> m_parameterBuffer;
    
    // UI thread escribe
    void parameterChanged(const juce::String& paramID, float value) {
        int start1, size1, start2, size2;
        m_parameterFifo.prepareToWrite(1, start1, size1, start2, size2);
        
        if (size1 > 0) {
            m_parameterBuffer[start1] = {paramID, value};
            m_parameterFifo.finishedWrite(1);
        }
    }
    
    // Audio thread lee
    void processBlock(...) {
        int start1, size1, start2, size2;
        m_parameterFifo.prepareToRead(m_parameterFifo.getNumReady(),
                                      start1, size1, start2, size2);
        
        for (int i = 0; i < size1; ++i) {
            auto& change = m_parameterBuffer[start1 + i];
            m_synthEngine->setParameter(change.id, change.value);
        }
        
        m_parameterFifo.finishedRead(size1);
    }
};
```

---

## 📏 LÍMITES Y RESTRICCIONES

### Tamaño de Archivos
- **Máximo:** 300 líneas por archivo
- **Ideal:** 150-200 líneas
- **Si excede:** Dividir en archivos más pequeños

### Complejidad
- **Máximo:** 10 niveles de indentación
- **Funciones:** Máximo 50 líneas
- **Clases:** Una responsabilidad clara

### Performance
- **CPU:** <5% @ 8 voces + efectos (i5 2.5GHz)
- **Latencia:** <10ms (JACK), <20ms (ALSA)
- **Memory:** <50MB RAM total

---

## 🧪 TESTING

### Niveles de Testing

1. **Unit Tests:** Cada clase DSP individual
2. **Integration Tests:** Módulos completos (Core, MIDI, etc)
3. **System Tests:** Plugin completo end-to-end
4. **Performance Tests:** CPU, latencia, memory

### Ejemplo Test

```cpp
TEST(PhaseDistOscillatorTest, GeneratesSineWave) {
    DSP::PhaseDistOscillator osc;
    osc.prepare(44100.0);
    osc.setFrequency(440.0f);
    osc.setWaveform(DSP::Waveform::Sine);
    
    // Generar 100 samples
    std::vector<float> samples;
    for (int i = 0; i < 100; ++i) {
        samples.push_back(osc.renderNextSample());
    }
    
    // Validar rango
    for (float sample : samples) {
        EXPECT_GE(sample, -1.0f);
        EXPECT_LE(sample, 1.0f);
    }
    
    // Validar frecuencia (FFT analysis)
    // ...
}
```

---

**Próximo documento:** `02_MILESTONES.md` - Sistema de tracking de hitos
