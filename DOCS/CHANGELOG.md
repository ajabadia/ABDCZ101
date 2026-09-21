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
