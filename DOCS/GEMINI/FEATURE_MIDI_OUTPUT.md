# FEATURE: MIDI OUTPUT PARA CONTROL DE HARDWARE

**Fecha:** 14 Diciembre 2025  
**Prioridad:** Media  
**Milestone sugerido:** 3 (MIDI) o 8 (Características Avanzadas)

---

## 🎯 OBJETIVO

Habilitar salida MIDI en la versión Standalone para:
1. Controlar el CZ-101 hardware original
2. Enviar bulk dumps (SysEx)
3. Usar el emulador como controlador MIDI

---

## 📋 IMPLEMENTACIÓN

### Cambios en CMakeLists.txt

```cmake
juce_add_plugin(CZ101Emulator
    # ... otros parámetros
    NEEDS_MIDI_OUTPUT TRUE  # ← Cambiar de FALSE a TRUE
)
```

### Cambios en PluginProcessor

```cpp
class CZ101AudioProcessor : public juce::AudioProcessor
{
public:
    // Agregar método para enviar MIDI
    void sendMidiMessage(const juce::MidiMessage& message);
    
    // Configurar puerto MIDI out
    void setMidiOutputDevice(const juce::String& deviceName);
    
private:
    std::unique_ptr<juce::MidiOutput> midiOutput;
};
```

### UI para Selección de Puerto

```cpp
// En PluginEditor
class MidiOutputSelector : public juce::ComboBox
{
public:
    MidiOutputSelector()
    {
        // Listar puertos MIDI disponibles
        auto devices = juce::MidiOutput::getAvailableDevices();
        for (int i = 0; i < devices.size(); ++i)
            addItem(devices[i].name, i + 1);
    }
};
```

---

## 🔧 FUNCIONALIDADES

### 1. Passthrough MIDI Básico

```cpp
void CZ101AudioProcessor::processBlock(...)
{
    // Enviar MIDI recibido al hardware
    if (midiOutput != nullptr)
    {
        for (const auto metadata : midiMessages)
        {
            midiOutput->sendMessageNow(metadata.getMessage());
        }
    }
}
```

### 2. Bulk Dumps (SysEx)

```cpp
void CZ101AudioProcessor::sendPresetToHardware(const Preset& preset)
{
    // Convertir preset a SysEx
    auto sysexData = preset.toSysEx();
    
    // Enviar
    juce::MidiMessage sysexMsg = juce::MidiMessage::createSysExMessage(
        sysexData.data(), sysexData.size());
    
    if (midiOutput != nullptr)
        midiOutput->sendMessageNow(sysexMsg);
}
```

### 3. Control Remoto

```cpp
// Enviar cambios de parámetros al hardware
void CZ101AudioProcessor::parameterChanged(const juce::String& paramID, float newValue)
{
    // Convertir parámetro a CC MIDI
    int ccNumber = parameterToCCMapping[paramID];
    int ccValue = static_cast<int>(newValue * 127);
    
    auto ccMessage = juce::MidiMessage::controllerEvent(1, ccNumber, ccValue);
    
    if (midiOutput != nullptr)
        midiOutput->sendMessageNow(ccMessage);
}
```

---

## 📝 TAREAS

- [ ] Modificar `CMakeLists.txt`: `NEEDS_MIDI_OUTPUT TRUE`
- [ ] Agregar `midiOutput` a `PluginProcessor`
- [ ] Implementar `setMidiOutputDevice()`
- [ ] Implementar `sendMidiMessage()`
- [ ] Crear UI selector de puerto MIDI
- [ ] Implementar passthrough MIDI
- [ ] Implementar bulk dump SysEx
- [ ] Documentar formato SysEx del CZ-101
- [ ] Test: Enviar nota al hardware
- [ ] Test: Enviar preset completo

---

## 🔗 REFERENCIAS

**JUCE Documentation:**
- `juce::MidiOutput` class
- `juce::MidiMessage` class
- Funcionalidad estándar de JUCE

**CZ-101 SysEx Format:**
- Consultar manual original
- Formato de bulk dumps
- CC mappings

---

## 💡 CASOS DE USO

### Caso 1: Edición Bidireccional
1. Editar preset en emulador
2. Enviar a CZ-101 hardware
3. Tocar en hardware real
4. Grabar audio del hardware

### Caso 2: Backup de Hardware
1. Recibir SysEx del CZ-101
2. Guardar como preset en emulador
3. Editar en emulador
4. Enviar de vuelta al hardware

### Caso 3: Live Performance
1. Usar emulador como controlador
2. Enviar MIDI a CZ-101 en escenario
3. Cambiar presets desde DAW
4. Sincronizar parámetros

---

**Prioridad:** Media (no bloqueante para v1.0)  
**Complejidad:** Baja (funcionalidad estándar JUCE)  
**Tiempo estimado:** 1-2 días
