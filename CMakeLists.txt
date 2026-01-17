cmake_minimum_required(VERSION 3.21)

project(CZ101Emulator VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
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
)

juce_generate_juce_header(CZ101Emulator)

# --- Source Files ---
file(GLOB_RECURSE SOURCES "Source/*.h" "Source/*.cpp")
# Robust Exclusion for Windows/Unix paths
list(FILTER SOURCES EXCLUDE REGEX "SysExTestMain\\.cpp$") 
list(FILTER SOURCES EXCLUDE REGEX "GoldenMasterMain\\.cpp$") 
# Add new files explicitly to ensure CMake detects them if GLOB fails to refresh
list(APPEND SOURCES 
    "Source/UI/UIManager.h"
    "Source/UI/UIManager.cpp"
    "Source/UI/Components/FilterPanel.h"
    "Source/UI/Components/FilterPanel.cpp"
    "Source/UI/Components/ChorusPanel.h"
    "Source/UI/Components/ChorusPanel.cpp"
    "Source/UI/Components/DelayPanel.h"
    "Source/UI/Components/DelayPanel.cpp"
    "Source/UI/Components/ReverbPanel.h"
    "Source/UI/Components/ReverbPanel.cpp"
    "Source/UI/Components/MacroPanel.h"
    "Source/UI/Components/MacroPanel.cpp"
    "Source/UI/Components/ArpeggiatorPanel.h"
    "Source/UI/Components/ArpeggiatorPanel.cpp"
)
target_sources(CZ101Emulator PRIVATE ${SOURCES})

if (MSVC)
    target_compile_options(CZ101Emulator PRIVATE /MP)
endif()

# --- Include Directories (ROBUST VERSION) ---
target_include_directories(CZ101Emulator PRIVATE
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
    Source/UI
    Source/UI/Components
    Source/UI/Overlays
    Source/UI/Sections
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
endif()

# Audit Fix 1.5.2: Golden Master Regression Test Suite
if (NOT JUCE_BUILD_HELPER_TOOLS)
    # Prepare sources: Exclude Standalone wrapper (contains main)
    set(SOURCES_GM ${SOURCES})
    list(FILTER SOURCES_GM EXCLUDE REGEX "StandaloneApp\\.cpp$")
    
    juce_add_console_app(CZ101GoldenMaster
        PRODUCT_NAME "CZ101GoldenMaster"
    )
    
    target_sources(CZ101GoldenMaster PRIVATE
        Source/Tests/GoldenMasterMain.cpp
        ${SOURCES_GM}
    )
    
    juce_generate_juce_header(CZ101GoldenMaster)

    target_include_directories(CZ101GoldenMaster PRIVATE 
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
        Source/UI
        Source/UI/Components
        Source/UI/Overlays
        Source/UI/Sections
        Source/Utils
        .
    )
    
    # Link Full JUCE modules (needs Audio, Graphics for Editor classes)
    target_link_libraries(CZ101GoldenMaster PRIVATE
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
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=0 # Ensure we don't trigger macros in headers
        JucePlugin_Name="ABD Z5001"
    )
    
    set_target_properties(CZ101GoldenMaster PROPERTIES CXX_STANDARD 17)
    
    if(ENABLE_SANITIZERS AND MSVC)
        target_compile_options(CZ101GoldenMaster PRIVATE /fsanitize=address)
    endif()

    message(STATUS "Defined Test Target: CZ101GoldenMaster")
endif()
