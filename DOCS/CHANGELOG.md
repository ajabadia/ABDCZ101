# CHANGELOG

All notable changes to the CZ-101 Emulator project will be documented in this file.

## [1.2.2] - 2026-09-21
### Fixed
- **WASM output -29 dB + volume knob dead (bridge)**: `wasm_process` applied a
  second master-gain stage with the stale formula `(masterVol/10) * HEADROOM`:
  the native parameter is 0..1 (not 0..10) and `gWasmSnapshot.system.masterVol`
  was never updated after init, so the whole engine played at a fixed 0.035
  factor (-29 dB) and MASTER_VOLUME did nothing in the web engine. The native
  plugin has NO such stage (the voice applies masterVol inside, smoothed, plus
  per-voice headroom) — the stage is removed; the scalar +/-0.99 clamp stays,
  with the same SAFE_HEAP reason as 1.2.1 (no AudioBuffer helpers over
  external-data buffers on this target). Exposed by `mode-gating`,
  `scope-real-audio` and `mod-matrix-wasm` (their level thresholds were tuned
  by ear against an engine that always played 29 dB low and could never pass).
- **KEY_FOLLOW_DCW knob silent in BOTH engines**: the d4132ca refactor moved
  the DCW anti-aliasing limit into PhaseDistOscillator AND removed the KF
  FIX/VAR timbre-curve injection, leaving `smoothedMatrix.keyTrackDcw` with no
  consumer (the knob had no audible effect; `kf-vs-matrix` failed with an
  exact-zero FIX-vs-VAR difference). The authentic curve
  (`getAuthenticDCWKeytrack(note, env, mode)`) is restored in
  `Voice::calculateDCWModulation` at full amount when FIX or VAR is selected —
  shared by the native plugin and the WASM bridge; the matrix source 11 path
  is unchanged.
- **Source-scan tests broke on CRLF**: three test files read `app.js` /
  `modMatrix.js` / `themes.css` / `lcdPanel.js` with `\n` anchors via
  `readFileSync`; the sources carry CRLF, so `toContain` failed on FORMAT, not
  content. The readers now normalise CRLF to LF (`drag-drop-preset-memory`,
  `factory-modern-keytrack`, `mod-matrix-defaults`).
### Verified
- Full suite GREEN for the first time: **47/47 files, 370/370 tests** (was
  15 failures across 7 files after 1.2.1 removed the crashes that masked all
  of this).
## [Unreleased]
### Added
- **Bundle de PRODUCCION de la WebUI (`npm run bundle`)**, con el patron del
  hermano ABDMS2000: `WebUI/vite.build.config.js` (Vite resuelve los bare imports
  `@abdsynths/shared` y `@abdsynths/midi-keyb` y empaqueta el grafo de JS) y
  `scripts/build_webui.js`, que construye y DESPUES comprueba lo que salio. Los
  comandos `bundle` y `dev` vuelven al `package.json` ahora que los ficheros
  existen; `dev` es el servidor de desarrollo pelado (`vite WebUI`), sin el
  config del bundle.
  - **La cascada no se empaqueta, y es la decision del fichero.** Vite trata cada
    `<link rel=stylesheet>` como entrada de su grafo CSS y los funde en un unico
    `assets/index.css` (medido: ademas se come el `<link>` de Google Fonts). Con
    once parciales en orden canonico, eso entrega "quien gana una regla" al
    empaquetador. Aqui los `<link>` se apartan del grafo con un testigo en la
    fase `pre` y se reponen en la `post`, y los once parciales se copian
    VERBATIM a `dist/styles/`: la hoja del bundle es, por construccion, la de la
    pagina de desarrollo. `validate_css_order.js` aprende a mirar el
    `dist/index.html` con el MISMO `EXPECTED_ORDER`, y `build_webui.js` falla si
    el orden se mueve o si un parcial deja de ser identico al de origen.
  - **`juce.js` no esta en el repo** (lo inyecta el WebView): sale del grafo con
    el mismo truco y vuelve con su URL original, para que el build no falle
    intentando resolverlo.
  - `juce.js` y el binario del motor son dos cosas distintas: el PEGAMENTO
    (`wasm/cz101_dsp.js`) si esta y el bundle lo exige; el `.wasm` no esta en el
    arbol (lo produce el build de emscripten) y el bundle lo arrastra en
    cuanto aparezca.
- **Distintivo VIVO en la cabecera del cajon de bloques (parametros tocados por
  bloque)**: el cajon de bloques es uno para toda la maquina, asi que sus 17
  secciones se turnaban para decir CUAL era la abierta y nada mas. Ahora la
  cabecera lleva el dato de la seccion visible: `3/12`, celdas que se han
  apartado del default de FABRICA sobre las que tiene la seccion. Se lee del
  DOM vivo, no del HTML: la seccion de la matriz de modulacion construye sus 24
  celdas en tiempo de ejecucion y tambien tiene distintivo (0/24 al abrir).
  La verdad es la del motor, no una cuenta del DOM: los dos lados se comparan
  ya NORMALIZADOS con el mapeo del contrato.
  - **La trampa del skew (medida, y por eso hay regresion)**: los dos lados NO
    llevan la misma unidad. El CONTROL lleva el valor normalizado cuando el
    parametro tiene skew (es lo que escribe `applyParameterToUI`), pero el
    `default` del registro llega siempre en CRUDO. Con el default pasado por la
    regla del control, el corte del paso alto salia **1/1 al abrir**, senalando
    como tocado un control que nadie habia movido. Son dos funciones
    (`normalizedValueOf` para el control, `factoryValueOf` para la fabrica).
  - **Fuera del contrato no se cuenta**: un id que el registro no declara no
    trae default con el que compararse (el `MACRO_CONTOUR` de la ficha VOICE) y
    un control sin valor legible tampoco (los 17 readout de la matriz, los
    24 ids de la seccion son 41). No se inventa un default para poder marcarlo.
  - **El contrato de foco es el compartido**: `overlayFocus.js` sigue siendo el
    del cajon (sin copia). El distintivo se respeta desde dentro —cerrado, el
    cajon lleva `inert` + `aria-hidden` y el distintivo se vacia al cerrar; al
    abrir se repinta ANTES de que entre el foco, para que la region viva
    (`role="status"`, `aria-live="polite"`) diga la verdad de la seccion que se
    esta anunciando. Los gestos se oyen por delegacion en el cuerpo del cajon;
    los valores que llegan SIN evento (preset, banco, push del motor, CC
    aprendido) repintan desde `applyParameterToUI`, que es la puerta publica y
    ahora envuelve al pintado de la celda.
  - **Lo que el distintivo ensena de la pagina, sin haber movido nada**: dos
    bloques nacen con una celda fuera de fabrica —VOICE ENGINE `1/6` por
    `LINE_SELECT` (la pagina arranca en Line 1, indice 0, y el contrato dice
    Line 1+1, indice 2) y ARPEGGIATOR `1/9` por `ARP_GATE` (la pagina escribe
    0.8 y el contrato dice 0.5). No se toco ninguno: el valor con el que arranca
    el motor es decision de la pagina, no del distintivo.
- `WebUI/tests/block-badge.test.js`: 12 casos (12 ficheros mas, **48/48 y
  383/383** en verde; antes 47/47 y 370/370), con la regla de skew, la
  regresion del default, el filtrado de lo que el contrato no puede juzgar, el
  orden refrescar-antes-del-foco y el embudo de `applyParameterToUI`.

### Fixed
- **La pagina moria a mitad de `app.js` y se llevaba por delante el selector de
  tema, el filmstrip de los deslizadores y el ajuste del lienzo**:
  `createNavbar({ ..., themeSwitcher, ... })` se ejecutaba antes de
  `let themeSwitcher = null`, y en un modulo eso no es `undefined` sino un
  `ReferenceError`: "Cannot access 'themeSwitcher' before initialization". La
  excepcion abortaba el cuerpo del modulo, asi que `new ThemeSwitcher(...)`,
  `enhanceRangeInputs(...)` (los faders de pelicula) y `mountFitStage(...)` (el
  encaje del lienzo) no se ejecutaban nunca. Medido el 2026-09-28 en las dos
  paginas (desarrollo y bundle) con el E2E que las compara: el error sale en
  ambas, con el nombre minificado en el bundle, que es la misma causa; y en las
  dos el selector de tema salia vacio. El bloque se mueve ANTES de armar el
  navbar, que es lo unico que hace falta —es un fallo de ORDEN, no de valores:
  `currentThemeId` y `navbarApi` se declaran mucho antes, y el `onChange` solo
  corre cuando el usuario cambia de tema, con el navbar ya vivo—. Lo destapo la
  comparacion dev-vs-dist del bloque: el distintivo se calcula antes de esa linea,
  asi que la pagina "funcionaba" para el distintivo y no para el resto.
- **La regla de skew estaba escrita de tres formas y dos no coincidian**: la
  condicion "este control lleva el valor NORMALIZADO" vivia duplicada en
  `rawToNormalized`/`normalizedToRaw` (contrato), `normalizedValueOf`
  (distintivo de bloques) y dos ramas de `app.js` (`applyParameterToControl` y el
  `updateVal` del input). Dos de ellas trataban `skew: 1.0` como "con skew" y la
  tercera no, asi que un parametro con la identidad declarada en la APVTS habria
  escrito el control en normalizado donde el motor espera el CRUDO. Hoy es
  latente (los 116 parametros del contrato: 114 sin `skew`, 2 con `0.3`, ninguno
  con `1.0`), pero las copias se podian separar en silencio. Ahora hay UN
  predicado, `usesSkew(spec)`, en el contrato generado — y tambien en la
  plantilla de `scripts/registry_generator.js`, que es de donde sale ese
  fichero, para que la regeneracion no lo pierda.
- **Un preset que no mencionaba `MODERN_HPF_CUTOFF` lo reiniciaba a 10 kHz**:
  el reinicio de los parametros no mencionados (`presetLoaded`) pasaba el
  `default` del registro tal cual cuando el parametro tenia skew, pero ese
  `default` llega siempre en CRUDO (asi lo declara la APVTS): el 20 Hz del paso
  alto llegaba a `applyParameterToUI` como si fuera el normalizado y se saturaba
  a `1.0`. Ahora el default entra SIEMPRE por `rawToNormalized`, con skew o sin
  el — la misma verdad que ya usaba `factoryValueOf` en el distintivo.
  (`MODERN_LPF_CUTOFF` no lo notaba: su default es 20000, que saturado y crudo
  coinciden por accidente.)
- Regresion de lo anterior en `tests/skew.test.js` (+4 casos): la identidad cuenta
  como "sin skew", el contrato solo tiene los dos cutoffs con `0.3`, ningun
  modulo vuelve a escribir la regla a mano (escaneo de `skew &&` y `.skew ?`), y
  el reinicio de fabrica pasa por el mapeo.

### Pending
- **El distintivo solo se ve con el cajon abierto**, y el gesto que lo haria
  util —saber que bloque esta movido ANTES de abrirlo— es justo el que el cajon
  esconde. En la superficie cada bloque ya tiene su boton EDIT: ahi es donde
  tendria que vivir la senal, si se decide que viva. Decision de superficie, no
  un fallo del distintivo.
- **Dos bloques nacen con una celda fuera de fabrica** (VOICE ENGINE `1/6` por
  `LINE_SELECT`, ARPEGGIATOR `1/9` por `ARP_GATE`): el valor con el que arranca
  el motor es decision de la pagina, no del distintivo. Si el badge debe salir en
  `0/N` al abrir, hay que alinear markup o contrato — y eso cambia lo que el
  motor arranca leyendo.
- ~~**El distintivo nunca se ha visto pasar por un bundle de PRODUCCION**~~ —
  **RESUELTO (2026-09-28)**: `WebUI/e2e/blockBadgeBundle.spec.js` abre el MISMO
  cajon en las dos paginas (desarrollo en el 5236, bundle servido en el 5239) y
  compara el dato, la PINTURA de la cabecera (por hash) y el orden de la hoja
  que cada una tiene cargada. Medido: `1/6` en VOICE ENGINE, `PERFORMANCE
  CONTROLS` y `DCO OSCILLATOR 1` dan el mismo dato y el mismo hash de cabecera en
  dev y en dist, y los once parciales llegan en el mismo orden en las dos
  (la hoja importada desde JS sale como un doceavo `index.css` en el bundle,
  donde en desarrollo la inyecta el cliente de Vite como `<style>` sin href: el
  mismo sitio en la cascada). La comparacion, ademas, **destapo un fallo real**
  de la pagina de desarrollo, que quedo arreglado en la entrega siguiente.
- **PROPUESTO, no hecho: que CMake sirva `WebUI/dist/` en vez de `WebUI/`.** La
  comparacion NO lo pide —el bundle es fiel en dato, pintura y cascada—, asi que
  sigue siendo decision del lado nativo. Lo que haria falta, en este orden:
  (1) que `cz101_dsp.wasm` este en el arbol (hoy solo esta el pegamento, y el
  WebView lo resuelve con `new URL(..., import.meta.url)`, o sea al lado del
  empaquetado); (2) construir el bundle ANTES de generar los binarios, porque
  `juce_add_binary_data` congela el glob en tiempo de configuracion; (3)
  cambiar el glob de `CMakeLists.txt:37-47` de `WebUI/src/*` a `WebUI/dist/*` y
  anadir la dependencia de `npm run bundle` al target. Sin (1) y (2), el
  plugin serviria un bundle sin motor.
- **El camino de preset/banco no se ejercita en ningun navegador**: necesita el
  motor de audio y el reloj del `AudioContext`, que en este navegador no avanza
  sin `--mute-audio`. Queda cubierto por el test de cableado
  (`block-badge.test.js`), no por un clic.

## [1.2.1] - 2026-09-21
### Fixed
- **Critical (WASM heap corruption)**: the offline WASM tests (36 failures across
  `dsp-offline`, `mod-matrix-wasm`, `kf-vs-matrix`, `write-wasm`, `transpose-wasm`...) died
  with `memory access out of bounds`. Root cause, isolated by rebuild with
  `SAFE_HEAP` + `ASSERTIONS=2` + emmalloc debug assertions and a code-level bisect:
  `juce::AudioBuffer::applyGainRamp` over an EXTERNAL-data buffer, under the WASM recipe
  (`-O3 -msimd128`), wrote past the emmalloc region of the caller-provided output buffers
  (SAFE_HEAP flagged the OOB store inside `wasm_process`; emmalloc flagged inconsistent
  regions on the following `free`). Fix in `Source/Wasm/WasmBridge.cpp`: the master-gain
  ramp + `+/-0.99` clamp are hand-rolled scalar loops with identical semantics (no
  `AudioBuffer` wrapper on the gain stage). Only the WASM bridge used this path.
- **Build config drift**: `wasm/CMakeLists.txt` now includes `node` in `-sENVIRONMENT`
  (the shipped binary always ran under node for the test suite, but the committed config
  could not rebuild it).
### Verified
- `dsp-offline` (pitch accuracy, voice termination, param bridge) passes; suite-wide
  failures drop from 37 to 16 with ZERO new failures — every remaining failure predates
  this change and was previously masked by the crashes.

## [1.2.0] - 2025-12-15
### Added
- **8-Stage Envelopes**: Complete implementation of the CZ-101's unique 8-stage envelope system (Rate/Level) for Pitch, Timbre (DCW), and Amplitude (DCA).
- **Pitch Envelope**: Added dedicated DCO envelope functionality and UI editor (Magenta trace).
- **Graphic Editors**: Added three spline-based graphical editors to the main interface for intuitive envelope shaping.
- **Envelope Setters**: Exposed direct control methods in `Voice` and `VoiceManager` for real-time envelope manipulation.
- **Reverb Effect**: Integrated Reverb module with Size/Mix controls in the UI.
- **Performance Monitor**: Added real-time CPU usage display to the plugin LCD.
- **Pitch Modulation**: Implemented DCO Pitch Envelope modulation logic in the Voice engine.

## [1.1.2] - 2025-12-15
### Fixed
- **Bug Fix**: Resolved "Stuck Notes" issue by implementing "Same-Note Retriggering" in `VoiceManager`. Playing the same note rapidly now reuses the correct voice instead of allocating duplicates that could get stuck in Sustain.
- **UI Improvement**: Added clear section headers (DCO, DCW, DCA, FX) and improved Knob label layout to prevent clipping.

## [1.1.1] - 2025-12-15
### Fixed
- **Critical Bug**: Fixed an issue where changing knobs (UI) or loading presets updated the visual controls but did not propagate values to the Audio Engine (`VoiceManager`). Now all parameters (Oscillators, Envelopes, DCW) update the sound in real-time.

## [1.1.0] - 2025-12-15

### Added
- **UI Redesign**: Implemented complete horizontal layout matching the original CZ-101 hardware specifications (Oscillators -> Envelopes -> Effects).
- **LCD Display**: Added a virtual LCD screen in the plugin header showing the current preset name.
- **Standalone MIDI Output**: Added logic to select and drive external MIDI hardware from the standalone application's virtual keyboard.
- **SysEx Support**: Added infrastructure in `MIDIProcessor` to handle System Exclusive messages (currently a scaffold for future implementation).
- **Expanded Controls**: Added UI knobs for all envelope stages (DCW/DCA), LFO Rate, Oscillator Detune, and Waveform selectors.

### Changed
- **DSP Core**: Fixed critical inaccuracy in Phase Distortion implementation. The DCW Envelope now modulates the **Phase Distortion Amount** (Timbre) as in the original hardware, rather than modulating amplitude.
- **Preset System**: Fixed major bug where loading a preset updated the internal state but did not apply values to the audio engine. Presets now load and sound immediately.
- **Code Structure**: Unified `PluginEditor` to use direct parameter attachments for better reliability without APVTS dependency.

## [1.0.0] - 2025-12-14
- Initial release with basic sound generation (Phase Distortion Oscillators).
- Functional 8-voice polyphony.
- Basic UI with limited controls.
- Integration of Effects (Delay, Filter).
