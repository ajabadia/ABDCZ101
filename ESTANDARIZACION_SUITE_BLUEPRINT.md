# 🏛️ Blueprint Maestro de Estandarización de Sintetizadores: De ABDMS2000/ABDEep a ABDCZ101 y ABDNeural

> **Propósito:** Definir la especificación técnica, arquitectura de referencia y hoja de ruta exhaustiva para alinear **ABDCZ101** con los estándares consolidados en **ABDMS2000** y **ABDEep**. Este documento actúa como **blueprint canónico** para este proyecto y como guía de transferencia para futuros sintetizadores del ecosistema, con foco prioritario en la próxima estandarización de **ABDNeural**.

---

## 📑 Índice de Contenidos

1. [Visión General y Filosofía de la Suite](#1-visión-general-y-filosofía-de-la-suite)
2. [Auditoría de Documentación Existente en el Ecosistema](#2-auditoría-de-documentación-existente-en-el-ecosistema)
3. [Los 5 Pilares de la Estandarización Universal](#3-los-5-pilares-de-la-estandarización-universal)
   - [Pilar 1: CMake Modular y Enlace de Dependencias](#pilar-1-cmake-modular-y-enlace-de-dependencias)
   - [Pilar 2: SSOT de Parámetros y Generación Contract-Driven](#pilar-2-ssot-de-parámetros-y-generación-contract-driven)
   - [Pilar 3: Puente Nativo WebView2Bridge e Integración WebUI](#pilar-3-puente-nativo-webview2bridge-e-integración-webui)
   - [Pilar 4: Arquitectura DSP Híbrida y Real-Time Safety](#pilar-4-arquitectura-dsp-híbrida-y-real-time-safety)
   - [Pilar 5: Ecosistema WebUI, Estilos y Batería de Pruebas](#pilar-5-ecosistema-webui-estilos-y-batería-de-pruebas)
4. [Hoja de Ruta de Ejecución Detallada para ABDCZ101](#4-hoja-de-ruta-de-ejecución-detallada-para-abdcz101)
5. [Guía de Transferencia para ABDNeural y Futuros Sintetizadores](#5-guía-de-transferencia-para-abdneural-y-futuros-sintetizadores)

---

## 1. Visión General y Filosofía de la Suite

En el ecosistema **ABDSynths**, sintetizadores como **ABDMS2000** y **ABDEep** han alcanzado la madurez arquitectónica al resolver tres desafíos estructurales:
1. **Desincronización cero (Zero Drift):** La interfaz gráfica WebUI (HTML/CSS/JS) y el motor nativo de C++ (JUCE APVTS y WASM) se alimentan estrictamente de la misma **Única Fuente de Verdad (Single Source of Truth - SSOT)** mediante contratos JSON y generadores automáticos.
2. **Arquitectura Híbrida Desacoplada:** El puente entre C++ y WebUI a través de `WebView2Bridge` garantiza seguridad en tiempo real (*Real-Time DSP Safety*), manteniendo el hilo de audio inmune a bloqueos, asignaciones de memoria o llamadas IPC.
3. **Dualidad DSP (Autenticidad vs. Extensión Moderna):** El motor preserva con fidelidad absoluta los circuitos y peculiaridades del hardware analógico/digital vintage (*Santuario Clásico*), al tiempo que ofrece un *Modo Expandido* potenciado por la librería transversal **`ABDSharedCode`** (anti-aliasing PolyBLEP, generadores de envolventes analógicas, LFO con tempo-sync y matrices de modulación desacopladas).

**ABDCZ101** cuenta con un motor de distorsión de fase excelente (`DspCz1.h`) y tests de WebUI consolidados, pero su integración estructural aún presenta elementos aislados: su generador de parámetros mantiene un array JavaScript hardcodeado localmente, su CMake incluye cabeceras planas en vez de targets modulares, y su puente nativo carece del desacoplamiento estándar de `WebView2Bridge`.

Este blueprint traza el camino exacto para cerrar esa brecha.

---

## 2. Auditoría de Documentación Existente en el Ecosistema

Para asegurar que no trabajamos sobre premisas obsoletas, se ha auditado la documentación activa en el monorepo:

### 2.1 En `ABDSharedCode/`
* **`INTEGRATION_GUIDE.md`**:
  * *Estado:* **Parcialmente actualizado.**
  * *Diagnóstico:* Define correctamente la filosofía modular zero-copy, el árbol de directorios de `DspCore`, `DspEffects` y `SynthCore`, y el consumo de `@abdsynths/midi-keyb`. Sin embargo, no documenta en detalle el flujo del generador de registros con verificación `--check`, ni la arquitectura de `WebView2Bridge` recién unificada en ABDEep y ABDMS2000.
  * *Acción:* Debe complementarse con las directrices de este blueprint.
* **`PROFUNDIZACION_JUNIO_DEEP_CZ101.md`**:
  * *Estado:* **Actualizado y muy preciso.**
  * *Diagnóstico:* Identifica con total acierto que ABDCZ101 es hoy un "consumidor JS" muy avanzado (`@abdsynths/midi-keyb`, `@abdsynths/shared`, `mountFitStage`), pero que en C++ aún mantiene mecanismos locales que deben converger hacia `ABDSharedCode`.
* **`INVENTARIO_MIGRACIONES_POR_PROYECTO.md`**:
  * *Estado:* **Requiere matización.**
  * *Diagnóstico:* Marca a ABDCZ101 como "100% Completado" en base a la exportación inicial de `DspCz1.h` y las pruebas de `parameterLookup.js`, pero omite que la integración del pipeline de parámetros, CMake modular y `WebView2Bridge` no está alineada con ABDMS2000.

### 2.2 En `ABDCZ101/`
* **`ROADMAP.md`**:
  * *Estado:* **Desactualizado respecto a la estandarización transversal.**
  * *Diagnóstico:* Centrado exclusivamente en el Modo Expandido vs Modo Original a nivel DSP. No contempla los pilares de sincronización de contratos SSOT con `ABDSharedAssets/contracts/casio_cz101_parameter_spec.json`, la modernización de CMake ni la integración del nuevo `WebView2Bridge`.
* **`migration_plan.md`**:
  * *Estado:* **Completado pero conceptualmente obsoleto.**
  * *Diagnóstico:* Documenta la migración manual de controles HTML por desarrolladores junior. Aclara que el registro se desincronizaba frecuentemente frente a `Parameters.cpp` porque dependía de arrays hardcodeados. Debe ser reemplazado conceptualmente por el generador contract-driven de este blueprint.
* **`Scripts/registry_generator.js`**:
  * *Estado:* **Crítico / Desalineado.**
  * *Diagnóstico:* Mantiene un array `const parameters = [...]` local de 313 líneas. Si se añade o modifica un parámetro en C++ o en el contrato compartido, debe duplicarse a mano aquí. No posee flag `--check`.

---

## 3. Los 5 Pilares de la Estandarización Universal

Para que un sintetizador (ABDCZ101 ahora, ABDNeural a continuación) pertenezca formalmente a la línea de excelencia de la suite, debe implementar de forma estricta los siguientes cinco pilares:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                       SINTETIZADOR ESTANDARIZADO                            │
├────────────────────────────────┬────────────────────────────────────────────┤
│ 1. SISTEMA CMAKE MODULAR       │ add_subdirectory(ABDSharedCode)            │
│                                │ Targets: SynthCore, DspCore, WebView2Bridge│
├────────────────────────────────┼────────────────────────────────────────────┤
│ 2. CONTRATO DE PARÁMETROS SSOT │ casio_cz101_parameter_spec.json (Assets)   │
│                                │ ──► registry_generator.js (--check)        │
│                                │ ──► ParameterRegistry.gen.{h,cpp} & .js    │
├────────────────────────────────┼────────────────────────────────────────────┤
│ 3. PUENTE NATIVO WEBVIEW2      │ JuceWebView2Component + ResourceProvider   │
│                                │ Desacoplamiento de Audio Thread (Lock-free)│
├────────────────────────────────┼────────────────────────────────────────────┤
│ 4. MOTOR DSP HÍBRIDO           │ Modo Clásico: Hardware puro (uPD933/Syx)   │
│                                │ Modo Expandido: VoiceAllocator, PolyBLEP   │
├────────────────────────────────┼────────────────────────────────────────────┤
│ 5. CALIDAD & CI/CD LOCAL       │ Tests C++ (Paridad APVTS) + Vitest (WebUI) │
└────────────────────────────────┴────────────────────────────────────────────┘
```

---

### Pilar 1: CMake Modular y Enlace de Dependencias

#### Diagnóstico del Problema
En CZ-101, CMake incluye la carpeta compartida mediante:
```cmake
target_include_directories(CZ101Emulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../ABDSharedCode
    Source
    ...)
```
Esto fuerza al compilador a resolver cabeceras crudas por inclusión directa, en lugar de consumir las librerías estructuradas que CMake compila y valida con flags de optimización homogéneos.

#### Solución Estandarizada (Modelo ABDMS2000 / ABDEep)
1. **Inclusión condicional del subdirectorio compartido:**
   ```cmake
   set(ABDSHARED_CODE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../ABDSharedCode")
   if(EXISTS "${ABDSHARED_CODE_DIR}/CMakeLists.txt")
       message(STATUS "Integrating ABDSharedCode from local tree: ${ABDSHARED_CODE_DIR}")
       add_subdirectory("${ABDSHARED_CODE_DIR}" "${CMAKE_BINARY_DIR}/ABDSharedCode")
   else()
       message(STATUS "Fetching ABDSharedCode via FetchContent...")
       # Configuración FetchContent de respaldo
   endif()
   ```
2. **Enlace de targets formales:**
   ```cmake
   target_link_libraries(CZ101Emulator PRIVATE
       ABDShared::DspCore
       ABDShared::SynthCore
       ABDShared::WebView2BridgeCpp
       WebUIAssets
       juce::juce_audio_utils
       juce::juce_audio_processors
   )
   ```
3. **Estructura anti-drift con `DspSources.cmake`:**
   Mantener la partición estricta de dos listas para garantizar que el motor nativo de escritorio y el motor WASM de navegador no diverjan jamás:
   - `CZ101_DSP_SOURCES`: Lista de fuentes compartidas compiladas en nativo.
   - `CZ101_DSP_SOURCES_WASM`: Fuentes procesadas por Emscripten.

---

### Pilar 2: SSOT de Parámetros y Generación Contract-Driven

#### Diagnóstico del Problema
Actualmente, `ABDCZ101/Scripts/registry_generator.js` define los parámetros en código JavaScript imperativo. Por otro lado, en `ABDSharedAssets/contracts/casio_cz101_parameter_spec.json` ya existe un contrato declarativo exhaustivo con **144 parámetros** (`LINE_SELECT`, `OSC1_WAVEFORM`, `DCA_ATTACK`, etc.). Ambos archivos viven en universos paralelos.

#### Solución Estandarizada (Modelo ABDMS2000)
1. **Contrato Único en `ABDSharedAssets`:**
   La única fuente de verdad para rangos, curvas, nombres, unidades y CCs MIDI es:
   `ABDSharedAssets/contracts/casio_cz101_parameter_spec.json`.
2. **Refactorización de `Scripts/registry_generator.js`:**
   El script debe leer el JSON contractual e instanciar el manifiesto de artefactos:
   ```javascript
   const ARTEFACTOS = [
     {
       clave: 'h',
       rel: 'Source/State/ParameterRegistry.gen.h',
       consumidores: 'Source/State/ParameterRegistry.gen.cpp y APVTS',
     },
     {
       clave: 'cpp',
       rel: 'Source/State/ParameterRegistry.gen.cpp',
       consumidores: 'Enlace C++ de tablas estáticas',
     },
     {
       clave: 'js',
       rel: 'WebUI/src/contracts/registry.gen.js',
       consumidores: 'WebUI/src (PARAMETERS_SPEC, PARAM_LOOKUP)',
     },
     {
       clave: 'dataJson',
       rel: 'schemas/parameter-registry.data.json',
       consumidores: 'Validación de esquema y scripts CLI',
     }
   ];
   ```
3. **Modo `--check` (Detección de Drift en CI/CD):**
   ```bash
   node Scripts/registry_generator.js --check
   ```
   Compara los archivos existentes en disco contra lo que el generador produciría en memoria. Si un desarrollador o agente modifica un archivo `.gen` a mano o cambia el contrato sin regenerar, el comando retorna código de salida `1` impidiendo commits con falso verde.
4. **Registro en `package.json`:**
   ```json
   "scripts": {
     "build:registry": "node Scripts/registry_generator.js",
     "check:registry": "node Scripts/registry_generator.js --check"
   }
   ```

---

### Pilar 3: Puente Nativo WebView2Bridge e Integración WebUI

#### Diagnóstico del Problema
Los sintes en desarrollo temprano suelen experimentar crasheos o glitches de audio causados por llamadas IPC que bloquean el hilo de audio o intentan acceder a la UI directamente desde `processBlock()`.

#### Solución Estandarizada (Modelo ABDEep)
1. **Consumo de `ABDShared::WebView2Bridge`:**
   - **`JuceWebView2Component.h`:** Envuelve `juce::WebBrowserComponent` con flags de renderizado acelerado por hardware, gestión de foco (`createOverlayFocus`) y callbacks de vida del WebView2.
   - **`WebView2ResourceProvider.h/.cpp`:** Intercepta el esquema de URLs virtuales (`synth://`) sirviendo los binarios de `WebUIAssets` en memoria, evitando lecturas directas a disco durante la ejecución del plugin.
2. **Desacoplamiento Estricto del Hilo de Audio:**
   - La comunicación de parámetros desde la WebUI hacia el motor C++ se procesa mediante una cola sin bloqueos (*lock-free queue*) o mediante `juce::AudioProcessorValueTreeState::getParameter(id)->setValueNotifyingHost(normVal)`.
   - **Prohibido:** Instanciar `MessageManagerLock` dentro de `processBlock()`.
3. **Triple Compatibilidad de Ejecución:**
   - **Plugin Nativo:** Windows (WebView2) / macOS (WebKit nativo).
   - **Web / WASM:** Emscripten AudioWorklet utilizando los mismos archivos generados en `registry.gen.js`.
   - **Standalone / Dev:** Acceso directo vía Web MIDI API cuando se prueba en un navegador web local (`pnpm run dev`).

---

### Pilar 4: Arquitectura DSP Híbrida y Real-Time Safety

#### Diagnóstico del Problema
Al añadir nuevas funciones a un sintetizador clásico, se corre el riesgo de "romper el timbre original" o introducir asignaciones dinámicas de memoria prohibidas en audio profesional.

#### Solución Estandarizada
1. **Frontera de Código: Dual-Engine Sanctuary:**
   - **Motor Clásico (Santuario Intocable):**
     - Emulación de distorsión de fase Casio uPD933 (`DspCz1.h`).
     - Generadores de envolvente de 8 etapas (Rate / Level) con matemática entera/fraccionaria exacta.
     - Filtro DCW biquad original con su ventana de deformación.
     - Decodificador y cargador de bancos SysEx `.syx`.
   - **Motor Expandido (Innovación con `ABDSharedCode`):**
     - `abd::synth::VoiceAllocator<16>`: Asignación polifónica probada con algoritmos de voice stealing (*LegacyTimestamp*, *Ms2000Ladder* o *RoundRobin*), liberando a CZ-101 de la limitación de 4/8 voces.
     - `abd::synth::PolyBLEP`: Osciladores anti-aliasing para enriquecer el timbre sin el coste de un sobremuestreo masivo.
     - `abd::synth::ADSREnvelope`: Curvas analógicas exponenciales para pads y leads modernos.
     - `abd::synth::LFO`: Modulación sincronizada al tempo del host (BPM Sync) con formas aleatorias Sample & Hold.
     - `DspEffects`: Cadena de efectos compartida (Chorus BBD analógico del CZ-5000, Reverb y Delay).
2. **Directrices de Real-Time DSP Safety:**
   - Cero llamadas a `malloc`, `free`, `new`, `delete` en el hilo de audio.
   - Cero uso de `std::string`, `juce::String` o contenedores de tamaño dinámico (`std::vector::resize`, `std::vector::push_back`) dentro de `processBlock()`.
   - Pre-asignación de todos los buffers y tablas en `prepareToPlay()`.

---

### Pilar 5: Ecosistema WebUI, Estilos y Batería de Pruebas

#### Diagnóstico del Problema
Falta de uniformidad visual, uso de scripts no estandarizados y riesgo de regresiones silenciosas en la interfaz.

#### Solución Estandarizada
1. **Vanilla CSS Estricto:**
   - Prohibido el uso de frameworks pesados (Tailwind, Bootstrap). Toda la interfaz se estiliza mediante CSS nativo y tokens declarados en variables CSS de la suite (`--accent-color`, `--bg-surface`, etc.).
2. **Batería de Pruebas Automatizadas (CI/CD Local):**
   - **Tests C++:** Verificación de roundtrip APVTS (`Cz101ParamsTests.cpp`), garantizando ratio de conversión 1.0 en todo el rango.
   - **Tests JS / Vitest:** Ejecución de `npm run test` validando contratos de esquemas, exportación, normalización de datos y teclado interactivo.

---

### 3.6 La Capa Transversal de la Suite: MidiKeyboard, fitStage y Tokens de ABDSharedAssets

Un aspecto cardinal que distingue a los sintetizadores de la suite es que **no reinventan los componentes de interacción**. Todos los sintetizadores consumen una capa transversal unificada que garantiza paridad estética y funcional:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                 CAPA TRANSVERSAL COMPARTIDA (ECOSISTEMA)                    │
├─────────────────────────┬───────────────────────────────────────────────────┤
│ @abdsynths/midi-keyb    │ • Teclado analógico interactivo con textura marfil │
│                         │ • Ruedas filmstrip fotográficas (Pitch/Mod)       │
│                         │ • parameterLookup.js (Traducción CC ↔ Dominio)    │
│                         │ • JuceMidiKeyboardComponent.h (Capa C++ JUCE)     │
├─────────────────────────┼───────────────────────────────────────────────────┤
│ @abdsynths/shared       │ • fitStage.js (Escalado matricial responsivo)     │
│                         │ • createOverlayFocus (Gestión de foco accesible)  │
├─────────────────────────┼───────────────────────────────────────────────────┤
│ ABDSharedAssets/styles  │ • Variables CSS (tokens de color, chasis, LEDs)   │
│                         │ • Widgets: .synth-knob, .synth-slider, toggles    │
│                         │ • Badges: .cal-badge-dsp, .cal-badge-module       │
├─────────────────────────┼───────────────────────────────────────────────────┤
│ ABDSharedAssets/contracts│• Hardware Profiles (SysEx, Banks, Identificación)│
│                         │ • Parameter Specs (<synth>_parameter_spec.json)   │
└─────────────────────────┴───────────────────────────────────────────────────┘
```

#### 🎹 1. `@abdsynths/midi-keyb` (`MidiKeyboard`)
El componente de teclado compartido es mucho más que una hilera de teclas en pantalla:
- **Estética Vintage:** Textura visual fotográfica de marfil envejecido adaptada a sintetizadores clásicos, con micro-animaciones CSS de pulsación (`.kbd-pressure-release`) y barrido de LEDs en arpegiador (`.led-sweep`).
- **Ruedas de Expresión Filmstrip (`wheel.js`):**
  - **Pitch Bend Wheel:** Emplea filmstrips de alta resolución con física de retorno centrada por resorte virtual al soltar el cursor o touch.
  - **Modulation Wheel:** Amortiguación inercial continua sin retorno automático.
- **Rango de Interpretación:** Selector de octavas en 5 niveles (-2 a +2 con indicadores lumínicos), soporte de Aftertouch de canal y polifónico, sensibilidad a velocidad (Velocity) y respuesta multitáctil completa.
- **Motor de Lookup Físico (`parameterLookup.js`):**
  Es el puente matemático entre el mundo MIDI y el dominio físico del sintetizador. Traduce cualquier CC (0–127) a unidades físicas reales (`Hz`, `segundos`, `cents`, `semitonos`) mediante curvas `Linear`, `Exp`, `Pow` o `None`, alimentándose de los esquemas en `ABDSharedAssets/contracts/`.
- **Capa Nativa en C++:** Para proyectos que requieren instanciar el teclado directamente desde JUCE sin cargar el WebUI completo, existe `ABDShared::MidiKeyboardCpp` (`JuceMidiKeyboardComponent.h`).

#### 📐 2. Escalado Responsivo Universal (`fitStage.js`)
El mayor problema visual en plugins de audio basados en WebView2 es que el DAW (Ableton, Reaper, Logic, FL Studio) puede redimensionar la ventana a cualquier proporción arbitraria, provocando barras de desplazamiento horribles o controles deformados.
- **Solución `fitStage`:**
  Calcula en tiempo real la relación de aspecto del sintetizador (`targetWidth` / `targetHeight`) y aplica una transformación CSS matricial uniforme:
  ```javascript
  const scale = Math.min(windowWidth / targetWidth, windowHeight / targetHeight);
  stage.style.transform = `scale(${scale})`;
  ```
- **Resultado:** La interfaz del sintetizador siempre se mantiene nítida, perfectamente centrada y con su aspecto de hardware intacto, sin scrollbars y sin importar la resolución o escala del monitor (4K, HiDPI).

#### 🎨 3. Sistema de Diseño y Widgets de `ABDSharedAssets`
- **Tokens de Color y Tema:** Se prohíbe el uso de colores genéricos o estilos hardcodeados en línea. Se consumen las variables de `ABDSharedAssets/styles/`:
  - `--bg-chassis`, `--panel-surface`, `--accent-led`, `--knob-track`, etc.
- **Widgets de Precisión:**
  - **Knobs analógicos (`.synth-knob`):** Arrastre vertical suave con opción de ajuste fino (`Shift + Drag`), renderizado de anillo de valor y doble clic para restablecer al valor `default`.
  - **Sliders con pista graduada (`.synth-slider`):** Diseñados para simular controles deslizantes de hardware.
  - **Badges de Estado:** Etiquetas estándar como `.cal-badge-dsp` (verde brillante para métricas en vivo) y `.cal-badge-module` (azul para secciones activas).
- **Vanilla CSS Estricto:** Prohibición absoluta de frameworks CSS como Tailwind o Bootstrap en toda la suite. El CSS vanilla garantiza cero sobrecarga de bundle, máxima velocidad de parseo en WebView2 y compatibilidad total con Content Security Policies (CSP) estrictas en entornos embebidos.

---

## 4. Hoja de Ruta de Ejecución Detallada para ABDCZ101

Para implementar esta estandarización en ABDCZ101 de forma ordenada y sin romper el build en ningún momento, se debe seguir la siguiente secuencia:

### 📍 Fase 1: Unificación del Contrato SSOT de Parámetros
* **Paso 1.1:** Auditar `ABDSharedAssets/contracts/casio_cz101_parameter_spec.json` y verificar que incluye los 144 parámetros del APVTS de `Parameters.cpp`.
* **Paso 1.2:** Reescribir `ABDCZ101/Scripts/registry_generator.js` eliminando el array local y adaptando el motor de generación de ABDMS2000 (`registry_core.js` + lectura de `casio_cz101_parameter_spec.json`).
* **Paso 1.3:** Implementar el flag `--check` en el generador y agregar `"check:registry"` a `package.json`.
* **Paso 1.4:** Regenerar artefactos (`ParameterRegistry.gen.h`, `ParameterRegistry.gen.cpp`, `registry.gen.js`, `parameter-registry.data.json`) y validar paridad con tests existentes.

### 📍 Fase 2: Modernización del Sistema de Compilación (CMake)
* **Paso 2.1:** Modificar `CMakeLists.txt` para integrar `add_subdirectory(../ABDSharedCode)` condicional.
* **Paso 2.2:** Sustituir los include directories planos por el enlace a los targets formales (`ABDShared::DspCore`, `ABDShared::SynthCore`, `ABDShared::WebView2BridgeCpp`).
* **Paso 2.3:** Verificar que `DspSources.cmake` mantenga sincronizadas las listas nativa y WASM.
* **Paso 2.4:** Compilar en limpio con MSVC y verificar que los artefactos binarios se generan sin advertencias.

### 📍 Fase 3: Desacoplamiento del Puente Nativo con `WebView2Bridge`
* **Paso 3.1:** Actualizar `Source/Plugin/PluginEditor.h` y `.cpp` para utilizar la arquitectura de `JuceWebView2Component` y `WebView2ResourceProvider`.
* **Paso 3.2:** Auditar `processBlock()` en `PluginProcessor.cpp` para asegurar cero asignaciones y cero bloqueos de hilos (Real-Time Safety).
* **Paso 3.3:** Verificar el soporte de ejecución en navegador web (`npm run dev`) con fallback a Web MIDI API.

### 📍 Fase 4: Integración del Modo Expandido con `SynthCore`
* **Paso 4.1:** Conectar `abd::synth::VoiceAllocator<16>` en el gestor de voces del Modo Expandido, manteniendo el asignador de 4/8 voces original en el Modo Clásico.
* **Paso 4.2:** Incorporar `abd::synth::OscPolyBlep` como oscilador secundario libre de aliasing.
* **Paso 4.3:** Incorporar envolventes exponenciales `ADSREnvelope` y LFO con sincronización BPM.
* **Paso 4.4:** Ejecutar pruebas comparativas A/B para verificar que el sonido del Modo Clásico permanece 100% inalterado.

### 📍 Fase 5: Calidad, CI/CD y Cierre
* **Paso 5.1:** Ejecutar `npm run check:registry` asegurando drift cero.
* **Paso 5.2:** Ejecutar la suite completa de Vitest (`npm test`) y pruebas C++ (`Cz101ParamsTests`).
* **Paso 5.3:** Actualizar `ROADMAP.md` de ABDCZ101 reflejando la estandarización completada.

---

## 5. Guía de Transferencia para ABDNeural y Futuros Sintetizadores

Una vez estandarizado ABDCZ101, la integración de **`ABDNeural`** (sintetizador de capturas neuronales y 56 efectos de audio) seguirá exactamente la misma plantilla técnica:

```
                  PLANTILLA DE TRANSFERENCIA A ABDNEURAL
                  ══════════════════════════════════════
1. CONTRATO JSON:
   Crear `ABDSharedAssets/contracts/abd_neural_parameter_spec.json`
   con metadatos de CCs, curvas y grupos para los parámetros de síntesis y efectos.

2. GENERADOR SSOT:
   Instanciar `Scripts/registry_generator.js` en ABDNeural con flag `--check`.

3. CMAKE MODULAR:
   add_subdirectory(../ABDSharedCode) enlazando:
   - ABDShared::DspCore
   - ABDShared::DspEffects  (crucial para el catálogo de 56 efectos)
   - ABDShared::WebView2BridgeCpp
   - ABDShared::SynthCore

4. PUENTE NATIVO:
   JuceWebView2Component + WebView2ResourceProvider (desacoplamiento IPC y audio thread).

5. INTERFAZ WEBUI:
   @abdsynths/midi-keyb + @abdsynths/shared + fitStage.js en Vanilla CSS estricto.

6. BATERÍA CI/CD:
   Suite de pruebas C++ (inferencia/DSP) y Vitest (WebUI/contratos) automatizadas.
```

Siguiendo este blueprint, cualquier nuevo instrumento añadido a la suite heredará de forma inmediata la robustez, seguridad en tiempo real y calidad de ingeniería que definen a **ABDMS2000** y **ABDEep**.
