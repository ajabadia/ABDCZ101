# 🚀 ABD Z5001 (CZ-5000 Emulator) - Improvement Plan

Este documento define el plan de acción consolidado, alineado con las auditorías del 16/01/2026, la estrategia **Multi-Modelo** y la **Guía de Refactorización Definitiva**.

**Estado Actual:** `Golden Master Verified` | `Ready for Phase 2`

---

## 🚨 FASE 1: Bugs Críticos & "Subtle Errors" (Completed)
*Correcciones de estabilidad, memoria, concurrencia y crashes. Todas las tareas críticas completadas y verificadas con Golden Master.*

- [x] **1.1. Memory Leak en StandaloneApp**
  - *Problema:* `MainWindow` toma `AudioProcessor*` raw ownership.
  - *Solución:* Usar `std::unique_ptr<AudioProcessor>` estricto.

- [x] **1.2. Race Condition en SysExCallback**
  - *Problema:* `pendingSysExPreset` accede concurrentemente (Midi Thread vs Message Thread).
  - *Solución:* Implementar `juce::CriticalSection` o `std::atomic` flag + Swap.

- [x] **1.3. WaveformDisplay Data Race**
  - *Problema:* `visFifo` lectura/escritura sin protección adecuada en `PluginEditor`.
  - *Solución:* Implementar **Triple Buffering** atómico (`VisBuffer` struct con 3 buffers).

- [x] **1.4. Delay Buffer Overflow**
  - *Problema:* `MAX_DELAY_SAMPLES` fijo a 44100.
  - *Solución:* Usar `std::vector` con resize dinámico en `prepareToPlay` (support 192kHz).

- [x] **1.5. Sample Rate Initialization Safety**
  - *Problema:* `Voice` constructor no inicia SR, posible div/0 en envelopes.
  - *Solución:* Inicializar SR a 44100.0 en constructor.

- [x] **1.6. Bank Manager Use-After-Free**
  - *Problema:* Borrar preset invalida iteradores mientras se pinta.
  - *Solución:* Usar `SafePointer` en callbacks y `ScopedLock` en acceso a lista.

---

## 🎛️ FASE 2: Fidelidad por Modelo (2 Semanas)
*Implementación de las diferencias auténticas entre CZ-101 y CZ-5000.*

- [ ] **2.1. Aliasing de Fase (CZ-5000 Fix)** (Nuevo)
    - **Problema:** Acumulación de error de fase `if (phase >= 1.0)`. Rompe Hard Sync.
    - **Solución:** Usar `phase = std::fmod(phase + inc, 1.0f)`.

- [ ] **2.2. Tablas de Rates Específicas**
    - **Acción:** Tablas separadas `HardwareConstants::CZ101_RATES` (lenta) y `CZ5000_RATES` (rápida).

- [ ] **2.3. Voice Count Dinámico**
    - **Acción:** `VoiceManager` usa `getMaxVoicesForModel()` (8/16/32).

- [ ] **2.4. Tone Mix UI**
    - **Acción:** Añadir Slider "Line Mix" (feature original perdida).

- [ ] **2.5. SysEx Device ID Configurable**
    - **Acción:** Añadir parámetro global "Device ID".

---

## 🏗️ FASE 3: Modernizado y Arquitectura (1 Semana)
*Documentación de esteroides y refactorización orientada a objetos.*

- [ ] **3.1. Documentación de Características "Modern"**
    - **Acción:** Tooltip/Dialog listando: 32 Voices, Filtros SVF, Stereo FX, Macros.

- [ ] **3.2. Refactor Strategy `ISynthModel`**
    - **Acción:** `ISynthModelVisitor` para dispatcher tipos de rendering (Mono/Stereo).

- [ ] **3.3. Command Queue Pattern** (Nuevo)
    - **Acción:** Desacoplar UI y Audio usando `ThreadSafeQueue` para todos los eventos (Envelopes, Model Switch).

- [ ] **3.4. Mod Wheel / Aftertouch Smoothing** (Nuevo)
    - **Problema:** Saltos bruscos en Audio Thread.
    - **Solución:** Usar `juce::LinearSmoothedValue`.

---

## ⚡ FASE 4: Optimización Avanzada (1 Semana)
*Mejoras de rendimiento y visualización profesional.*

- [ ] **4.1. SIMD Voice Processing**
    - **Acción:** Renderizar 4 voces simultáneas con `juce::dsp::SIMDRegister`.

- [ ] **4.2. Throttle Waveform Display**
    - **Acción:** Limitar repintado a 30Hz y pintar solo `getClipBounds()`.

- [ ] **4.3. Envelope Editor Logarítmico**
    - **Acción:** Grid logarítmico para precisión en tiempos cortos.

---

## 📚 GUÍA DE REFACTORIZACIÓN DEFINITIVA

Esta sección define la metodología obligatoria para ejecutar las fases anteriores sin regresiones.

### 🛠️ 1. Setup Previo (Obligatorio)
*   **Git:** Crear branch `refactor/phase1-critical`, taggear `pre-refactor-stable`.
*   **Tools:** Configurar `ThreadSanitizer` (TSan) y `UndefinedBehaviorSanitizer` (UBSan) en CMake.
*   **Tests:** Crear suite de regresión con `Catch2` (Test de carga/guardado de Presets con hash MD5).

### 🧪 2. Estrategia de Testing "Golden Master"
Para cada preset de fábrica (0-63):
1.  Cargar preset.
2.  Renderizar 1024 samples.
3.  Guardar Hash MD5 en archivo de referencia.
4.  **CI:** En cada commit, renderizar y comparar hash. 0 tolerancia a cambios no intencionados.

### 💀 3. Trampas de JUCE a Evitar
*   **APVTS:** Usar siempre `juce::ParameterAttachment`, nunca `addListener` manual.
*   **FileChooser:** Siempre lanzar asíncronamente vía `MessageManager::callAsync`.
*   **Paint:** Usar `g.getClipBounds()` para no repintar componentes ocultos.

### 🗓️ 4. Roadmap de Ejecución Semanal
*   **Semana 1 (Fondo):** Setup CI/TSan, Tests de Regresión, Fixes Fase 1.
*   **Semana 2 (Core DSP):** Refactor `Voice` (Extract Class: Oscillator/Env/Filter Sections).
*   **Semana 3 (Thread Safety):** Implementar `SharedState` con Atomics.
*   **Semana 4 (UI/UX):** LCD Bitmap Font, Async Bank Manager.

### 💡 Principio "Single Pass"
Cada función de DSP debe tener una única responsabilidad y flujo lineal:
`UpdateEnvelopes() -> UpdateLFO() -> RenderOscillators() -> ProcessEffects()`
