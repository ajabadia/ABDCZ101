# CZ-101 EMULATOR - ESPECIFICACIONES DSP

**Versión:** 1.0  
**Fecha:** 14 Diciembre 2025  
**Propósito:** Referencia rápida de valores numéricos para implementación

---

## 🎵 WAVEFORMS (Formas de Onda)

### Tabla de Waveforms Disponibles

| ID | Nombre | Descripción | Armónicos | Uso Típico |
|----|--------|-------------|-----------|------------|
| 0 | Sine | Onda sinusoidal pura | Solo fundamental | Pads suaves, bajos claros |
| 1 | Sawtooth | Rampa lineal ascendente | Todos (1/n) | Leads agresivos, bajos |
| 2 | Square | Pulso 50% duty cycle | Solo impares (4/πn) | Leads metálicos, bases huecas |
| 3 | Triangle | Rampa up/down | Solo impares (8/π²n²) | Leads suaves, strings |
| 4 | Pulse | Pulso 25% duty cycle | Impares modulados | Leads brillantes |
| 5 | DoubleSine | Dos ciclos de seno | 2× fundamental | Sub-basses con movimiento |
| 6 | HalfSine | Semicírculo (0 a π) | Fundamental + impares | Pads, campanas |
| 7 | ResonantSaw | Sawtooth + resonancia | Todos + picos | FM, sound design |
| 8 | ResonantTriangle | Triangle + resonancia | Impares + picos | Texturas complejas |
| 9 | Trapezoid | Rampa + meseta | Mixto | Leads únicos |

### Especificaciones Técnicas

**Tamaño de tabla:** 256 samples  
**Rango de valores:** -1.0 a +1.0  
**Interpolación:** Lineal  
**Resolución angular:** 360° / 256 = 1.40625° por sample

### Fórmulas Matemáticas

```cpp
// Sine Wave
for (int i = 0; i < 256; ++i) {
    float phase = (i / 256.0f) * TWO_PI;
    table[i] = std::sin(phase);
}

// Sawtooth Wave
for (int i = 0; i < 256; ++i) {
    table[i] = -1.0f + (2.0f * i / 255.0f);
}

// Square Wave
for (int i = 0; i < 256; ++i) {
    table[i] = (i < 128) ? 1.0f : -1.0f;
}

// Triangle Wave
for (int i = 0; i < 256; ++i) {
    if (i < 128) {
        table[i] = -1.0f + (2.0f * i / 127.0f);  // Subida
    } else {
        table[i] = 1.0f - (2.0f * (i - 128) / 127.0f);  // Bajada
    }
}
```

---

## 🌊 PHASE DISTORTION

### Algoritmo Core

**Propósito:** Modificar la fase antes del lookup de tabla para cambiar el timbre

**Fórmula:**
```cpp
float applyPhaseDistortion(float phase, float amount) {
    if (amount < 0.01f) return phase;
    
    // Mapeo de fase usando sine
    float piPhase = phase * PI;
    float k = amount * 0.9f;  // Limitar a 0.9 para evitar singularidades
    
    float sinPhase = std::sin(piPhase);
    float compressed = std::asin(juce::jlimit(-1.0f, 1.0f, k * sinPhase));
    float distortedPhase = (2.0f / PI) * compressed;
    
    // Remapear a rango 0-1
    return (distortedPhase + 1.0f) * 0.5f;
}
```

**Parámetros:**
- `phase`: Fase de entrada (0.0 - 1.0)
- `amount`: Cantidad de distorsión (0.0 - 1.0)
  - 0.0 = Sin distorsión (sine puro)
  - 0.3 = Distorsión suave
  - 0.7 = Distorsión agresiva
  - 1.0 = Máxima distorsión

**Efecto audible:**
- Cambia el contenido armónico dinámicamente
- Simula el comportamiento del CZ-101 original
- Crea timbres únicos de Phase Distortion

---

## 📈 ENVELOPES (Envolventes)

### ADSR Básico

**Rangos de tiempo:**

| Parámetro | Mínimo | Máximo | Típico |
|-----------|--------|--------|--------|
| Attack | 0 ms | 2000 ms | 10-500 ms |
| Decay | 0 ms | 3000 ms | 100-1000 ms |
| Sustain | 0% | 100% | 70-100% |
| Release | 0 ms | 3000 ms | 100-1500 ms |

### Curvas Exponenciales

**Attack Curve (más rápido al principio):**
```cpp
float attackCurve(float time, float attackTime) {
    return 1.0f - std::exp(-time / (attackTime * 0.3f));
}
```

**Decay Curve (lento al final):**
```cpp
float decayCurve(float time, float decayTime, float sustainLevel) {
    return sustainLevel + (1.0f - sustainLevel) * std::exp(-time / (decayTime * 0.5f));
}
```

**Release Curve (más rápido al principio):**
```cpp
float releaseCurve(float time, float releaseTime, float sustainLevel) {
    return sustainLevel * std::exp(-time / (releaseTime * 0.4f));
}
```

### Presets de Envelope Típicos

**Fast Attack Pad:**
```
Attack:  100 ms
Decay:   500 ms
Sustain: 100%
Release: 1000 ms
```

**Percussive Lead:**
```
Attack:  10 ms
Decay:   200 ms
Sustain: 80%
Release: 300 ms
```

**String Patch:**
```
Attack:  800 ms
Decay:   2000 ms
Sustain: 90%
Release: 1500 ms
```

**Bell/FM Sound:**
```
Attack:  50 ms
Decay:   3000 ms
Sustain: 20%
Release: 2000 ms
```

### DCW vs DCA Envelopes

**DCW (Digital Controlled Wave):**
- Controla la **cantidad de distorsión de fase**
- Rango: 0.0 (sine puro) a 1.0 (waveform completo)
- Afecta el **timbre**, no el volumen
- Típicamente más lento que DCA

**DCA (Digital Controlled Amplifier):**
- Controla el **nivel de amplitud**
- Rango: 0.0 (silencio) a 1.0 (máximo)
- Afecta el **volumen**, no el timbre
- Típicamente más rápido que DCW

### Pitch Envelope (IMPORTANTE)

**⚠️ DIFERENCIA CRÍTICA DEL CZ-101:**

En el CZ-101 real, el **Pitch Envelope modula la FRECUENCIA del oscilador**, no solo el timbre.

**Implementación correcta:**

```cpp
float Voice::processSample() {
    // Obtener modulación de pitch envelope (0.0 - 1.0)
    float pitchMod = pitchEnv.process();
    
    // Convertir a semitonos (rango típico: ±12 semitonos)
    float pitchSemitones = (pitchMod - 0.5f) * 24.0f;  // -12 a +12
    
    // Aplicar a frecuencia del oscilador
    float pitchRatio = std::pow(2.0f, pitchSemitones / 12.0f);
    float modulatedFreq = baseFrequency * pitchRatio;
    
    osc1.setFrequency(modulatedFreq);
    osc2.setFrequency(modulatedFreq * detuneRatio);
    
    // ... resto del procesamiento
}
```

**Rangos típicos:**
- Vibrato suave: ±2 semitonos
- Pitch sweep: ±12 semitonos
- Efectos extremos: ±24 semitonos

**Referencia:** `06_ADDITIONAL_NOTES.md` sección 3

---

## 🎹 MIDI SPECIFICATIONS

### Note Range

**Rango MIDI:** 0-127 (C-1 a G9)  
**Rango práctico:** 21-108 (A0 a C8)  
**Afinación:** A4 = 440 Hz (MIDI note 69)

### Frequency Calculation

```cpp
float midiNoteToFrequency(int midiNote) {
    return 440.0f * std::pow(2.0f, (midiNote - 69.0f) / 12.0f);
}
```

**Ejemplos:**
- MIDI 60 (C4) = 261.63 Hz
- MIDI 69 (A4) = 440.00 Hz
- MIDI 72 (C5) = 523.25 Hz

### MIDI CC Mappings

| CC# | Nombre | Rango | Mapeo | Descripción |
|-----|--------|-------|-------|-------------|
| 1 | Modulation Wheel | 0-127 | 0.0-1.0 | DCW Depth / Vibrato |
| 7 | Channel Volume | 0-127 | 0.0-1.0 | Master Volume |
| 11 | Expression | 0-127 | 0.0-1.0 | Volumen dinámico |
| 64 | Sustain Pedal | 0-63/64-127 | OFF/ON | Mantiene notas |
| 65 | Portamento | 0-63/64-127 | OFF/ON | Glide entre notas |
| 72 | Release Time | 0-127 | 0-3000ms | Tiempo de release |
| 74 | Brightness | 0-127 | 0.0-1.0 | DCW Amount |
| 91 | Reverb Level | 0-127 | 0.0-1.0 | Reverb Mix |
| 92 | Tremolo Rate | 0-127 | 0.1-20Hz | LFO 1 Rate |
| 93 | Chorus Rate | 0-127 | 0.5-5Hz | Chorus LFO |

### Pitch Bend

**Rango:** -8192 a +8191 (14-bit)  
**Rango típico:** ±2 semitonos (configurable)  
**Rango máximo:** ±24 semitonos

```cpp
float pitchBendToSemitones(int pitchBendValue, int range) {
    // pitchBendValue: -8192 a +8191
    // range: semitonos (típicamente 2)
    float normalized = pitchBendValue / 8192.0f;  // -1.0 a +1.0
    return normalized * range;
}
```

### Velocity Sensitivity

**Rango MIDI:** 1-127 (0 = note off)  
**Normalizado:** 0.0078 - 1.0

```cpp
float velocityToGain(int velocity) {
    return velocity / 127.0f;
}
```

**Curvas de respuesta:**
- **Linear:** gain = velocity / 127
- **Exponential:** gain = (velocity / 127)²
- **Logarithmic:** gain = log(1 + velocity) / log(128)

---

## 🔊 AUDIO PROCESSING

### Sample Rates Soportados

| Sample Rate | Uso | Calidad |
|-------------|-----|---------|
| 44.1 kHz | CD quality, estándar | Buena |
| 48 kHz | Video, DAWs modernos | Buena |
| 88.2 kHz | Hi-res (2× CD) | Excelente |
| 96 kHz | Hi-res estándar | Excelente |
| 192 kHz | Hi-res máximo | Overkill |

**Recomendado:** 44.1 kHz o 48 kHz para balance calidad/performance

### Buffer Sizes

| Buffer Size | Latencia @ 44.1kHz | Latencia @ 48kHz | Uso |
|-------------|-------------------|------------------|-----|
| 64 samples | 1.45 ms | 1.33 ms | Performance en vivo |
| 128 samples | 2.90 ms | 2.67 ms | Grabación |
| 256 samples | 5.80 ms | 5.33 ms | Producción |
| 512 samples | 11.61 ms | 10.67 ms | Mixing |
| 1024 samples | 23.22 ms | 21.33 ms | Mastering |

**Recomendado:** 128-256 samples para balance latencia/estabilidad

### Bit Depth

**Procesamiento interno:** 32-bit float  
**Salida:** 24-bit o 32-bit float  
**Simulación vintage:** 14-bit quantization (opcional)

---

## 🎚️ ALIASING Y AUTENTICIDAD

### Aliasing Controlado (14-bit Quantization)

**Propósito:** Emular el DAC de 14-bits del CZ-101 original

```cpp
float applyAliasing(float sample, float amount) {
    if (amount < 0.01f) return sample;
    
    // Calcular bits efectivos (8-14)
    int bits = static_cast<int>(14.0f * (1.0f - amount) + 8.0f * amount);
    bits = juce::jlimit(8, 14, bits);
    
    int levels = (1 << bits);  // 2^bits
    
    // Quantizar
    int quantized = static_cast<int>(sample * (levels * 0.5f) + (levels * 0.5f));
    quantized = juce::jlimit(0, levels - 1, quantized);
    
    // Dequantizar
    return (quantized / static_cast<float>(levels)) * 2.0f - 1.0f;
}
```

**Parámetros:**
- `amount`: 0.0 = sin aliasing (14-bit), 1.0 = máximo aliasing (8-bit)

### Jitter Analógico

**Propósito:** Simular inestabilidad de osciladores analógicos

```cpp
float applyJitter(float frequency, float jitterAmount) {
    // Jitter típico: ±0.1% de la frecuencia
    float jitterRange = frequency * 0.001f * jitterAmount;
    float randomJitter = (rand() / (float)RAND_MAX * 2.0f - 1.0f) * jitterRange;
    return frequency + randomJitter;
}
```

### Saturación de Transistor

**Propósito:** Soft clipping característico de circuitos analógicos

```cpp
float applySaturation(float sample, float drive) {
    // Curva tanh() para saturación suave
    return std::tanh(sample * drive) / std::tanh(drive);
}
```

**Valores típicos:**
- `drive = 1.0`: Sin saturación
- `drive = 2.0`: Suave (recomendado para CZ-101)
- `drive = 5.0`: Moderada
- `drive = 10.0`: Agresiva

### Ruido Rosa Vintage

**Propósito:** Simular ruido de circuito analógico (~-60dB)

```cpp
class PinkNoise {
public:
    float getNextSample() {
        // Paul Kellett's algorithm
        float white = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
        m_pink = 0.99765f * m_pink + white * 0.0009005f;
        return m_pink;
    }
    
private:
    float m_pink = 0.0f;
};
```

**Niveles recomendados:**
- Silent: 0.00000 (-∞ dB)
- Very Subtle: 0.00003 (-90 dB)
- Warm (CZ-101): 0.00035 (-69 dB)
- Obvious: 0.001 (-60 dB)

---

## 🎛️ EFECTOS

### Reverb

**Tipo:** Convolución simple o Freeverb  
**Parámetros:**
- Room Size: 0.0 - 1.0
- Damping: 0.0 - 1.0
- Wet/Dry Mix: 0.0 - 1.0

**Valores típicos CZ-101:**
```
Room Size: 0.7
Damping: 0.5
Mix: 0.3 (30% wet)
```

### Chorus

**Tipo:** Delay line + LFO  
**Parámetros:**
- Rate: 0.5 - 5.0 Hz
- Depth: 0.0 - 10.0 ms
- Mix: 0.0 - 1.0

**Valores típicos:**
```
Rate: 1.5 Hz
Depth: 3.0 ms
Mix: 0.4 (40% wet)
```

### Delay

**Tipo:** Tape simulation  
**Parámetros:**
- Time: 1 - 2000 ms
- Feedback: 0.0 - 0.95
- Mix: 0.0 - 1.0

**Valores típicos:**
```
Time: 400 ms (quarter note @ 120 BPM)
Feedback: 0.4
Mix: 0.25 (25% wet)
```

---

## 🔢 CONSTANTES IMPORTANTES

```cpp
namespace Constants {
    // Audio
    constexpr double DEFAULT_SAMPLE_RATE = 44100.0;
    constexpr int DEFAULT_BUFFER_SIZE = 512;
    constexpr int MAX_VOICES = 8;
    
    // MIDI
    constexpr int MIDI_NOTE_MIN = 0;
    constexpr int MIDI_NOTE_MAX = 127;
    constexpr float A4_FREQUENCY = 440.0f;
    constexpr int A4_MIDI_NOTE = 69;
    
    // Oscillator
    constexpr int WAVETABLE_SIZE = 256;
    constexpr float TWO_PI = 6.28318530718f;
    constexpr float PI = 3.14159265359f;
    
    // Envelopes
    constexpr float MIN_ENVELOPE_TIME_MS = 0.0f;
    constexpr float MAX_ENVELOPE_TIME_MS = 3000.0f;
    
    // Effects
    constexpr float MAX_DELAY_TIME_MS = 2000.0f;
    constexpr float MAX_LFO_RATE_HZ = 20.0f;
    constexpr float MIN_LFO_RATE_HZ = 0.1f;
}
```

---

## 📚 REFERENCIAS

**Documentos fuente:**
- `CZ101-CODIGO-REAL-ESPECIFICACIONES.md` - Código de referencia
- `CZ101-PRESETS-VALIDACION-AUDIO.md` - Presets con valores
- `CZ101-FINAL-CHECKLIST.md` - Checklist completo

**Usar este documento como:**
- ✅ Referencia rápida durante implementación
- ✅ Validación de valores numéricos
- ✅ Guía para tests unitarios

---

**Última actualización:** 14 Diciembre 2025  
**Próximo documento:** `04_UI_DESIGN.md`
