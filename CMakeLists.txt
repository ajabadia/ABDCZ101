cmake_minimum_required(VERSION 3.21)

project(CZ101Emulator VERSION 1.0.0)

# C++17 mínimo
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Agregar JUCE
# Usuario tiene JUCE instalado en C:\JUCE\
set(JUCE_DIR "C:/JUCE")

if(EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(STATUS "Using local JUCE installation at: ${JUCE_DIR}")
    add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)
else()
    message(FATAL_ERROR "JUCE not found at ${JUCE_DIR}. Please verify installation.")
endif()

# Crear plugin
juce_add_plugin(CZ101Emulator
    COMPANY_NAME "CZ101"
    PLUGIN_MANUFACTURER_CODE Cz10
    PLUGIN_CODE Cz01
    FORMATS VST3 Standalone
    PRODUCT_NAME "CZ-101 Emulator"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

# Generar JuceHeader.h para compatibilidad
juce_generate_juce_header(CZ101Emulator)

# Source files
target_sources(CZ101Emulator PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    # Milestone 1: Oscillators
    Source/DSP/Oscillators/WaveTable.cpp
    Source/DSP/Oscillators/WaveTable.h
    Source/DSP/Oscillators/PhaseDistOsc.cpp
    Source/DSP/Oscillators/PhaseDistOsc.h
    Source/DSP/Oscillators/WaveShaper.cpp
    Source/DSP/Oscillators/WaveShaper.h
    # Milestone 2: Envelopes
    Source/DSP/Envelopes/ADSREnvelope.cpp
    Source/DSP/Envelopes/ADSREnvelope.h
    Source/DSP/Envelopes/MultiStageEnv.cpp
    Source/DSP/Envelopes/MultiStageEnv.h
    # Milestone 2: Voice
    Source/Core/Voice.cpp
    Source/Core/Voice.h
    # Milestone 3: Polyphony & MIDI
    Source/Core/VoiceManager.cpp
    Source/Core/VoiceManager.h
    Source/MIDI/MIDIProcessor.cpp
    Source/MIDI/MIDIProcessor.h
    # Milestone 4: Modulation
    Source/DSP/Modulation/LFO.cpp
    Source/DSP/Modulation/LFO.h
    # Milestone 5: Filters & Effects
    Source/DSP/Filters/ResonantFilter.cpp
    Source/DSP/Filters/ResonantFilter.h
    Source/DSP/Effects/Delay.cpp
    Source/DSP/Effects/Delay.h
    # Milestone 6: State Management
    Source/State/Parameters.cpp
    Source/State/Parameters.h
    Source/State/PresetManager.cpp
    Source/State/PresetManager.h
    # Milestone 7: UI
    Source/UI/CZ101LookAndFeel.cpp
    Source/UI/CZ101LookAndFeel.h
    Source/UI/Components/Knob.cpp
    Source/UI/Components/Knob.h
    Source/UI/Components/WaveformDisplay.cpp
    Source/UI/Components/WaveformDisplay.h
    Source/UI/Components/PresetBrowser.cpp
    Source/UI/Components/PresetBrowser.h
    Source/UI/Components/MIDIActivityIndicator.cpp
    Source/UI/Components/MIDIActivityIndicator.h
    # Milestone 9: Testing & Utilities
    Source/Utils/PerformanceMonitor.cpp
    Source/Utils/PerformanceMonitor.h
    Source/Utils/DSPHelpers.h
    Source/Utils/StringHelpers.h
    Source/Utils/SmoothedValue.h
)

# Include directories
target_include_directories(CZ101Emulator PRIVATE
    Source
    Source/Core
    Source/DSP
    Source/DSP/Oscillators
    Source/DSP/Envelopes
    Source/DSP/Filters
    Source/DSP/Effects
    Source/DSP/Modulation
    Source/MIDI
    Source/State
    Source/UI
    Source/UI/Components
    Source/UI/LookAndFeel
    Source/Utils
)

# JUCE modules necesarios
target_link_libraries(CZ101Emulator
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Compile definitions
target_compile_definitions(CZ101Emulator
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
)

# Mensajes informativos
message(STATUS "CZ-101 Emulator - Configuration Complete")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  JUCE Directory: ${JUCE_DIR}")


# Testing (GoogleTest)
option(BUILD_TESTS "Build tests" OFF)  # Deshabilitado hasta tener archivos de test

if(BUILD_TESTS)
    enable_testing()
    
    # GoogleTest
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
    
    # Test executable (se agregarán tests progresivamente)
    add_executable(CZ101Tests
        # Tests se agregarán en cada milestone
    )
    
    target_link_libraries(CZ101Tests
        PRIVATE
            gtest_main
            gmock_main
    )
    
    include(GoogleTest)
    gtest_discover_tests(CZ101Tests)
endif()

# Mensajes informativos
message(STATUS "CZ-101 Emulator - Configuration Complete")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Tests: ${BUILD_TESTS}")
