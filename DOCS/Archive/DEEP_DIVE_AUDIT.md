# AUDIT DEEP DIVE: UI, EVENTOS Y ARQUITECTURA
**Versión**: 107-EXTENDED | **Foco**: Controles LCD, Overlays, Sistema de Eventos, Threading

## ⚠️ CRÍTICOS NUEVOS (Nivel 1)

### 1. Callback Hell en Controles LCD (Race Real)
**Archivo**: `PluginEditor.cpp:68-71`
**Código**:
```cpp
cursorLeft.onClick  = [this] { audioProcessor.getLCDStateManager().onCursorLeft(); };
```
**Problema Oculto**: Si `onCursorLeft()` desencadena una actualización de preset (ej: cambiar `OSC1_WAVEFORM`), el `LCDStateManager` puede llamar a `voiceManager.setOsc1Waveforms()`, que si el audio thread está en `renderNextBlock()`, causará **Priority Inversion**. El message thread espera a un lock interno del voice manager que nunca debería existir.
**Impacto**: En macOS, esto puede hacer que el WindowServer marque el plugin como "no responsive" si el audio thread está sobrecargado.
**Solución Urgente**:
```cpp
// En PluginEditor.h
std::atomic<bool> isProcessingLCDCommand{false};

// En lambda
cursorLeft.onClick = [this] {
    if (isProcessingLCDCommand.exchange(true)) return; // Skip si ya procesando
    juce::MessageManager::callAsync([this] {
        audioProcessor.getLCDStateManager().onCursorLeft();
        isProcessingLCDCommand = false;
    });
};
```

### 2. Memory Leak en Overlays (Sutil)
**Archivo**: `PluginEditor.h:139-141`
**Código**:
```cpp
addChildComponent(nameOverlay);
addChildComponent(aboutDialog);
addChildComponent(bankManagerOverlay);
```
**Problema**: `bankManagerOverlay` contiene un `juce::ListBox` con model `this`. Cuando el destructor de `PluginEditor` corre, el `bankManagerOverlay` se destruye primero (orden de declaración), pero su `ListBox` aún puede tener pending callbacks al message thread que apuntan a memoria liberada.
**Impacto**: Crash raro al cerrar ventana rápidamente después de abrir Bank Manager.
**Solución**:
```cpp
// En ~CZ101AudioProcessorEditor()
bankManagerOverlay.listBox.setModel(nullptr); // Explicit detach
```

### 3. SetSize() Duplicado y Race Condition
**Archivo**: `PluginEditor.cpp:70-73`
**Código**:
```cpp
setSize(800, 500); // Strict 8:5 ratio
setResizeLimits(800, 500, 1600, 1000);
setSize(800, 500); // Duplicado!
setResizeLimits(800, 500, 1600, 1000); // Duplicado!
```
**Problema**: El segundo `setSize()` dispara un `resized()` SÍNCRONO antes de que el constructor haya inicializado todos los componentes. Si `resized()` accede a `lcdDisplay` (que aún no se construyó cómo `setStateManager`), obtienes use-after-construction.
**Impacto**: Undefined behavior en builds Release, bug Heisenberg en Debug.
**Solución**: Elimina el segundo `setSize()` y el segundo `setResizeLimits()`.

### 4. SysExManager Fragment Buffer Overflow
**Archivo**: `SysExManager.cpp:handleSysEx()`
**Código**:
```cpp
fragmentBuffer.append(data, size);
while (fragmentBuffer.getSize() > 0) {
    const uint8_t* bytes = static_cast<const uint8_t*>(fragmentBuffer.getData());
    // ... si no encuentra F0, resetea
}
```
**Problema**: `fragmentBuffer` es `juce::MemoryBlock`. Si recibes SysEx corrupto sin F7, el buffer crece indefinidamente hasta >10KB. Si luego recibes un SysEx válido, el parsing puede leer más allá de `fragmentBuffer.getSize()` debido a `decodeNibblePair` sin bounds check final.
**Impacto**: Buffer overflow exploitable con MIDI malicioso.
**Solución**:
```cpp
// Después de encontrar F0 pero no F7
if (fragmentBuffer.getSize() > 10000) {
    juce::Logger::writeToLog("SysEx fragment too large, discarding");
    fragmentBuffer.reset(); // Hard reset
    return;
}
```

## 🔧 ALTA PRIORIDAD (Nivel 2)

### 5. Preset Browser Callback Sin Validación
**Archivo**: `PluginEditor.cpp:78-83`
**Debt**: Si `pitchEditorL1.updateData()` llama a `repaint()` y el componente no está visible, estás gastando CPU pintando off-screen. No hay `if (isVisible())` guard.

### 6. Menu Bar Model No Thread-Safe
**Archivo**: `PluginEditor.cpp:298-310`
**Problema**: `getMenuForIndex()` es llamado desde el message thread, pero `m.addItem()` puede invalidar si el audio thread está modificando el `PresetManager` (ej: borrando preset).
**Solución**: `juce::ScopedLock lock(presetManager.getLock());`

### 7. Tooltip Window Race
**Archivo**: `PluginEditor.h:67`
**Problema**: Si el usuario mueve el mouse rápidamente sobre 20 componentes, el `TooltipWindow` crea 20 timers internos. Al destruirse el editor, los timers pueden disparar en objetos destruidos.
**Solución**: `tooltipWindow.setTip(nullptr);` en destructor.

### 8. LookAndFeel Memory Leak
**Archivo**: `PluginEditor.cpp:42`
**Problema**:El LookAndFeel es asignado con puntero crudo. Si `changeListenerCallback` se dispara antes de que el destructor corra, `sendLookAndFeelChange()` accede a `customLookAndFeel` destruida.
**Solución**: Usa `juce::SharedResourcePointer`.

## 📐 DISEÑO ARQUITECTÓNICO (Nivel 3)

### 9. SysExManager es un God Object Inyectado
**Refactorización**: Divide en `LCDNavigator`, `LCDParameterMapper`, `LCDEditController`.

### 10. No hay "MIDI Learn" para Botones LCD
**Solución**: Haz que `LCDStateManager` herede de `juce::ChangeListener`.

## 🧪 TESTABILIDAD
### 11. Botones Son Imposibles de Unit Test
**Solución**: Usa `std::function<void()>` como miembros.

## 🎨 UI/UX DETALLES
### 12. Layout Escalable Roto en FlexBox
**Problema**: `hMid` se calcula mal en resoluciones altas.
**Solución**: Usa `juce::Grid` con fr units.

### 13. Falta de Hysteresis en Dibujo
**Problema**: Flicker al cambiar temas.
**Solución**: Chequeo de `findColour` antes de `setColour`.

## 🛡️ SEGURIDAD AVANZADA
### 14. Preset JSON Injection
**Problema**: Path traversal ("../../etc/passwd").
**Solución**: Sanitiza nombres con `juce::File::createLegalFileName`.

## 📊 INFORME DE DEUDA TÉCNICA
*   **Race Condition LCD**: 🔴 P1 (Crash en producción)
*   **Memory Leak Overlay**: 🔴 P1 (Crash en cierre rápido)
*   **SysEx Overflow**: 🔴 P1 (Buffer overflow)
*   **Menu Bar Thread-Safe**: 🟡 P2 (Corrupción de datos)
