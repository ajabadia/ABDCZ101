# CZ-101 EMULATOR - ESTRATEGIA DE TESTING

**Versión:** 1.0  
**Fecha:** 14 Diciembre 2025  
**Framework:** GoogleTest + JUCE UnitTest

---

## 🎯 OBJETIVOS DE TESTING

### Metas

1. **Cobertura:** >80% del código DSP
2. **Confiabilidad:** 0 crashes en 24h de uso continuo
3. **Performance:** CPU <5% @ 8 voces + efectos
4. **Regresión:** Todos los tests pasan antes de cada commit

---

## 📊 NIVELES DE TESTING

### Pirámide de Testing

```
           ┌─────────────┐
           │   Manual    │  5%  - Testing manual en DAW
           │   Testing   │
           ├─────────────┤
           │ Integration │  15% - Tests de módulos completos
           │    Tests    │
           ├─────────────┤
           │    Unit     │  80% - Tests de clases individuales
           │    Tests    │
           └─────────────┘
```

---

## 🧪 UNIT TESTS

### Estructura

```
Tests/
├── DSP/
│   ├── WaveTableTest.cpp
│   ├── PhaseDistOscTest.cpp
│   ├── WaveShaperTest.cpp
│   ├── ADSREnvelopeTest.cpp
│   ├── MultiStageEnvTest.cpp
│   ├── LFOTest.cpp
│   ├── ModMatrixTest.cpp
│   ├── ReverbTest.cpp
│   ├── ChorusTest.cpp
│   └── DelayTest.cpp
├── Core/
│   ├── VoiceTest.cpp
│   └── VoiceManagerTest.cpp
├── MIDI/
│   ├── MIDIProcessorTest.cpp
│   └── CCMapperTest.cpp
└── State/
    ├── PresetTest.cpp
    └── PresetManagerTest.cpp
```

### Ejemplo: WaveTableTest.cpp

```cpp
#include <gtest/gtest.h>
#include "Source/DSP/Oscillators/WaveTable.h"

class WaveTableTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup común para todos los tests
    }
};

TEST_F(WaveTableTest, SineWaveHasCorrectRange) {
    DSP::WaveTable waveTable;
    auto sineTable = waveTable.getSineTable();
    
    for (float sample : sineTable) {
        EXPECT_GE(sample, -1.0f);
        EXPECT_LE(sample, 1.0f);
    }
}

TEST_F(WaveTableTest, SineWaveIsSymmetric) {
    DSP::WaveTable waveTable;
    auto sineTable = waveTable.getSineTable();
    
    // Primera mitad debe ser opuesta a segunda mitad
    for (int i = 0; i < 128; ++i) {
        EXPECT_NEAR(sineTable[i], -sineTable[i + 128], 0.01f);
    }
}

TEST_F(WaveTableTest, SineWaveStartsAtZero) {
    DSP::WaveTable waveTable;
    auto sineTable = waveTable.getSineTable();
    
    EXPECT_NEAR(sineTable[0], 0.0f, 0.01f);
}

TEST_F(WaveTableTest, SineWavePeakAtQuarter) {
    DSP::WaveTable waveTable;
    auto sineTable = waveTable.getSineTable();
    
    // Pico en 90° (índice 64)
    EXPECT_NEAR(sineTable[64], 1.0f, 0.01f);
}
```

### Ejemplo: PhaseDistOscTest.cpp

```cpp
#include <gtest/gtest.h>
#include "Source/DSP/Oscillators/PhaseDistOsc.h"
#include <cmath>

class PhaseDistOscTest : public ::testing::Test {
protected:
    DSP::PhaseDistOscillator osc;
    
    void SetUp() override {
        osc.prepare(44100.0);
    }
};

TEST_F(PhaseDistOscTest, GeneratesSineWave) {
    osc.setFrequency(440.0f);
    osc.setWaveform(DSP::Waveform::Sine);
    osc.setPhaseDistortion(0.0f);  // Sin distorsión
    
    std::vector<float> samples;
    for (int i = 0; i < 100; ++i) {
        samples.push_back(osc.renderNextSample());
    }
    
    // Verificar rango
    for (float sample : samples) {
        EXPECT_GE(sample, -1.0f);
        EXPECT_LE(sample, 1.0f);
    }
}

TEST_F(PhaseDistOscTest, FrequencyIsAccurate) {
    osc.setFrequency(440.0f);
    osc.setWaveform(DSP::Waveform::Sine);
    
    // Generar 1 segundo de audio
    std::vector<float> samples;
    for (int i = 0; i < 44100; ++i) {
        samples.push_back(osc.renderNextSample());
    }
    
    // Contar zero crossings
    int zeroCrossings = 0;
    for (size_t i = 1; i < samples.size(); ++i) {
        if (samples[i-1] < 0.0f && samples[i] >= 0.0f) {
            zeroCrossings++;
        }
    }
    
    // 440 Hz = 440 ciclos/segundo = 440 zero crossings positivos
    EXPECT_NEAR(zeroCrossings, 440, 5);  // ±5 Hz tolerancia
}

TEST_F(PhaseDistOscTest, PhaseDistortionChangesTimbre) {
    osc.setFrequency(440.0f);
    osc.setWaveform(DSP::Waveform::Sine);
    
    // Sin distorsión
    osc.setPhaseDistortion(0.0f);
    std::vector<float> samplesNoDist;
    for (int i = 0; i < 100; ++i) {
        samplesNoDist.push_back(osc.renderNextSample());
    }
    
    // Con distorsión
    osc.reset();
    osc.setPhaseDistortion(0.7f);
    std::vector<float> samplesWithDist;
    for (int i = 0; i < 100; ++i) {
        samplesWithDist.push_back(osc.renderNextSample());
    }
    
    // Verificar que son diferentes
    bool isDifferent = false;
    for (size_t i = 0; i < 100; ++i) {
        if (std::abs(samplesNoDist[i] - samplesWithDist[i]) > 0.1f) {
            isDifferent = true;
            break;
        }
    }
    
    EXPECT_TRUE(isDifferent);
}
```

### Ejemplo: ADSREnvelopeTest.cpp

```cpp
#include <gtest/gtest.h>
#include "Source/DSP/Envelopes/ADSREnvelope.h"

class ADSREnvelopeTest : public ::testing::Test {
protected:
    DSP::ADSREnvelope env;
    
    void SetUp() override {
        env.prepare(44100.0);
        env.setAttackTime(100.0f);   // 100ms
        env.setDecayTime(200.0f);    // 200ms
        env.setSustainLevel(0.7f);   // 70%
        env.setReleaseTime(150.0f);  // 150ms
    }
};

TEST_F(ADSREnvelopeTest, StartsAtZero) {
    EXPECT_FLOAT_EQ(env.getNextValue(), 0.0f);
}

TEST_F(ADSREnvelopeTest, ReachesMaxAfterAttack) {
    env.noteOn();
    
    // Simular 100ms @ 44.1kHz = 4410 samples
    float maxValue = 0.0f;
    for (int i = 0; i < 4410; ++i) {
        float value = env.getNextValue();
        if (value > maxValue) maxValue = value;
    }
    
    EXPECT_NEAR(maxValue, 1.0f, 0.05f);  // Debe llegar cerca de 1.0
}

TEST_F(ADSREnvelopeTest, DecaysToSustainLevel) {
    env.noteOn();
    
    // Esperar attack + decay = 300ms = 13230 samples
    for (int i = 0; i < 13230; ++i) {
        env.getNextValue();
    }
    
    // Ahora debe estar en sustain
    float sustainValue = env.getNextValue();
    EXPECT_NEAR(sustainValue, 0.7f, 0.05f);
}

TEST_F(ADSREnvelopeTest, ReleasesToZero) {
    env.noteOn();
    
    // Esperar a sustain
    for (int i = 0; i < 13230; ++i) {
        env.getNextValue();
    }
    
    env.noteOff();
    
    // Esperar release = 150ms = 6615 samples
    for (int i = 0; i < 6615; ++i) {
        env.getNextValue();
    }
    
    float finalValue = env.getNextValue();
    EXPECT_NEAR(finalValue, 0.0f, 0.05f);
}

TEST_F(ADSREnvelopeTest, IsNotActiveAfterRelease) {
    env.noteOn();
    EXPECT_TRUE(env.isActive());
    
    env.noteOff();
    
    // Esperar release completo
    for (int i = 0; i < 10000; ++i) {
        env.getNextValue();
    }
    
    EXPECT_FALSE(env.isActive());
}
```

---

## 🔗 INTEGRATION TESTS

### Objetivo

Verificar que módulos completos funcionan juntos correctamente.

### Ejemplo: VoiceIntegrationTest.cpp

```cpp
#include <gtest/gtest.h>
#include "Source/Core/Voice.h"

class VoiceIntegrationTest : public ::testing::Test {
protected:
    Core::Voice voice;
    
    void SetUp() override {
        voice.prepare(44100.0);
    }
};

TEST_F(VoiceIntegrationTest, NoteOnGeneratesSound) {
    voice.noteOn(60, 0.8f);  // C4, velocity 80%
    
    // Generar 1000 samples
    std::vector<float> samples;
    for (int i = 0; i < 1000; ++i) {
        samples.push_back(voice.renderNextSample());
    }
    
    // Verificar que hay sonido (no todo ceros)
    bool hasSound = false;
    for (float sample : samples) {
        if (std::abs(sample) > 0.01f) {
            hasSound = true;
            break;
        }
    }
    
    EXPECT_TRUE(hasSound);
}

TEST_F(VoiceIntegrationTest, NoteOffStopsSound) {
    voice.noteOn(60, 0.8f);
    
    // Generar sonido
    for (int i = 0; i < 1000; ++i) {
        voice.renderNextSample();
    }
    
    voice.noteOff();
    
    // Esperar release completo (3000ms = 132300 samples)
    for (int i = 0; i < 132300; ++i) {
        voice.renderNextSample();
    }
    
    // Ahora debe estar silencioso
    float sample = voice.renderNextSample();
    EXPECT_NEAR(sample, 0.0f, 0.01f);
    EXPECT_FALSE(voice.isActive());
}

TEST_F(VoiceIntegrationTest, VelocityAffectsVolume) {
    // Nota con velocity baja
    voice.noteOn(60, 0.2f);
    float sampleLow = 0.0f;
    for (int i = 0; i < 1000; ++i) {
        float s = voice.renderNextSample();
        if (std::abs(s) > std::abs(sampleLow)) sampleLow = s;
    }
    
    // Reset
    voice.kill();
    voice.prepare(44100.0);
    
    // Nota con velocity alta
    voice.noteOn(60, 1.0f);
    float sampleHigh = 0.0f;
    for (int i = 0; i < 1000; ++i) {
        float s = voice.renderNextSample();
        if (std::abs(s) > std::abs(sampleHigh)) sampleHigh = s;
    }
    
    // Velocity alta debe generar más volumen
    EXPECT_GT(std::abs(sampleHigh), std::abs(sampleLow));
}
```

---

## 🎭 PERFORMANCE TESTS

### CPU Usage Test

```cpp
TEST(PerformanceTest, CPUUsageUnder5Percent) {
    Core::SynthEngine engine;
    engine.prepare(44100.0, 512);
    
    // Tocar 8 notas simultáneas
    for (int note = 60; note < 68; ++note) {
        engine.noteOn(note, 0.8f);
    }
    
    // Medir tiempo de procesamiento
    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midiBuffer;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Procesar 1000 bloques
    for (int i = 0; i < 1000; ++i) {
        engine.renderNextBlock(buffer, midiBuffer);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Calcular CPU usage
    // 1000 bloques × 512 samples = 512000 samples
    // @ 44.1kHz = 11.61 segundos de audio
    double audioTimeSeconds = 11.61;
    double processingTimeSeconds = duration.count() / 1000000.0;
    double cpuUsage = (processingTimeSeconds / audioTimeSeconds) * 100.0;
    
    std::cout << "CPU Usage: " << cpuUsage << "%" << std::endl;
    
    EXPECT_LT(cpuUsage, 5.0);  // Menos de 5%
}
```

### Memory Leak Test

```cpp
TEST(PerformanceTest, NoMemoryLeaks) {
    // Usar Valgrind o Visual Studio Memory Profiler
    // Este test debe correr en CI/CD
    
    Core::SynthEngine engine;
    engine.prepare(44100.0, 512);
    
    // Tocar y liberar muchas notas
    for (int i = 0; i < 10000; ++i) {
        engine.noteOn(60 + (i % 12), 0.8f);
        
        // Procesar un poco
        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer midiBuffer;
        engine.renderNextBlock(buffer, midiBuffer);
        
        engine.noteOff(60 + (i % 12));
    }
    
    // Valgrind debe reportar 0 leaks
}
```

---

## 🎵 AUDIO ARTIFACT TESTS

### Test para Clicks (Crítico)

**Objetivo:** Detectar clicks en transiciones de notas rápidas

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

### Test para Arpegios Rápidos

**Objetivo:** Verificar voice stealing sin glitches

```cpp
TEST(VoiceManagerTest, ArpeggioWithoutGlitches) {
    VoiceManager vm;
    vm.prepare(44100.0);
    
    std::vector<float> samples;
    
    // Tocar arpegio rápido (16th notes @ 120 BPM = 125ms por nota)
    int notesPerBeat = 4;
    int samplesPerNote = 44100 * 0.125;  // 125ms
    
    for (int note = 60; note < 72; ++note) {  // C4 a B4
        vm.noteOn(note, 0.8f);
        
        for (int i = 0; i < samplesPerNote; ++i) {
            samples.push_back(vm.processSample());
        }
        
        vm.noteOff(note);
    }
    
    // Verificar que no hay discontinuidades
    int glitchCount = 0;
    for (size_t i = 1; i < samples.size(); ++i) {
        float diff = std::abs(samples[i] - samples[i-1]);
        if (diff > 0.3f) glitchCount++;
    }
    
    EXPECT_LT(glitchCount, 5);  // Máximo 5 glitches aceptables
}
```

### Test para Pitch Bend Continuo

**Objetivo:** Detectar discontinuidades en pitch bend

```cpp
TEST(VoiceTest, SmoothPitchBend) {
    Voice voice;
    voice.prepare(44100.0);
    voice.noteOn(60, 0.8f);
    
    std::vector<float> samples;
    
    // Pitch bend de -2 a +2 semitonos en 1 segundo
    for (int i = 0; i < 44100; ++i) {
        float bendAmount = -2.0f + (4.0f * i / 44100.0f);  // -2 a +2
        voice.setPitchBend(bendAmount);
        samples.push_back(voice.processSample());
    }
    
    // Verificar suavidad (no debe haber saltos bruscos)
    for (size_t i = 10; i < samples.size() - 10; ++i) {
        // Comparar con ventana de 10 samples
        float avg = 0.0f;
        for (int j = -5; j <= 5; ++j) {
            avg += samples[i + j];
        }
        avg /= 11.0f;
        
        float deviation = std::abs(samples[i] - avg);
        EXPECT_LT(deviation, 0.2f);
    }
}
```

### Test para Sustain Pedal con Muchas Notas

**Objetivo:** Verificar estabilidad con >8 notas sostenidas

```cpp
TEST(VoiceManagerTest, SustainPedalStressTest) {
    VoiceManager vm;
    vm.prepare(44100.0);
    
    // Activar sustain pedal
    vm.setSustainPedal(true);
    
    // Tocar 12 notas (más que las 8 voces disponibles)
    for (int note = 60; note < 72; ++note) {
        vm.noteOn(note, 0.8f);
        
        // Procesar un poco
        for (int i = 0; i < 100; ++i) {
            vm.processSample();
        }
    }
    
    // Soltar todas las teclas (pero sustain sigue activo)
    for (int note = 60; note < 72; ++note) {
        vm.noteOff(note);
    }
    
    // Procesar 1 segundo
    std::vector<float> samples;
    for (int i = 0; i < 44100; ++i) {
        samples.push_back(vm.processSample());
    }
    
    // Soltar sustain pedal
    vm.setSustainPedal(false);
    
    // Procesar release
    for (int i = 0; i < 44100; ++i) {
        samples.push_back(vm.processSample());
    }
    
    // Verificar que no hay crashes ni NaN
    for (float sample : samples) {
        EXPECT_FALSE(std::isnan(sample));
        EXPECT_FALSE(std::isinf(sample));
        EXPECT_GE(sample, -1.0f);
        EXPECT_LE(sample, 1.0f);
    }
}
```

**Referencia:** `06_ADDITIONAL_NOTES.md` sección 5

---

## 🎵 AUDIO VALIDATION TESTS

### Frequency Accuracy Test

```cpp
TEST(AudioValidationTest, OscillatorFrequencyAccurate) {
    DSP::PhaseDistOscillator osc;
    osc.prepare(44100.0);
    osc.setFrequency(440.0f);
    osc.setWaveform(DSP::Waveform::Sine);
    
    // Generar 1 segundo
    std::vector<float> samples;
    for (int i = 0; i < 44100; ++i) {
        samples.push_back(osc.renderNextSample());
    }
    
    // FFT analysis (usar JUCE FFT)
    juce::dsp::FFT fft(12);  // 2^12 = 4096 points
    std::vector<float> fftData(4096 * 2, 0.0f);
    
    // Copiar samples a fftData
    for (int i = 0; i < 4096; ++i) {
        fftData[i] = samples[i];
    }
    
    fft.performFrequencyOnlyForwardTransform(fftData.data());
    
    // Encontrar pico
    int peakBin = 0;
    float peakMagnitude = 0.0f;
    for (int i = 0; i < 2048; ++i) {
        if (fftData[i] > peakMagnitude) {
            peakMagnitude = fftData[i];
            peakBin = i;
        }
    }
    
    // Calcular frecuencia del pico
    float binFrequency = (44100.0f / 4096.0f) * peakBin;
    
    EXPECT_NEAR(binFrequency, 440.0f, 5.0f);  // ±5 Hz
}
```

---

## 🔄 REGRESSION TESTS

### Test Suite Completo

```bash
# Correr todos los tests antes de commit
./run_tests.sh

# Output esperado:
# [==========] Running 50 tests from 15 test suites.
# [----------] Global test environment set-up.
# [----------] 5 tests from WaveTableTest
# [ RUN      ] WaveTableTest.SineWaveHasCorrectRange
# [       OK ] WaveTableTest.SineWaveHasCorrectRange (0 ms)
# ...
# [==========] 50 tests from 15 test suites ran. (1234 ms total)
# [  PASSED  ] 50 tests.
```

### CI/CD Integration

```yaml
# .github/workflows/tests.yml
name: Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-latest, macos-latest, windows-latest]
    
    steps:
    - uses: actions/checkout@v2
    
    - name: Install dependencies
      run: |
        # Instalar JUCE, GoogleTest, etc
    
    - name: Build
      run: |
        mkdir build
        cd build
        cmake ..
        cmake --build .
    
    - name: Run tests
      run: |
        cd build
        ctest --output-on-failure
    
    - name: Upload coverage
      if: matrix.os == 'ubuntu-latest'
      run: |
        # Upload a Codecov
```

---

## 📋 TEST CHECKLIST

### Antes de cada Milestone

- [ ] Todos los unit tests pasan
- [ ] Todos los integration tests pasan
- [ ] Performance tests pasan
- [ ] No memory leaks (Valgrind)
- [ ] Code coverage >80%
- [ ] Manual testing en DAW

### Antes de cada Release

- [ ] Regression tests completos
- [ ] Audio validation tests
- [ ] Cross-platform testing (Win/Mac/Linux)
- [ ] 24h stability test
- [ ] User acceptance testing

---

## 🛠️ HERRAMIENTAS

### Testing Frameworks

- **GoogleTest:** Unit tests
- **JUCE UnitTest:** Alternativa integrada con JUCE
- **Catch2:** Otra opción popular

### Profiling

- **Valgrind:** Memory leaks (Linux)
- **Instruments:** Performance profiling (macOS)
- **Visual Studio Profiler:** Performance (Windows)

### Coverage

- **gcov/lcov:** Code coverage (Linux)
- **llvm-cov:** Code coverage (macOS)
- **OpenCppCoverage:** Code coverage (Windows)

---

## 📈 MÉTRICAS DE CALIDAD

### Targets

| Métrica | Target | Actual |
|---------|--------|--------|
| Code Coverage | >80% | TBD |
| Tests Passing | 100% | TBD |
| CPU Usage | <5% | TBD |
| Memory Leaks | 0 | TBD |
| Crashes (24h) | 0 | TBD |

---

**Última actualización:** 14 Diciembre 2025  
**Documentación completa:** ✅
