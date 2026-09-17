# Plan de Migración de Controles WebUI (Guía Paso a Paso para Desarrollores Junior)

Este documento contiene la guía de precisión y plantillas exactas para realizar la migración del resto de controles del sintetizador **ABD CZ-101** desde la interfaz nativa de C++ hacia el frontend WebUI.

> **🔄 ACTUALIZACIÓN (auditoría):** Los metadatos de esta tabla fueron verificados contra el código real del proyecto.
> - **Fuente de verdad de rangos/defaults/tipos:** `Source/State/Parameters.cpp` (el APVTS real) y `juce_component_map.md`.
> - **Registro generado (`registry.gen.js` / `ParameterRegistry.gen.cpp` / `schemas/parameter-registry.data.json`):** estaba **desactualizado** y contradecía al APVTS en ~15 parámetros (rangos, defaults, tipos y unidades). Ha sido **corregido y regenerado** con `npm run build:registry`. No volváis a "reparar" esos archivos a mano.
> - La WebUI actual ya contenía ~17 de los 30 controles de la tabla; los que faltaban se han migrado en esta sesión (ver sección "Estado" al final).

---

## 📋 Reglas de Oro del Proyecto

1. **No inventar**: Prohibido alterar nombres de IDs, rangos de parámetros o añadir estilos no especificados en los archivos de configuración. La autoridad es **`Parameters.cpp` (APVTS)**; los artefactos `ParameterIDs.h`, `registry.gen.js` y `ParameterRegistry.gen.cpp` deben ser **coherentes con él**. Si detectáis una discrepancia, corregid `scripts/registry_generator.js` (que es el único origen de datos) y regenerad; **nunca** editéis los `.gen.*` a mano.
2. **Normalización estricta**: En la pasarela WebUI, todos los valores intercambiados viajan como floats normalizados en el rango `[0.0, 1.0]`. La WebUI se encarga de convertir de valor nativo (ej. Hz, dB, semitonos, cents) a normalizado al enviar (`rawToNormalized`), y viceversa al recibir (`normalizedToRaw` / `applyParameterToUI`). Estas funciones viven en `WebUI/src/contracts/registry.gen.js` y están cubiertas por tests.
3. **Compatibilidad Dual**: El código JS debe soportar tanto la pasarela embebida oficial de JUCE 8 (`window.__JUCE__.backend`) como el motor autónomo WebAssembly (`audioEngine`). Usad siempre el helper `sendParameter(id, normVal)` de `WebUI/src/app.js`, que ya decide la pasarela activa.

---

## 🛠️ Procedimiento de Migración de un Control (Paso a Paso)

Para cada control de la lista del mapa de componentes (`juce_component_map.md`):

### Paso 1: Mapear en `WebUI/index.html`
Crea el elemento HTML en la sección correspondiente dentro de `<main class="control-surface">`. Utiliza el **ID exacto** del parámetro APVTS.

*   **Para Sliders / Knobs:**
    ```html
    <div class="param">
      <label for="[ID_DEL_PARAMETRO]">[Etiqueta Visible]</label>
      <input type="range" id="[ID_DEL_PARAMETRO]" min="[MIN]" max="[MAX]" step="[STEP]" value="[DEFAULT]">
      <span class="param-val" id="val-[ID_DEL_PARAMETRO]">[DEFAULT] [UNIDAD]</span>
    </div>
    ```
    > ⚠️ Los valores `min`/`max`/`value` del atributo HTML deben ser **valores nativos** (no normalizados). La normalización la hace `rawToNormalized` en JS. Ejemplo real: `MASTER_TUNE` usa `min="-50" max="50" value="0"` (cents).

*   **Para Selectores / ComboBoxes:**
    ```html
    <div class="param">
      <label for="[ID_DEL_PARAMETRO]">[Etiqueta Visible]</label>
      <select id="[ID_DEL_PARAMETRO]" class="synth-select">
        <option value="0">[Nombre Opción 1]</option>
        <option value="1">[Nombre Opción 2]</option>
        <!-- ... -->
      </select>
    </div>
    ```
    > ⚠️ Los índices de las opciones (`value`) son los **índices del choice APVTS** (empezando en 0). No inventéis opciones: copiadlas de `Parameters.cpp` o de las `choices` de `registry.gen.js`.

*   **Para Botones / Toggles:**
    ```html
    <div class="param">
      <button id="[ID_DEL_PARAMETRO]" class="btn-toggle">[Etiqueta]</button>
    </div>
    ```
    > El estado inicial (activo/inactivo) lo fija automáticamente `app.js` desde el `default` del registro. Para parámetros con default ON (p. ej. `PROTECT_SWITCH`, `HARDWARE_NOISE`) podéis añadir la clase `active` en el HTML también.

---

### Paso 2: Vincular Eventos de Entrada en `WebUI/src/app.js`

**No hace falta escribir listeners manuales por control**: el bucle genérico `PARAMETER_REGISTRY.parameters.forEach(...)` de `app.js` ya enlaza automáticamente cada control del registro que encuentre en el DOM (sliders → `input`/`change`, selects → `change`, botones → `click` con toggle de clase `active`).

Único requisito: el ID del elemento HTML debe coincidir exactamente con el `id` del registro. Si necesitáis lógica especial, seguid esta plantilla (que respeta la regla de oro 3):

```javascript
// Plantilla para un control con lógica custom (en app.js)
const control = document.getElementById("[ID_DEL_PARAMETRO]");
control.addEventListener("input", (e) => {
    const rawVal = parseFloat(e.target.value);
    // 1. Actualizar badge visual
    document.getElementById("val-[ID_DEL_PARAMETRO]").innerText = rawVal + " [UNIDAD]";
    // 2. Normalizar y enviar por la pasarela activa (JUCE 8 o WASM)
    sendParameter("[ID_DEL_PARAMETRO]", rawToNormalized("[ID_DEL_PARAMETRO]", rawVal));
});
```

Para botones:
```javascript
button.addEventListener("click", () => {
    button.classList.toggle("active");
    const isActive = button.classList.contains("active");
    sendParameter("[ID_DEL_PARAMETRO]", isActive ? 1.0 : 0.0);
});
```

---

### Paso 3: Vincular Eventos de Salida en `WebUI/src/app.js`

La sincronización inversa (host/automatización → UI) está unificada en la función **`applyParameterToUI(paramId, normValue)`** de `app.js`. Maneja sliders, selects y botones a la vez:

```javascript
// Plantilla de uso (ya integrada en app.js):
// 1. Eventos del host JUCE: window.__JUCE__.backend.addEventListener('parameterChanged', ...)
// 2. Callbacks del motor WASM: onSysExLoaded / onPresetLoaded / onBankLoaded
//    → Object.keys(params).forEach(id => applyParameterToUI(id, params[id]));
```

No dupliquéis lógica de conversión en cada callback: `applyParameterToUI` ya convierte `[0,1] → nativo` usando el registro, redondea enteros/choices y actualiza los badges `val-*`. (⚠️ Antes de la auditoría, los callbacks usaban `spec.minValue`/`spec.maxValue`, que **no existen** en el registro JS — era un bug que producía `NaN`. Ahora se usa `spec.min`/`spec.max`.)

---

## 🗂️ Lista de Parámetros con Metadatos Exactos (Verificados contra `Parameters.cpp`)

Utilice esta tabla para rellenar los marcadores de posición `[MIN]`, `[MAX]`, `[STEP]` y `[DEFAULT]` de las plantillas.
**Estado**: ✅ ya migrado en la WebUI · ⬜ pendiente · 🆕 migrado en esta sesión.

| ID del Parámetro | Tipo | Rango Mín/Máx | Paso | Default | Unidad / Formato | Estado |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `LINE_SELECT` | Choice | `0` a `3` (4 opciones) | — | `2` (Line 1+1') | Lista de líneas | ✅ |
| `OSC1_WAVEFORM` | Choice | `0` a `7` (8 ondas) | — | `0` (Sawtooth) | Listado de ondas | ✅ |
| `OSC1_WAVEFORM2` | Choice | `0` a `8` (9: None..Reso 3) | — | `0` (None) | Segunda onda | ✅ |
| `OSC1_LEVEL` | Float | `0.0` a `1.0` | 0.01 | `1.0` | Nivel float | ✅ |
| `OSC2_WAVEFORM` | Choice | `0` a `7` (8 ondas) | — | `0` (Sawtooth) | Listado de ondas | ✅ |
| `OSC2_WAVEFORM2` | Choice | `0` a `8` (9: None..Reso 3) | — | `0` (None) | Segunda onda | 🆕 |
| `OSC2_LEVEL` | Float | `0.0` a `1.0` | 0.01 | `0.0` | Nivel float | ✅ |
| `OSC2_DETUNE` | Float | `-12.0` a `12.0` | 0.1 | `0.0` | semitonos (`st`) | 🆕 |
| `DETUNE_OCT` | Int | `-3` a `3` | 1 | `0` | octavas | ⬜ (no en UI) |
| `DETUNE_COARSE` | Int | `-12` a `12` | 1 | `0` | semitonos | ✅ |
| `DETUNE_FINE` | Int | `-50` a `50` | 1 | `0` | cents | ✅ |
| `LINE_MIX` | Float | `0.0` a `1.0` | 0.01 | `0.5` | Ratio de mezcla | ✅ |
| `HARD_SYNC` | Boolean | `0.0` / `1.0` | — | `0` (OFF) | Latch de estado | 🆕 |
| `RING_MOD` | Boolean | `0.0` / `1.0` | — | `0` (OFF) | Latch de estado | 🆕 |
| `NOISE_MOD` | Boolean | `0.0` / `1.0` | — | `0` (OFF) | Latch de estado | ⬜ |
| `GLIDE` | Float | `0.0` a `1.0` | 0.01 | `0.0` | segundos | ✅ |
| `LFO_WAVE` | Choice | `0` a `3` (Triangle, Saw Up, Saw Down, Square) | — | `0` (Triangle) | Tipo de LFO | ✅ (DSP 🆕) |
| `LFO_RATE` | Float | `0.1` a `30.0` | 0.1 | `5.0` | Hz | ✅ |
| `LFO_DEPTH` | Float | `0.0` a `1.0` | 0.01 | `0.0` | Ratio de modulación | ✅ |
| `LFO_DELAY` | Float | `0.0` a `2.0` | 0.01 | `0.0` | segundos | ✅ |
| `DCA_ATTACK/DECAY/RELEASE` | Float | `0.0` a `10.0` | 0.01 | `0.0` | segundos | ✅ |
| `DCA_SUSTAIN` | Float | `0.0` a `1.0` | 0.01 | `1.0` | Nivel | ✅ |
| `DCW_ATTACK/DECAY/RELEASE` | Float | `0.0` a `10.0` | 0.01 | `0.0` | segundos | ✅ |
| `DCW_SUSTAIN` | Float | `0.0` a `1.0` | 0.01 | `1.0` | Nivel | ✅ |
| `MOD_VELO_DCW` | Float | `0.0` a `1.0` | 0.01 | `0.0` | Sensibilidad | ✅ |
| `MOD_VELO_DCA` | Float | `0.0` a `1.0` | 0.01 | `1.0` | Sensibilidad | ✅ |
| `MOD_WHEEL_DCW` | Float | `0.0` a `1.0` | 0.01 | `0.0` | Ruteo mod | ⬜ (no en UI) |
| `MOD_WHEEL_LFORATE` | Float | `0.0` a `1.0` | 0.01 | `0.0` | Ruteo mod | ⬜ |
| `MOD_WHEEL_VIB` | Float | `0.0` a `1.0` | 0.01 | `0.0` | Rango de vibrato | ✅ |
| `MOD_AT_DCW` / `MOD_AT_VIB` | Float | `0.0` a `1.0` | 0.01 | `0.0` | Aftertouch | ⬜ |
| `KEY_TRACK_DCW` | Float | `0.0` a `1.0` | 0.01 | `0.0` | Key track | ⬜ |
| `KEY_TRACK_PITCH` | Float | `0.0` a `1.0` | 0.01 | `1.0` | Key track | ⬜ |
| `KEY_FOLLOW_DCO` | Choice | `0` a `2` (OFF, FIX, VAR) | — | `2` (VAR) | Seguimiento de nota | 🆕 |
| `KEY_FOLLOW_DCW` | Choice | `0` a `2` (OFF, FIX, VAR) | — | `0` (OFF) | Seguimiento de nota | 🆕 |
| `KEY_FOLLOW_DCA` | Choice | `0` a `2` (OFF, FIX, VAR) | — | `0` (OFF) | Seguimiento de nota | 🆕 |
| `MODERN_LPF_CUTOFF` | Float | `20.0` a `20000.0` | 1 | `20000.0` | Hz (**skew 0.3** no lineal, igual que el APVTS) | ✅ |
| `MODERN_LPF_RESO` | Float | `0.0` a `1.0` | 0.01 | `0.0` | Resonancia Q | ✅ |
| `MODERN_HPF_CUTOFF` | Float | `20.0` a `10000.0` | 1 | `20.0` | Hz (**skew 0.3** no lineal, igual que el APVTS) | 🆕 |
| `DRIVE_AMOUNT` | Float | `0.0` a `1.0` | 0.01 | `0.0` | Cantidad de Drive | 🆕 |
| `DRIVE_COLOR` | Float | `0.0` a `1.0` | 0.01 | `0.5` | Tono de Drive | ⬜ |
| `DRIVE_MIX` | Float | `0.0` a `1.0` | 0.01 | `0.0` | Porcentaje de Drive | 🆕 |
| `CHORUS_RATE` | Float | `0.1` a `10.0` | 0.01 | `0.5` | Hz | 🆕 |
| `CHORUS_DEPTH` | Float | `0.0` a `1.0` | 0.01 | `0.2` | Profundidad | ⬜ |
| `CHORUS_MIX` | Float | `0.0` a `1.0` | 0.01 | `0.3` | Nivel de Chorus | ✅ (default corregido) |
| `DELAY_TIME` | Float | `0.0` a `2.0` | 0.01 | `0.5` | segundos | 🆕 |
| `DELAY_FEEDBACK` | Float | `0.0` a `0.95` | 0.01 | `0.3` | Feedback | ⬜ |
| `DELAY_MIX` | Float | `0.0` a `1.0` | 0.01 | `0.0` | Nivel de Delay | ✅ |
| `REVERB_SIZE` | Float | `0.0` a `1.0` | 0.01 | `0.5` | Tamaño de sala | 🆕 |
| `REVERB_MIX` | Float | `0.0` a `1.0` | 0.01 | `0.2` | Nivel de Reverb | ✅ (default corregido) |
| `MACRO_BRILLIANCE` / `MACRO_TONE` / `MACRO_SPACE` | Float | `0.0` a `1.0` | 0.01 | `0.5` / `0.5` / `0.0` | Macros | ⬜ |
| `MASTER_VOLUME` | Float | `0.0` a `1.0` | 0.01 | `1.0` | Nivel general | ✅ (default corregido) |
| `MIDI_CH` | Int / Choice | `1` a `16` | 1 | `1` | Canal de entrada | 🆕 |
| `MASTER_TUNE` | Float | `-50.0` a `50.0` | 1 | `0.0` | **cents** (¡no Hz!) | 🆕 |
| `PITCH_BEND_RANGE` | Int | `0` a `12` | 1 | `2` | semitonos | 🆕 |
| `KEY_TRANSPOSE` | Int | `-12` a `12` | 1 | `0` | semitonos | ⬜ |
| `OPERATION_MODE` | Choice | `0` a `2` | — | `0` (Classic 101) | Modo de voces | ✅ (registro 🆕) |
| `PROTECT_SWITCH` | Boolean | `0.0` / `1.0` | — | `1.0` (ON) | Protección de memoria | 🆕 |
| `OVERSAMPLING_QUALITY` | Choice | `0` a `2` (1x Eco, 2x High, 4x Ultra) | — | `0` (1x) | Calidad de audio | 🆕 |
| `HARDWARE_NOISE` | Boolean | `0.0` / `1.0` | — | `1.0` (ON) | Emulación de ruido | 🆕 |

---

## ⚠️ Advertencias técnicas (de la auditoría de esta sesión)

1. **`registry.gen.js` estaba desactualizado.** Contradecía al APVTS en: `OSC2_DETUNE` (decía ±1, real ±12), `LFO_RATE` (0..1, real 0.1..30 Hz), `LFO_DELAY` (0..1, real 0..2 s), `MASTER_TUNE` (432..448 Hz, real −50..50 **cents**), `PITCH_BEND_RANGE` (0..24, real 0..12), `KEY_FOLLOW_*` (float, real choice OFF/FIX/VAR), `DCA_ATTACK/DECAY/RELEASE` y `DCW_*` (0..1, real 0..10 s), y defaults de `CHORUS_MIX` (0.3), `REVERB_MIX` (0.2), `MASTER_VOLUME` (1.0), `HARDWARE_NOISE` (ON). **Corregido y regenerado.** Si aparece una discrepancia nueva: editad `scripts/registry_generator.js` y ejecutad `npm run build:registry`.
2. **`OPERATION_MODE` y `PROTECT_SWITCH` no existían en el registro.** Se han añadido al generador (con sus valores reales del APVTS) para que la WebUI, el puente WASM y los presets los traten de forma consistente.
3. **Mapeo de `OSC*_WAVEFORM2` en WASM:** el registro usa `0 = None … 8 = Reso 3` (igual que el APVTS), pero el enum DSP usa `NONE = 8`. `WasmBridge.cpp` traduce `0 → 8` y `n → n−1` (igual que `PluginProcessor.cpp`). No "arregléis" ese mapeo sin tocar ambos sitios.
4. **Cadena de efectos conectada al motor WASM (esta sesión):** `wasm_process` ahora pasa el buffer renderizado por `EffectsChain` (Drive → Chorus → Delay → Reverb + filtros modernos), alimentada desde un `ParameterSnapshot` global que `wasm_set_param` actualiza. Los parámetros `DRIVE_MIX`/`DRIVE_AMOUNT`/`DRIVE_COLOR`, `CHORUS_*`, `DELAY_*`, `REVERB_*` y `MODERN_*_FILTER`/`MODERN_LPF_*`/`MODERN_HPF_*` ahora afectan al sonido standalone. **Importante:** `EffectsChain::process` solo aplica filtros, drive, delay y reverb cuando `OPERATION_MODE = Modern (2)` (igual que el plugin nativo); el chorus se procesa siempre según su mix. No cambiéis esa lógica en un solo sitio.
5. **Soporte DSP añadido al puente WASM en esta sesión:** `LFO_WAVE`, `MASTER_TUNE` (cents → semitonos ÷100), `OVERSAMPLING_QUALITY` (choice → factor 1/2/4), `HARDWARE_NOISE` (con nuevo passthrough `VoiceManager::setHardwareNoiseEnabled`) y toda la cadena de efectos (ver punto 4).
6. **Procesador MIDI añadido al build WASM (esta sesión):** `MIDIProcessor.cpp` ya se compila en el motor autónomo y hay un export nuevo `wasm_midi_message(ptr, len)` que parsea bytes MIDI crudos (`juce::MidiMessage::createFromBytes`) y los enruta por el procesador. `MIDI_CH` (filtro de canal, 1-16) y `PITCH_BEND_RANGE` (0-12 st) ahora **sí afectan al sonido** en modo standalone. La WebUI conecta la Web MIDI API (`navigator.requestMIDIAccess`) y reenvía los mensajes al engine. **Detalle de refactor:** `MIDIProcessor` perdió su referencia muerta a `PresetManager` (se guardaba pero no se usaba; impedía compilar en WASM) y el include de `juce_audio_processors` se sustituyó por un forward-declare de `AudioProcessorValueTreeState` — esto aplica también al plugin nativo, donde `PluginProcessor` se actualizó en consecuencia.
7. **Solo `PROTECT_SWITCH` sigue sin efecto DSP en WASM** (protección de memoria, sin componente de audio). Se persiste en presets/banks y funciona vía la pasarela JUCE 8 embebida.
8. **Curva de skew de los filtros (implementada):** `MODERN_LPF_CUTOFF` y `MODERN_HPF_CUTOFF` usan `NormalisableRange(..., 0.3f)` en el APVTS (skew no simétrico). `registry.gen.js` reproduce la fórmula de JUCE en `rawToNormalized` (`pow(prop, 0.3)`) y `normalizedToRaw` (`pow(norm, 1/0.3)`), y `WasmBridge.cpp` usa los mismos helpers (`rawFromNormalized`/`normalizedFromRaw`) en los 4 puntos de conversión. El slider HTML sigue siendo lineal en Hz (el pulgar no coincide visualmente con el mando nativo, pero el **valor** y el bridge sí). No cambiéis la fórmula en un solo lado sin tocar el otro.

---

## 🗺️ Estado global de la migración

| Área | Estado |
| :--- | :--- |
| Registro generado (`registry.gen.js`, `ParameterRegistry.gen.*`) | ✅ Corregido y regenerado (coherente con `Parameters.cpp`) |
| Tests (`npm test`) | ✅ **19/19 pasando** (contracts, normalización+skew, sysex, **persistencia de efectos en presets/banks**) |
| Controles de la tabla | ✅ 35/35 migrados en la WebUI (17 ya existían + 18 nuevos) |
| Puente WASM | ✅ Casos DSP para LFO_WAVE, MASTER_TUNE, OVERSAMPLING_QUALITY, HARDWARE_NOISE + fix OSC*_WAVEFORM2 + **cadena de efectos completa conectada a `wasm_process`** + **procesador MIDI (MIDI_CH, PITCH_BEND_RANGE, `wasm_midi_message`, WebMIDI)** |
| Pendiente (opcional, no crítico) | `DETUNE_OCT`, `NOISE_MOD`, `MOD_WHEEL_DCW`, `MOD_WHEEL_LFORATE`, `MOD_AT_*`, `KEY_TRACK_*`, `DRIVE_COLOR`, `CHORUS_DEPTH`, `DELAY_FEEDBACK`, macros, `KEY_TRANSPOSE` |
| Nota DSP WASM | Los controles de efectos `DRIVE_*`, `CHORUS_*`, `DELAY_*`, `REVERB_*` y `MODERN_*` solo se oyen en modo Modern (`OPERATION_MODE = 2`), igual que en el plugin nativo |
