# CHANGELOG

All notable changes to the CZ-101 Emulator project will be documented in this file.

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
