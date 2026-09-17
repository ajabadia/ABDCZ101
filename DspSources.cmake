# ============================================================================
# ABDCZ101 - Listas compartidas de fuentes (single-source, anti-drift)
#
# Consumidas por DOS builds:
#   - CMakeLists.txt raiz (plugin nativo + tests GoldenMaster/Snapshot/SysEx)
#   - wasm/CMakeLists.txt (build WebAssembly con juce_shim)
#
# HISTORIA: el raiz usaba file(GLOB_RECURSE) (implicito) y el WASM una lista
# explicita a mano — mismo modo de fallo que sufrio ABDMS2000 (4 ficheros DSP
# sin replicar). Implicito+explicito garantiza drift silencioso al anadir un
# fichero: ahora las dos listas viven aqui, lado a lado.
#
# CZ101_DSP_SOURCES      : plugin nativo (JUCE completo, UI incluida).
# CZ101_DSP_SOURCES_WASM : subconjunto headless compilable bajo Emscripten
#                          (juce_audio_processors=0, sin juce_gui_*).
#
# DIFERENCIA WASM (documentada, NO drift) — 14 ficheros:
#   - PluginProcessor.cpp / PluginEditor*.cpp : frontend nativo del plugin.
#   - Standalone/*.cpp                        : app standalone nativa.
#   - Utils/PerformanceMonitor.cpp            : timers nativos (no RT web).
#   - State/Parameters.cpp                    : APVTS nativo (el bridge web usa
#                                               su propio estado en WasmState.h).
#   - State/BankManager.cpp / PresetManager.cpp / PresetSerializer.cpp:
#                                               API nativa sobre APVTS; el web
#                                               consume presets via WasmPresets.
#   - State/Presets/bass|strings|bells.cpp    : presets de usuario legacy del
#                                               APVTS (equivalente factory en web).
#   - State/Presets/factory_data_bank_0..3.cpp: datos legacy — el web incluye
#                                               factory_data_bank_cz101.cpp que
#                                               cubre los 64 presets via
#                                               getFactoryPresetData().
# Regla: si anades un modulo DSP al nativo, debe ir tambien en WASM (o
# justificarlo en este bloque).
#
# NOTA: al procesar este fichero, CMAKE_CURRENT_LIST_DIR apunta al directorio
# que lo contiene (la raiz del proyecto), sea quien sea el consumidor.
# ============================================================================

set(CZ101_DSP_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/Source/Core/Voice.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/Core/VoiceManager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/DSP/Effects/Chorus.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/DSP/Effects/DriveEffect.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/DSP/Effects/EffectsChain.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/DSP/Effects/Reverb.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/DSP/Effects/StereoChorus.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/DSP/Envelopes/ADSREnvelope.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/DSP/Envelopes/MultiStageEnv.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/DSP/Filters/ResonantFilter.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/DSP/Modulation/LFO.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/DSP/Oscillators/PhaseDistOsc.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/DSP/Oscillators/WaveShaper.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/DSP/Oscillators/WaveTable.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/MIDI/MIDIProcessor.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/MIDI/SysExManager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/PluginEditor.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/PluginEditor_ResourceProvider.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/PluginProcessor.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/Standalone/HeadlessRpcServer.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/Standalone/StandaloneApp.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/State/BankManager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/State/EnvelopeSerializer.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/State/FactoryBuilder.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/State/ParameterRegistry.gen.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/State/Parameters.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/State/PresetManager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/State/PresetSerializer.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/State/Presets/bass.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/State/Presets/bells.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/State/Presets/factory_data_bank_0.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/State/Presets/factory_data_bank_1.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/State/Presets/factory_data_bank_2.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/State/Presets/factory_data_bank_3.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/State/Presets/factory_data_bank_cz101.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/State/Presets/factory_names.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/State/Presets/strings.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/Utils/PerformanceMonitor.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/Wasm/WasmBank.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/Wasm/WasmBridge.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/Wasm/WasmEnvelopes.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/Wasm/WasmMacros.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/Wasm/WasmParams.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/Wasm/WasmPresets.cpp"
)

set(CZ101_DSP_SOURCES_WASM
    "${CMAKE_CURRENT_LIST_DIR}/Source/Core/Voice.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/Core/VoiceManager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/DSP/Effects/Chorus.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/DSP/Effects/DriveEffect.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/DSP/Effects/EffectsChain.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/DSP/Effects/Reverb.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/DSP/Effects/StereoChorus.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/DSP/Envelopes/ADSREnvelope.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/DSP/Envelopes/MultiStageEnv.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/DSP/Filters/ResonantFilter.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/DSP/Modulation/LFO.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/DSP/Oscillators/PhaseDistOsc.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/DSP/Oscillators/WaveShaper.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/DSP/Oscillators/WaveTable.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/MIDI/MIDIProcessor.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/MIDI/SysExManager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/State/EnvelopeSerializer.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/State/FactoryBuilder.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/State/ParameterRegistry.gen.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/State/Presets/factory_data_bank_1.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/State/Presets/factory_data_bank_2.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/State/Presets/factory_data_bank_3.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/State/Presets/factory_data_bank_cz101.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/State/Presets/factory_names.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/Wasm/WasmBank.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/Wasm/WasmBridge.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/Wasm/WasmEnvelopes.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/Wasm/WasmMacros.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/Wasm/WasmParams.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Source/Wasm/WasmPresets.cpp"
)
