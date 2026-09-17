cmake_minimum_required(VERSION 3.21)

project(CZ101Emulator VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(JUCE_DIR "C:/JUCE")
if(EXISTS "${JUCE_DIR}/CMakeLists.txt")
    add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)
else()
    message(FATAL_ERROR "JUCE not found at ${JUCE_DIR}.")
endif()

juce_add_plugin(CZ101Emulator
    COMPANY_NAME "CZ101"
    FORMATS Standalone VST3
    PRODUCT_NAME "CZ-5000_Emulator"
    IS_SYNTH TRUE
    NEEDS_WEBVIEW2 TRUE
)

target_compile_definitions(CZ101Emulator PUBLIC 
    JUCE_USE_WIN_WEBVIEW2=1 
    JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
)

# --- Auto-Versioning System ---
add_custom_command(TARGET CZ101Emulator PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR} -P "${CMAKE_CURRENT_SOURCE_DIR}/Scripts/UpdateVersion.cmake"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Auto-incrementing build version..."
)

juce_generate_juce_header(CZ101Emulator)

# --- WebUI Binary Assets ---
file(GLOB_RECURSE WEBUI_ASSETS 
    "WebUI/src/*" 
    "WebUI/icons/*" 
    "WebUI/*.html" 
    "WebUI/*.js" 
    "WebUI/*.webmanifest" 
    "WebUI/*.css" 
    "WebUI/*.png" 
    "WebUI/*.svg" 
    "WebUI/*.json"
)
juce_add_binary_data(WebUIAssets 
    HEADER_NAME "WebUIAssets.h" 
    NAMESPACE "WebUIAssets" 
    SOURCES ${WEBUI_ASSETS}
)
target_link_libraries(CZ101Emulator PRIVATE WebUIAssets)

# --- Source Files ---
# Lista DSP compartida con el build WASM (single-source, anti-drift):
# DspSources.cmake define CZ101_DSP_SOURCES y CZ101_DSP_SOURCES_WASM.
# Los tests de Source/Tests quedan fuera de la lista (llevan sus propios mains
# y se anaden por target en cada test de abajo). Los .h van via GLOB (solo
# afecta a la visibilidad en el IDE, no al enlazado).
include("${CMAKE_CURRENT_SOURCE_DIR}/DspSources.cmake")

file(GLOB_RECURSE SOURCES "Source/*.h")
list(APPEND SOURCES ${CZ101_DSP_SOURCES})

# UI legacy files removed (WebView2 is the sole interface)

target_sources(CZ101Emulator PRIVATE ${SOURCES})

if (MSVC)
    target_compile_options(CZ101Emulator PRIVATE /MP)
endif()

# --- Include Directories (ROBUST VERSION) ---
target_include_directories(CZ101Emulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../ABDSharedCode
    Source
    Source/Core
    Source/DSP
    Source/DSP/Effects
    Source/DSP/Envelopes
    Source/DSP/Filters
    Source/DSP/Modulation
    Source/DSP/Oscillators
    Source/MIDI
    Source/State

    Source/Utils
)

# --- JUCE Modules ---
target_link_libraries(CZ101Emulator PRIVATE
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
    juce::juce_opengl
)

# --- Compile Definitions ---
target_compile_definitions(CZ101Emulator PUBLIC
    JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
    JUCE_VST3_CAN_REPLACE_VST2=0
)

# Audit Fix 6.1: ARM Hard-Float ABI for cross-platform bit-determinism
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm" OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch32")
    target_compile_options(CZ101Emulator PRIVATE -mfloat-abi=hard -mfpu=neon)
endif()

# Audit Fix 1.5.1: Sanitizers (Address, Undefined) - Phase 1.5 Safety Net
option(ENABLE_SANITIZERS "Enable Address and Undefined Behavior Sanitizers" OFF)

if(ENABLE_SANITIZERS)
    if(MSVC)
        # MSVC Address Sanitizer (Requires Visual Studio Component installed)
        target_compile_options(CZ101Emulator PRIVATE /fsanitize=address)
        message(STATUS "Enabled MSVC Address Sanitizer (/fsanitize=address)")
    else()
        # Clang/GCC
        target_compile_options(CZ101Emulator PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(CZ101Emulator PRIVATE -fsanitize=address,undefined)
        message(STATUS "Enabled Clang/GCC Sanitizers (Address, Undefined)")
    endif()
endif()

# --- Test Executable ---
if (NOT JUCE_BUILD_HELPER_TOOLS) 
    add_executable(CZ101SysExTest
        Source/Tests/SysExTestMain.cpp
        Source/MIDI/SysExManager.cpp
        Source/State/EnvelopeSerializer.cpp
    )

    target_include_directories(CZ101SysExTest PRIVATE Source)

    # Link minimal JUCE modules
    target_link_libraries(CZ101SysExTest PRIVATE
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
    )
    
    target_compile_definitions(CZ101SysExTest PUBLIC JUCE_CONSOLE_APP=1)
    set_target_properties(CZ101SysExTest PROPERTIES CXX_STANDARD 17)

    message(STATUS "Defined Test Target: CZ101SysExTest")

    # SysEx encode/decode round-trip regression test Ã¢â‚¬â€ verifies that
    # createPatchDump() Ã¢â€ â€™ decodePatch() preserves parameters, envelopes
    # and extended modulation modes (incl. the gapped PDL formula).
    add_executable(CZ101SysExRoundTripTest
        Source/Tests/SysExRoundTripTest.cpp
        Source/MIDI/SysExManager.cpp
        Source/State/EnvelopeSerializer.cpp
    )

    target_include_directories(CZ101SysExRoundTripTest PRIVATE Source)

    target_link_libraries(CZ101SysExRoundTripTest PRIVATE
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
    )

    target_compile_definitions(CZ101SysExRoundTripTest PUBLIC JUCE_CONSOLE_APP=1)
    set_target_properties(CZ101SysExRoundTripTest PROPERTIES CXX_STANDARD 17)

    message(STATUS "Defined Test Target: CZ101SysExRoundTripTest")

endif()

# Audit Fix 1.5.2: Golden Master Regression Test Suite
if (NOT JUCE_BUILD_HELPER_TOOLS)
    # Prepare sources: Exclude Standalone wrapper (contains main)
    set(SOURCES_GM ${SOURCES})
    list(FILTER SOURCES_GM EXCLUDE REGEX "StandaloneApp\\.cpp$")
    list(FILTER SOURCES_GM EXCLUDE REGEX "PluginEditor.*\\.cpp$")
    
    juce_add_console_app(CZ101GoldenMaster
        PRODUCT_NAME "CZ101GoldenMaster"
    )
    
    target_sources(CZ101GoldenMaster PRIVATE
        Source/Tests/GoldenMasterMain.cpp
        ${SOURCES_GM}
    )
    
    juce_generate_juce_header(CZ101GoldenMaster)

    target_include_directories(CZ101GoldenMaster PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../ABDSharedCode 
        Source
        Source/Core
        Source/DSP
        Source/DSP/Effects
        Source/DSP/Envelopes
        Source/DSP/Filters
        Source/DSP/Modulation
        Source/DSP/Oscillators
        Source/MIDI
        Source/State
        Source/Utils
        .
    )
    
    # Link Full JUCE modules (needs Audio, Graphics for Editor classes)
    target_link_libraries(CZ101GoldenMaster PRIVATE
        WebUIAssets
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
        juce::juce_opengl
        juce::juce_cryptography
    )

    target_compile_definitions(CZ101GoldenMaster PUBLIC 
        JUCE_CONSOLE_APP=1 
        GOLDEN_MASTER_BUILD=1
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=0 # Ensure we don't trigger macros in headers
        JucePlugin_Name="ABD Z5001"
    )
    
    set_target_properties(CZ101GoldenMaster PROPERTIES CXX_STANDARD 17)
    
    if(ENABLE_SANITIZERS AND MSVC)
        target_compile_options(CZ101GoldenMaster PRIVATE /fsanitize=address)
    endif()

    message(STATUS "Defined Test Target: CZ101GoldenMaster")

    # ================================================================
    # Snapshot Ã¢â€ â€™ DSP regression test
    # ================================================================
    set(SOURCES_SNAP ${SOURCES})
    list(FILTER SOURCES_SNAP EXCLUDE REGEX "StandaloneApp\\.cpp$")
    list(FILTER SOURCES_SNAP EXCLUDE REGEX "PluginEditor.*\\.cpp$")

    juce_add_console_app(CZ101SnapshotDSPTest
        PRODUCT_NAME "CZ101SnapshotDSPTest"
    )

    target_sources(CZ101SnapshotDSPTest PRIVATE
        Source/Tests/SnapshotDSPTest.cpp
        ${SOURCES_SNAP}
    )

    juce_generate_juce_header(CZ101SnapshotDSPTest)

    target_include_directories(CZ101SnapshotDSPTest PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../ABDSharedCode
        Source
        Source/Core
        Source/DSP
        Source/DSP/Effects
        Source/DSP/Envelopes
        Source/DSP/Filters
        Source/DSP/Modulation
        Source/DSP/Oscillators
        Source/MIDI
        Source/State
        Source/Utils
        .
    )

    target_link_libraries(CZ101SnapshotDSPTest PRIVATE
        WebUIAssets
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
        juce::juce_opengl
        juce::juce_cryptography
    )

    target_compile_definitions(CZ101SnapshotDSPTest PUBLIC
        JUCE_CONSOLE_APP=1
        SNAPSHOT_DSP_TEST_BUILD=1
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=0
        JucePlugin_Name="ABD Z5001"
    )

    set_target_properties(CZ101SnapshotDSPTest PROPERTIES CXX_STANDARD 17)

    if(ENABLE_SANITIZERS AND MSVC)
        target_compile_options(CZ101SnapshotDSPTest PRIVATE /fsanitize=address)
    endif()

    message(STATUS "Defined Test Target: CZ101SnapshotDSPTest")
endif()
