# CHANGELOG

All notable changes to the CZ-101 Emulator project will be documented in this file.

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
