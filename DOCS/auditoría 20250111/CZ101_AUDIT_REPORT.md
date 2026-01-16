# AUDITORÍA DE CÓDIGO - CZ101 SYNTHESIZER
## Informe Técnico Detallado

**Fecha:** 2025-11-02  
**Versión del Proyecto:** Build 41 (2026-01-11 20:43:47)  
**Archivos Auditados:** 35 archivos fuente  

---

## RESUMEN EJECUTIVO

El proyecto CZ101 Synthesizer es una emulación del sintetizador Casio CZ-101/CZ-5000 implementada en JUCE/C++. Se ha identificado un total de **42 problemas** categorizados como sigue:

- 🔴 **Errores Críticos:** 7 (pueden causar crashes o comportamiento indefinido)
- 🟡 **Errores de Lógica:** 12 (afectan el funcionamiento del sintetizador)
- 🟠 **Malas Prácticas:** 15 (problemas de mantenibilidad y rendimiento)
- 🔵 **Mejoras de Diseño:** 8 (optimizaciones y mejoras de arquitectura)

---

## 1. ERRORES CRÍTICOS (ALTA PRIORIDAD)

### 1.1. Race Condition en Command FIFO
**Archivo:** `PluginProcessor.cpp` (líneas 586-620)  
**Gravedad:** 🔴 CRÍTICO  
**Descripción:** 
```cpp
void CZ101AudioProcessor::scheduleEnvelopeUpdate(const EnvelopeUpdateCommand& cmd)
{
    int start1, size1, start2, size2;
    commandFifo.prepareToWrite(1, start1, size1, start2, size2);
    if (size1 > 0) commandBuffer[start1] = cmd;
    else if (size2 > 0) commandBuffer[start2] = cmd;
    commandFifo.finishedWrite(size1 + size2);
}
```
**Problema:** No hay verificación de espacio disponible antes de escribir. Si el FIFO está lleno, se puede perder comandos o sobrescribir datos.  
**Solución:**
```cpp
void CZ101AudioProcessor::scheduleEnvelopeUpdate(const EnvelopeUpdateCommand& cmd)
{
    int start1, size1, start2, size2;
    int space = commandFifo.getFreeSpace();
    if (space < 1) {
        juce::Logger::writeToLog("Warning: Command FIFO full, dropping envelope update");
        return;
    }
    
    commandFifo.prepareToWrite(1, start1, size1, start2, size2);
    if (size1 > 0) commandBuffer[start1] = cmd;
    else if (size2 > 0) commandBuffer[start2] = cmd;
    commandFifo.finishedWrite(size1 + size2);
}
```

### 1.2. Buffer Overflow en SysExManager
**Archivo:** `SysExManager.cpp` (línea 3987)  
**Gravedad:** 🔴 CRÍTICO  
**Descripción:** 
```cpp
static uint8_t decodeNibblePair(const uint8_t* payload, int& offset, int maxSize) {
    if (offset + 1 >= maxSize) {
        juce::Logger::writeToLog("⚠️ SysEx: Offset overflow at offset=" + juce::String(offset));
        return 0;
    }
    uint8_t lowNibble = payload[offset++] & 0x0F;
    uint8_t highNibble = payload[offset++] & 0x0F;
    return result;
}
```
**Problema:** La verificación de límites es incorrecta. Debe ser `offset + 2 > maxSize` porque se leen 2 bytes.  
**Solución:**
```cpp
static uint8_t decodeNibblePair(const uint8_t* payload, int& offset, int maxSize) {
    if (offset + 2 > maxSize) {
        juce::Logger::writeToLog("⚠️ SysEx: Offset overflow at offset=" + juce::String(offset));
        offset = maxSize; // Prevenir lecturas futuras inválidas
        return 0;
    }
    uint8_t lowNibble = payload[offset++] & 0x0F;
    uint8_t highNibble = payload[offset++] & 0x0F;
    uint8_t result = (highNibble << 4) | lowNibble;
    return result;
}
```

### 1.3. División por Cero en Rate Mapping
**Archivo:** `ADSRtoStage.h` (línea 2423)  
**Gravedad:** 🔴 CRÍTICO  
**Descripción:** 
```cpp
auto msToRateCoeff = [sampleRate](float ms) -> float {
    if (ms < 1.0f) ms = 1.0f;
    if (ms > 8000.0f) ms = 8000.0f;
    float sec = ms / 1000.0f;
    float k = 5.5f / (sec * static_cast<float>(sampleRate));
    float coeff = std::exp(-k);
    return std::clamp(coeff, 0.001f, 0.99f);
};
```
**Problema:** Si `sampleRate` es 0 (no inicializado), hay división por cero.  
**Solución:**
```cpp
auto msToRateCoeff = [sampleRate](float ms) -> float {
    if (ms < 1.0f) ms = 1.0f;
    if (ms > 8000.0f) ms = 8000.0f;
    float sec = ms / 1000.0f;
    double sr = (sampleRate > 0.0) ? sampleRate : 44100.0; // Fallback
    float k = 5.5f / (sec * static_cast<float>(sr));
    float coeff = std::exp(-k);
    return std::clamp(coeff, 0.001f, 0.99f);
};
```

### 1.4. Gestión Inadecuada de Memoria en StandaloneApp
**Archivo:** `StandaloneApp.cpp` (líneas 4642, 4734)  
**Gravedad:** 🔴 CRÍTICO  
**Descripción:** 
```cpp
// Línea 4642: Doble inicialización del deviceManager
mainWindow.reset(new MainWindow(getApplicationName(), new CZ101AudioProcessor(), settings.get()));

// Dentro de MainWindow constructor (línea 4734):
auto err = deviceManager.initialiseWithDefaultDevices(0, 2);
```
**Problema:** El AudioDeviceManager se inicializa dos veces, lo que puede causar problemas de memoria y recursos de audio.  
**Solución:** Eliminar la inicialización doble:
```cpp
// En MainWindow constructor, cambiar:
MainWindow(const juce::String& name, juce::AudioProcessor* createdProcessor, 
           juce::PropertiesFile* settings)
    : DocumentWindow(name, /* ... */),
      m_processor(createdProcessor)
{
    // ... código existente ...
    
    // Quitar esta línea (ya se inicializa después):
    // auto err = deviceManager.initialiseWithDefaultDevices(0, 2);
    
    // Mantener solo la inicialización condicional desde settings:
    if (settings != nullptr)
    {
       auto xml = settings->getXmlValue("audioDeviceState");
       if (xml != nullptr)
           deviceManager.initialise(0, 2, xml.get(), true);
       else
           deviceManager.initialiseWithDefaultDevices(0, 2); // Solo si no hay settings
    }
}
```

### 1.5. Acceso a Datos No Sincronizados en VoiceManager
**Archivo:** `VoiceManager.h/cpp` (getters)  
**Gravedad:** 🔴 CRÍTICO  
**Descripción:** 
```cpp
void VoiceManager::getDCWStage(int index, float& rate, float& level) const noexcept { 
    voices[0].getDCWStage(index, rate, level); 
}
```
**Problema:** Se accede solo a `voices[0]` para leer parámetros, pero no hay garantía de que esta voz represente el estado actual. Además, si hay operaciones de escritura concurrentes, puede haber race conditions.  
**Solución:**
```cpp
// Opción 1: Usar una voz de referencia dedicada
class VoiceManager {
private:
    Voice referenceVoice; // Voz para lectura de parámetros
    // ...
};

void VoiceManager::getDCWStage(int index, float& rate, float& level) const noexcept { 
    referenceVoice.getDCWStage(index, rate, level); 
}

// En setSampleRate, sincronizar:
void VoiceManager::setSampleRate(double sampleRate) noexcept {
    for (auto& voice : voices)
        voice.setSampleRate(sampleRate);
    referenceVoice.setSampleRate(sampleRate); // Mantener sincronizada
}
```

### 1.6. Desbordamiento de Buffer Circular en Chorus
**Archivo:** `Chorus.cpp` (líneas 1753-1757)  
**Gravedad:** 🔴 CRÍTICO  
**Descripción:** 
```cpp
float readPosL = static_cast<float>(writeIndex) - delayL;
if (readPosL < 0.0f) readPosL += bufferSize;

float readPosR = static_cast<float>(writeIndex) - delayR;
if (readPosR < 0.0f) readPosR += bufferSize;
```
**Problema:** Si `delayL` o `delayR` son mayores que `bufferSize`, el ajuste no funcionará correctamente.  
**Solución:**
```cpp
float readPosL = static_cast<float>(writeIndex) - delayL;
readPosL = std::fmod(readPosL, static_cast<float>(bufferSize));
if (readPosL < 0.0f) readPosL += static_cast<float>(bufferSize);

float readPosR = static_cast<float>(writeIndex) - delayR;
readPosR = std::fmod(readPosR, static_cast<float>(bufferSize));
if (readPosR < 0.0f) readPosR += static_cast<float>(bufferSize);
```

### 1.7. Inicialización Incorrecta de PresetManager
**Archivo:** `PluginProcessor.cpp` (líneas 466-467)  
**Gravedad:** 🔴 CRÍTICO  
**Descripción:** 
```cpp
presetManager.loadPreset(0);
```
**Problema:** Se carga el preset 0 antes de que el VoiceManager tenga la sample rate configurada, lo que puede causar problemas de inicialización.  
**Solución:**
```cpp
void CZ101AudioProcessor::prepareToPlay(double sr, int samplesPerBlock) 
{
    // Primero configurar sample rate
    voiceManager.setSampleRate(sr);
    // ... resto del código ...
    
    // Después cargar el preset
    presetManager.loadPreset(0);
}
```

---

## 2. ERRORES DE LÓGICA (MEDIA PRIORIDAD)

### 2.1. Line Select Logic Incorrecta
**Archivo:** `PluginProcessor.cpp` (líneas 526-534)  
**Gravedad:** 🟡 ALTO  
**Descripción:** 
```cpp
// 1. Line Select Muting Logic
if (lineSel == 0) l2 = 0.0f; // Line 1 Only
if (lineSel == 1) l1 = 0.0f; // Line 2 Only

// 2. Line Select 1+1' Logic (Copy Line 1 to Line 2)
if (lineSel == 2) { 
    w2_1 = w1_1;
    w2_2 = w1_2;
}
```
**Problema:** La lógica de copia en modo 1+1' no copia los niveles, solo las formas de onda. Además, no hay manejo del modo 1+2 (lineSel == 3).  
**Solución:**
```cpp
// Line Select Logic (FIXED)
switch (lineSel) {
    case 0: // Line 1 Only
        l2 = 0.0f;
        break;
        
    case 1: // Line 2 Only
        l1 = 0.0f;
        break;
        
    case 2: // Line 1 + 1' (Copy Line 1 to Line 2 with slight detune)
        w2_1 = w1_1;
        w2_2 = w1_2;
        l2 = l1; // Copiar también el nivel
        // Aplicar detune sutil si no está configurado
        if (parameters.osc2Detune && parameters.osc2Detune->get() == 0.0f) {
            voiceManager.setOsc2Detune(2.0f); // 2 cents de detune
        }
        break;
        
    case 3: // Line 1 + 2 (Independent lines)
        // No modificación necesaria, usar valores originales
        break;
}
```

### 2.2. Vibrato Depth Mapping Incorrecto
**Archivo:** `Voice.cpp` (líneas 1108-1113)  
**Gravedad:** 🟡 ALTO  
**Descripción:** 
```cpp
// === LFO VIBRATO ===
float vibratoMod = 1.0f;
if (vibratoDepth > 0.001f) {
    float currentLFO = lfoModule.getNextValue();
    float lfoSemitones = currentLFO * vibratoDepth;
    vibratoMod = std::pow(2.0f, lfoSemitones / 12.0f);
}
```
**Problema:** El LFO genera valores de -1 a 1, pero el cálculo asume 0 a 1. Esto causa vibrato asimétrico.  
**Solución:**
```cpp
// === LFO VIBRATO (FIXED) ===
float vibratoMod = 1.0f;
if (vibratoDepth > 0.001f) {
    float currentLFO = lfoModule.getNextValue();
    // Mapear de [-1, 1] a [0, 1] para vibrato unipolar
    float lfoSemitones = ((currentLFO + 1.0f) * 0.5f) * vibratoDepth;
    vibratoMod = std::pow(2.0f, lfoSemitones / 12.0f);
}
```

### 2.3. Pitch Envelope Range Incorrecto
**Archivo:** `Voice.cpp` (líneas 1091-1093)  
**Gravedad:** 🟡 ALTO  
**Descripción:** 
```cpp
// Pitch envelope: 0.0 = -1 octava, 0.5 = unison, 1.0 = +1 octava
float semitones = (pitchEnvVal - 0.5f) * 100.0f;     // ±50 semitones
```
**Problema:** El rango de ±50 semitones (±4 octavas) es excesivo y no corresponde al hardware original CZ-101 (±1 octava típicamente).  
**Solución:**
```cpp
// Pitch envelope: Rango auténtico CZ-101 (±1 octava = ±12 semitones)
float semitones = (pitchEnvVal - 0.5f) * 24.0f;     // ±12 semitones
```

### 2.4. Envelope Rate Mapping Inconsistente
**Archivo:** `MultiStageEnv.cpp` (líneas 2632-2644)  
**Gravedad:** 🟡 ALTO  
**Descripción:** 
```cpp
float MultiStageEnvelope::rateToSeconds(float rate) const noexcept
{
    // CZ-101 Rate approximation
    // Rate 0-99 mapped to ms
    // Using exponential curve
    float r = 1.0f - rate;
    return 0.001f + (std::pow(r, 4.0f) * 30.0f);
}
```
**Problema:** Esta función no coincide con el mapeo usado en `ADSRtoStage.h`. Hay inconsistencia en cómo se mapean los rates.  
**Solución:** Unificar el mapeo:
```cpp
// En MultiStageEnv.h, agregar:
static float rateToSeconds(float rate, double sampleRate = 44100.0) noexcept
{
    // Usar el mismo algoritmo que ADSRtoStage
    rate = std::clamp(rate, 0.0f, 1.0f);
    float r = 1.0f - rate;
    return 0.001f + (std::pow(r, 4.0f) * 30.0f);
}
```

### 2.5. MIDI CC Mapping Incompleto
**Archivo:** `MIDIProcessor.cpp` (líneas 3854-3890)  
**Gravedad:** 🟡 ALTO  
**Descripción:** Solo se manejan CC 1, 5, 6 y 65. Faltan muchos controles estándar.  
**Solución:** Implementar mapeo completo:
```cpp
void MIDIProcessor::handleControlChange(int cc, int value) noexcept
{
    float normValue = value / 127.0f;
    
    switch (cc)
    {
        case 1: // Mod Wheel (Vibrato Depth)
            voiceManager.setVibratoDepth(normValue * 1.0f);
            break;
            
        case 5: // Portamento Time
            portamentoTime = normValue * 2.0f;
            if (portamentoEnabled)
                voiceManager.setGlideTime(portamentoTime);
            break;
            
        case 6: // Data Entry (Master Tune)
            {
                float tune = (value - 64) / 64.0f;
                voiceManager.setMasterTune(tune);
            }
            break;
            
        case 64: // Sustain Pedal
            if (value >= 64)
                ; // Sustain ON - implementar
            else
n                ; // Sustain OFF - implementar
            break;
            
        case 65: // Portamento On/Off
            portamentoEnabled = (value >= 64);
            voiceManager.setGlideTime(portamentoEnabled ? portamentoTime : 0.0f);
            break;
            
        case 123: // All Notes Off
            voiceManager.allNotesOff();
            break;
            
        default:
            // Log unhandled CCs for debugging
            juce::Logger::writeToLog("Unhandled CC: " + juce::String(cc) + " = " + juce::String(value));
            break;
    }
}
```

### 2.6. Voice Stealing Algorithm Débil
**Archivo:** `VoiceManager.cpp` (líneas 1519-1526)  
**Gravedad:** 🟡 ALTO  
**Descripción:** 
```cpp
int VoiceManager::findVoiceToSteal() const noexcept
{
    // Simple: steal first active voice (oldest)
    for (size_t i = 0; i < voices.size(); ++i)
        if (voices[i].isActive())
            return static_cast<int>(i);
    return 0;
}
```
**Problema:** No considera la envolvente ni la amplitud de la voz. Puede robar la voz más audible.  
**Solución:**
```cpp
int VoiceManager::findVoiceToSteal() const noexcept
{
    // Prioridad: 
    // 1. Voces en fase de release (menor importancia)
    // 2. Voces con menor amplitud
    // 3. La voz más antigua
    
    int oldestIndex = -1;
    float lowestAmplitude = 1.0f;
    
    for (size_t i = 0; i < voices.size(); ++i)
    {
        if (voices[i].isActive())
        {
            // Obtener amplitud aproximada (necesitar método en Voice)
            float amp = voices[i].getCurrentAmplitude(); // Necesita implementar
            
            // Priorizar voces en release
            bool isInRelease = voices[i].isInReleasePhase(); // Necesita implementar
            
            if (isInRelease || amp < lowestAmplitude)
            {
                lowestAmplitude = amp;
                oldestIndex = static_cast<int>(i);
            }
        }
    }
    
    return (oldestIndex >= 0) ? oldestIndex : 0;
}
```

### 2.7. SysEx Validation Insuficiente
**Archivo:** `SysExManager.cpp` (líneas 4030-4093)  
**Gravedad:** 🟡 ALTO  
**Descripción:** La validación del header SysEx es básica y no verifica el checksum.  
**Solución:** Implementar validación completa:
```cpp
void SysExManager::handleSysEx(
    const void* data,
    int size,
    const juce::String& patchName)
{
    const auto bytes = static_cast<const uint8_t*>(data);
    
    // Validación robusta
    if (size < 10) {
        juce::Logger::writeToLog("❌ SysEx too small: " + juce::String(size) + " bytes");
        return;
    }
    
    // Verificar estructura completa
    if (bytes[0] != SYSEX_START || bytes[size-1] != SYSEX_END) {
        juce::Logger::writeToLog("❌ Invalid SysEx delimiters");
        return;
    }
    
    // Verificar ID de fabricante
    if (bytes[1] != MANUF_ID_1 || bytes[2] != MANUF_ID_2 || bytes[3] != MANUF_ID_3) {
        juce::Logger::writeToLog("❌ Not a Casio SysEx");
        return;
    }
    
    // Verificar device ID
    uint8_t deviceId = bytes[4];
    if ((deviceId & 0xF0) != DEVICE_ID_BASE) {
        juce::Logger::writeToLog("❌ Invalid device ID: " + juce::String::toHexString(deviceId));
        return;
    }
    
    // Verificar tamaño de payload (debe ser 256 bytes = 512 nibbles)
    int payloadSize = size - 8; // Excluir F0, ID, device, function, program, F7
    if (payloadSize != 512) {
        juce::Logger::writeToLog("⚠️ Unexpected payload size: " + juce::String(payloadSize) + 
                                " (expected 512 nibbles)");
        // Continuar pero con advertencia
    }
    
    // Verificar checksum si está presente
    // ... implementar verificación de checksum
}
```

### 2.8. Preset Loading Race Condition
**Archivo:** `PresetManager.cpp` (líneas 5096-5117)  
**Gravedad:** 🟡 ALTO  
**Descripción:** Si se carga un preset mientras el audio se está procesando, puede haber inconsistencias.  
**Solución:** Proteger con mutex:
```cpp
class PresetManager {
private:
    mutable juce::CriticalSection presetLock;
    // ...
};

void PresetManager::loadPreset(int index)
{
    juce::ScopedLock lock(presetLock);
    
    if (index >= 0 && index < static_cast<int>(presets.size()))
    {
        currentPreset = presets[index];
        applyPresetToProcessor();
        
        if (voiceManager)
        {
            applyEnvelopeToVoice(currentPreset.pitchEnv, 0);
            applyEnvelopeToVoice(currentPreset.dcwEnv, 1);
            applyEnvelopeToVoice(currentPreset.dcaEnv, 2);
        }
    }
}
```

### 2.9. Waveform Display Buffer Overflow
**Archivo:** `WaveformDisplay.cpp` (líneas 7440-7442)  
**Gravedad:** 🟡 ALTO  
**Descripción:** 
```cpp
void WaveformDisplay::pushBuffer(const juce::AudioBuffer<float>& buffer)
{
    auto* channelData = buffer.getReadPointer(0);
    int numSamples = buffer.getNumSamples();
    
    for (int i = 0; i < numSamples; ++i)
    {
        waveformData[writePos] = channelData[i];
        writePos = (writePos + 1) % waveformData.size();
    }
}
```
**Problema:** No hay verificación de límites si numSamples > waveformData.size()  
**Solución:**
```cpp
void WaveformDisplay::pushBuffer(const juce::AudioBuffer<float>& buffer)
{
    auto* channelData = buffer.getReadPointer(0);
    int numSamples = buffer.getNumSamples();
    int bufferSize = static_cast<int>(waveformData.size());
    
    // Limitar samples a copiar
    int samplesToCopy = juce::jmin(numSamples, bufferSize);
    
    for (int i = 0; i < samplesToCopy; ++i)
    {
        waveformData[writePos] = channelData[i];
        writePos = (writePos + 1) % bufferSize;
    }
}
```

### 2.10. Chorus LFO Phase Wrapping Incorrecto
**Archivo:** `Chorus.cpp` (líneas 1738-1739)  
**Gravedad:** 🟡 ALTO  
**Descripción:** 
```cpp
lfoPhase += lfoIncrement;
if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
```
**Problema:** No maneja correctamente valores negativos.  
**Solución:**
```cpp
lfoPhase += lfoIncrement;
while (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
while (lfoPhase < 0.0f) lfoPhase += 1.0f;
```

### 2.11. Phase Distortion Boundary Check Ausente
**Archivo:** `PhaseDistOsc.cpp` (línea 3225-3227)  
**Gravedad:** 🟡 ALTO  
**Descripción:** 
```cpp
if (distortedPhase >= 1.0f) distortedPhase -= 1.0f;
if (distortedPhase < 0.0f) distortedPhase += 1.0f;
```
**Problema:** Si `distortedPhase` es muy grande (ej: 2.5), solo se resta 1.0f una vez.  
**Solución:**
```cpp
distortedPhase = std::fmod(distortedPhase, 1.0f);
if (distortedPhase < 0.0f) distortedPhase += 1.0f;
```

### 2.12. MIDI Output Selector No Implementado
**Archivo:** `PluginEditor.cpp` (líneas 216-223)  
**Gravedad:** 🟡 ALTO  
**Descripción:** El selector de salida MIDI no tiene implementación.  
**Solución:**
```cpp
void CZ101AudioProcessorEditor::refreshMidiOutputs()
{
    midiOutputSelector.clear();
    midiOutputSelector.addItem("None", 1);
    
    auto devices = juce::MidiOutput::getAvailableDevices();
    for (int i = 0; i < devices.size(); ++i)
        midiOutputSelector.addItem(devices[i].name, i + 2);
    
    // Conectar callback
    midiOutputSelector.onChange = [this]() {
        int selected = midiOutputSelector.getSelectedItemIndex();
        if (selected == 0) {
            activeMidiOutput.reset();
        } else {
            auto devices = juce::MidiOutput::getAvailableDevices();
            if (selected - 1 < devices.size()) {
                activeMidiOutput = juce::MidiOutput::openDevice(devices[selected - 1].identifier);
            }
        }
    };
}
```

---

## 3. MALAS PRÁCTICAS (MEDIA PRIORIDAD)

### 3.1. Hardcoded Magic Numbers
**Ubicación:** Múltiples archivos  
**Gravedad:** 🟠 MEDIO  
**Descripción:** Hay números mágicos sin documentación:
- `1024` (tamaño de buffer) en varios lugares
- `256` (tablas de onda) en WaveTable
- `8` (voces máximas) en VoiceManager

**Solución:** Crear constantes:
```cpp
// En un archivo de configuración (Config.h)
namespace CZ101 {
namespace Config {
    constexpr int MAX_VOICES = 8;
    constexpr int WAVETABLE_SIZE = 256;
    constexpr int VISUALIZATION_BUFFER_SIZE = 1024;
    constexpr int COMMAND_QUEUE_SIZE = 1024;
    constexpr float DEFAULT_SAMPLE_RATE = 44100.0;
}
}
```

### 3.2. Repetición de Código en VoiceManager
**Archivo:** `VoiceManager.h/cpp`  
**Gravedad:** 🟠 MEDIO  
**Descripción:** Los métodos de VoiceManager tienen patrones repetitivos:
```cpp
void VoiceManager::setOsc1Level(float level) noexcept {
    for (auto& voice : voices) voice.setOsc1Level(level);
}
// ... se repite para cada parámetro
```

**Solución:** Usar macros o templates:
```cpp
// En VoiceManager.h
#define VOICE_FORWARD_METHOD(methodName, paramType) \
    void methodName(paramType param) noexcept { \
        for (auto& voice : voices) voice.methodName(param); \
    }

class VoiceManager {
    // ...
    VOICE_FORWARD_METHOD(setOsc1Level, float)
    VOICE_FORWARD_METHOD(setOsc2Level, float)
    // ...
};
```

### 3.3. Gestión de Archivos Sin Verificación
**Archivo:** `PresetManager.cpp` (líneas 5688-5717)  
**Gravedad:** 🟠 MEDIO  
**Descripción:** No se verifica si los archivos se pueden escribir/leer.  
**Solución:**
```cpp
void PresetManager::saveBank(const juce::File& file)
{
    // Verificar que el archivo es escribible
    if (!file.getParentDirectory().hasWriteAccess()) {
        juce::Logger::writeToLog("Cannot save bank: No write access to " + file.getFullPathName());
        return;
    }
    
    juce::Array<juce::var> bankArray;
    // ... resto del código ...
    
    // Guardar con verificación
    if (!file.replaceWithText(jsonString)) {
        juce::Logger::writeToLog("Failed to save bank to " + file.getFullPathName());
    }
}
```

### 3.4. Logging Inconsistente
**Ubicación:** Múltiples archivos  
**Gravedad:** 🟠 MEDIO  
**Descripción:** Algunos logs usan `juce::Logger`, otros usan `std::cout`.  
**Solución:** Unificar logging:
```cpp
// Logger.h
namespace CZ101 {
namespace Utils {
    class Logger {
    public:
        static void log(const juce::String& message, juce::Logger::LogLevel level = juce::Logger::LogLevel::info) {
            juce::Logger::writeToLog(message);
            
            // En debug, también imprimir a consola
            #ifdef DEBUG
            std::cout << message.toStdString() << std::endl;
            #endif
        }
        
        static void error(const juce::String& message) {
            log("ERROR: " + message, juce::Logger::LogLevel::error);
        }
        
        static void warning(const juce::String& message) {
            log("WARNING: " + message, juce::Logger::LogLevel::warning);
        }
    };
}
}
```

### 3.5. Falta de Validación de Parámetros
**Ubicación:** Múltiples archivos  
**Gravedad:** 🟠 MEDIO  
**Descripción:** Muchos métodos no validan rangos de entrada.  
**Solución:** Agregar validación:
```cpp
// En Voice.h
void Voice::setOsc2Detune(float cents) noexcept {
    cents = std::clamp(cents, -100.0f, 100.0f); // Limitar a rango válido
    osc2Detune = cents;
    currentDetuneFactor = std::pow(2.0f, cents / 1200.0f);
}
```

### 3.6. Memory Layout Ineficiente
**Archivo:** `Voice.h`  
**Gravedad:** 🟠 MEDIO  
**Descripción:** Los miembros de Voice no están ordenados por tamaño, causando padding innecesario.  
**Solución:** Reordenar miembros:
```cpp
class Voice {
    // Agrupar por tamaño (de mayor a menor)
    double sampleRate = 44100.0;    // 8 bytes
    
    // Envelopes (objetos grandes)
    DSP::MultiStageEnvelope dcwEnvelope;
    DSP::MultiStageEnvelope dcaEnvelope;
    DSP::MultiStageEnvelope pitchEnvelope;
    DSP::LFO lfoModule;
    
    // Arrays
    std::array<float, 8> rates, levels; // Si es necesario
    
    // Miembros de 4 bytes
    float osc1Level = 0.5f;
    float osc2Level = 0.5f;
    float osc2Detune = 0.0f;
    float baseFrequency = 440.0f;
    float currentDetuneFactor = 1.0f;
    float glideTime = 0.0f;
    float currentFrequency = 440.0f;
    float targetFrequency = 440.0f;
    float vibratoDepth = 0.0f;
    float pitchBendFactor = 1.0f;
    float masterTuneFactor = 1.0f;
    float currentVelocity = 1.0f;
    
    // Miembros de 2 bytes o menos
    int currentNote = -1;
    
    // Bools (agrupados)
    bool isHardSyncEnabled = false;
    bool isRingModEnabled = false;
    
    // Structs anidadas al final
    struct ADSRParams {
        float attackMs = 10.0f;
        float decayMs = 200.0f;
        float sustainLevel = 0.5f;
        float releaseMs = 100.0f;
    };
    
    ADSRParams dcwADSR;
    ADSRParams dcaADSR;
    ADSRParams pitchADSR;
};
```

### 3.7. Falta de Namespace Consistente
**Ubicación:** Headers  
**Gravedad:** 🟠 MEDIO  
**Descripción:** Algunos headers no están correctamente anidados en namespaces.  
**Solución:** Asegurar que todos los headers tengan:
```cpp
namespace CZ101 {
namespace [SubNamespace] {
    // Contenido
}
}
```

### 3.8. Includes Innecesarios
**Ubicación:** Múltiples archivos  
**Gravedad:** 🟠 MEDIO  
**Descripción:** Inclusión de `<JuceHeader.h>` cuando se necesitan solo módulos específicos.  
**Solución:** Usar includes específicos:
```cpp
// En lugar de #include <JuceHeader.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
```

### 3.9. Falta de Tests Unitarios
**Ubicación:** General  
**Gravedad:** 🟠 MEDIO  
**Descripción:** Solo hay tests básicos de SysEx.  
**Solución:** Implementar suite de tests completa:
- Tests de envelopes: Verificar que los rates y niveles se aplican correctamente
- Tests de osciladores: Verificar generación de formas de onda
- Tests de MIDI: Verificar mapeo de CCs
- Tests de performance: Verificar CPU usage

### 3.10. Documentación Ausente
**Ubicación:** General  
**Gravedad:** 🟠 MEDIO  
**Descripción:** Muchas funciones carecen de documentación.  
**Solución:** Documentar al menos:
- Todas las funciones públicas
- Parámetros de templates
- Comportamiento esperado
- Excepciones lanzadas

### 3.11. Uso de Raw Pointers
**Ubicación:** Múltiples archivos  
**Gravedad:** 🟠 MEDIO  
**Descripción:** Se usan raw pointers en lugar de smart pointers modernos.  
**Solución:**
```cpp
// En lugar de:
LCDStateManager* stateManager = nullptr;

// Usar:
std::unique_ptr<LCDStateManager> stateManager;
// o
juce::OptionalScopedPointer<LCDStateManager> stateManager;
```

### 3.12. Casting Innecesario
**Archivo:** `PluginProcessor.cpp` (línea 487)  
**Gravedad:** 🟠 MEDIO  
**Descripción:** 
```cpp
auto* channelDataL = buffer.getWritePointer(0);
auto* channelDataR = buffer.getWritePointer(1);
```
**Problema:** No se verifica si el buffer tiene suficientes canales.  
**Solución:**
```cpp
int numChannels = buffer.getNumChannels();
auto* channelDataL = buffer.getWritePointer(0);
auto* channelDataR = (numChannels > 1) ? buffer.getWritePointer(1) : channelDataL;
```

### 3.13. Inicialización de Miembros en Orden Incorrecto
**Archivo:** `PluginProcessor.h`  
**Gravedad:** 🟠 MEDIO  
**Descripción:** Los miembros se inicializan en orden diferente al declarado.  
**Solución:** Ordenar inicialización:
```cpp
// En PluginProcessor.h, orden correcto:
class CZ101AudioProcessor {
private:
    // Orden de inicialización en constructor:
    CZ101::Core::VoiceManager voiceManager;           // 1
    CZ101::MIDI::MIDIProcessor midiProcessor;         // 2
    CZ101::State::Parameters parameters;              // 3
    CZ101::State::PresetManager presetManager;        // 4
    CZ101::MIDI::SysExManager sysExManager;           // 5
    // ... resto en el mismo orden
};
```

### 3.14. Falta de Const Correctness
**Ubicación:** General  
**Gravedad:** 🟠 MEDIO  
**Descripción:** Muchos métodos que no modifican estado no son const.  
**Solución:**
```cpp
// En lugar de:
float getCurrentNote() const noexcept { return currentNote; }

// También hacer const los métodos de cálculo:
float calculateFrequency(int midiNote) const noexcept;
```

### 3.15. Excepciones No Manejadas
**Ubicación:** General  
**Gravedad:** 🟠 MEDIO  
**Descripción:** No hay try-catch en operaciones que pueden lanzar excepciones.  
**Solución:**
```cpp
void PresetManager::loadBank(const juce::File& file)
{
    try {
        if (!file.existsAsFile()) return;
        
        juce::var data = juce::JSON::parse(file);
        // ... resto del código ...
    }
    catch (const std::exception& e) {
        juce::Logger::writeToLog("Error loading bank: " + juce::String(e.what()));
    }
    catch (...) {
        juce::Logger::writeToLog("Unknown error loading bank");
    }
}
```

---

## 4. MEJORAS DE DISEÑO (BAJA PRIORIDAD)

### 4.1. Implementar Parameter Attachment Pattern
**Archivo:** `PluginEditor.cpp`  
**Gravedad:** 🔵 BAJO  
**Descripción:** Los attachments se crean manualmente.  
**Solución:** Crear factory:
```cpp
// ParameterAttachmentFactory.h
class ParameterAttachmentFactory {
public:
    static std::unique_ptr<juce::SliderParameterAttachment> 
    createSliderAttachment(juce::AudioParameterFloat& param, juce::Slider& slider) {
        return std::make_unique<juce::SliderParameterAttachment>(param, slider);
    }
    
    static std::unique_ptr<juce::ComboBoxParameterAttachment>
    createComboBoxAttachment(juce::AudioParameterChoice& param, juce::ComboBox& combo) {
        return std::make_unique<juce::ComboBoxParameterAttachment>(param, combo);
    }
};
```

### 4.2. Separar DSP de UI
**Ubicación:** General  
**Gravedad:** 🔵 BAJO  
**Descripción:** La arquitectura actual mezcla lógica de DSP con UI.  
**Solución:** Implementar arquitectura MVC más clara:
- Model: Parameters, Presets
- View: Editor, Components
- Controller: Processor, VoiceManager

### 4.3. Implementar Double Buffering para Envelopes
**Archivo:** `Voice.cpp`  
**Gravedad:** 🔵 BAJO  
**Descripción:** Los envelopes se actualizan en el thread de audio.  
**Solución:** Usar double buffering para parámetros:
```cpp
class Voice {
    struct EnvelopeParams {
        std::array<float, 8> rates;
        std::array<float, 8> levels;
        int sustainPoint;
        int endPoint;
    };
    
    EnvelopeParams dcwParams[2]; // Double buffer
    std::atomic<int> activeBuffer{0};
    
    void swapBuffers() {
        activeBuffer.store(1 - activeBuffer.load(), std::memory_order_release);
    }
};
```

### 4.4. Optimización de Rendimiento
**Ubicación:** General  
**Gravedad:** 🔵 BAJO  
**Descripción:** Oportunidades de SIMD y optimización.  
**Solución:**
- Usar `juce::dsp::SIMDRegister` para operaciones vectoriales
- Implementar oversampling selectivo
- Usar lookup tables para funciones matemáticas

### 4.5. Soporte Multi-canal
**Ubicación:** General  
**Gravedad:** 🔵 BAJO  
**Descripción:** Actualmente solo soporta stereo.  
**Solución:** Extender a layouts arbitrarios:
```cpp
bool CZ101AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Permitir cualquier layout con al menos 2 canales de salida
    return layouts.getMainOutputChannelSet().size() >= 2;
}
```

### 4.6. Implementar Undo/Redo
**Archivo:** `PluginProcessor.cpp`  
**Gravedad:** 🔵 BAJO  
**Descripción:** No hay sistema de undo/redo.  
**Solución:** Usar `juce::UndoManager` con los parámetros.

### 4.7. Mejorar Preset Format
**Archivo:** `PresetManager.cpp`  
**Gravedad:** 🔵 BAJO  
**Descripción:** El formato actual no es extensible.  
**Solución:** Usar versión en formato:
```cpp
// En saveBank:
juce::DynamicObject::Ptr obj = new juce::DynamicObject();
obj->setProperty("version", "2.0");
obj->setProperty("name", juce::String(preset.name));
// ... resto de datos ...

// En loadBank:
if (presetVar.hasProperty("version")) {
    juce::String version = presetVar["version"].toString();
    if (version == "2.0") {
        // Load v2 format
    } else {
        // Convert from old format
    }
} else {
    // Assume v1 format
}
```

### 4.8. Implementar Preset Categories/Tags
**Archivo:** `PresetManager.h`  
**Gravedad:** 🔵 BAJO  
**Descripción:** Los presets no tienen categorías.  
**Solución:**
```cpp
struct Preset {
    std::string name;
    std::string author;
    std::vector<std::string> tags; // "Bass", "Lead", "Pad", etc.
    // ... resto ...
};
```

---

## RECOMENDACIONES GENERALES

### Priorización de Tareas

1. **Inmediatamente (Semana 1):**
   - Corregir errores críticos (1.1-1.7)
   - Implementar validación de parámetros

2. **Corto Plazo (Semanas 2-3):**
   - Corregir errores de lógica (2.1-2.12)
   - Implementar tests básicos

3. **Mediano Plazo (Mes 1-2):**
   - Refactorizar malas prácticas
   - Mejorar arquitectura

4. **Largo Plazo (Mes 3+):**
   - Implementar mejoras de diseño
   - Optimización de rendimiento

### Herramientas Recomendadas

- **Static Analysis:** clang-tidy, cppcheck
- **Memory Debugging:** Valgrind, AddressSanitizer
- **Profiling:** Perf, Intel VTune
- **Testing:** Google Test, Catch2

### Estándares de Codificación

- Usar C++17 features donde apropiado
- Seguir JUCE coding standards
- Implementar RAII consistentemente
- Documentar APIs públicas

---

## CONCLUSIÓN

El proyecto CZ101 Synthesizer tiene una base sólida pero requiere correcciones críticas antes de ser considerado estable. Los errores de race condition y buffer overflow deben abordarse con urgencia.

La arquitectura general es buena, pero la separación entre DSP y UI podría mejorarse. La implementación auténtica del CZ-101 es notable y debe preservarse.

**Recomendación:** Implementar primero los fixes críticos, luego establecer un ciclo de desarrollo con tests automatizados para prevenir regresiones.

---

**Auditor realizado por:** Senior JUCE/C++ Developer  
**Fecha de auditoría:** 2025-11-02  
**Versión del informe:** 1.0
