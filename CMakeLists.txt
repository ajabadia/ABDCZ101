cmake_minimum_required(VERSION 3.21)

# Definición del proyecto sin forzar el generador de VS para ser compatible con cualquier entorno moderno.
#
# 'C' esta aqui por un motivo concreto, no por adorno. JUCE lo exige: su
# JUCEModuleSupport.cmake comprueba CMAKE_C_COMPILE_OBJECT al includirse, y su
# proyecto tambien lo pide con LANGUAGES C CXX. Declarando aqui solo CXX, el C
# queda habilitado dentro del subdirectorio de JUCE, no en el ambito raiz, y el
# generador se encuentra sin la regla de compilacion de C al generar: "Missing
# variable is: CMAKE_C_COMPILE_OBJECT", con "Configuring done" ya impreso y
# Generate(step) fallando despues. Pasaba en los dos sistemas operativos a la vez.
#
# ABDAudioLab, en esta misma suite, con el mismo JUCE 8.0.4 por FetchContent,
# declara LANGUAGES C CXX y su build pasa. Esta linea es esa, puesta aqui.
project(CZ101Emulator VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- JUCE ---
#
# Antes esto era set(JUCE_DIR "D:/desarrollos/juce"): una ruta ABSOLUTA de la
# maquina de desarrollo, metida a mano. En el runner de GitHub no existe, asi que
# el `if(EXISTS)` caia al `FATAL_ERROR` y el job moria en "Configure CMake" en
# los dos sistemas operativos, antes de compilar una sola linea. Un repo cuyo
# build solo funciona en la maquina de quien lo escribio.
#
# Ahora se resuelve en este orden, y el orden es lo que importa:
#
#   1. Si quien compila pasa -DJUCE_DIR=<ruta>, se respeta. Es lo que necesita el
#      que ya tiene JUCE a mano y no quiere volver a bajarlo.
#   2. Si no, y hay un JUCE/ junto al CMakeLists, se usa ese.
#   3. Si no, se baja con FetchContent, fijando la version.
#
# El pin es 8.0.4, el mismo que usa ABDAudioLab en esta suite, donde su CI
# compila con MSVC sin problema. Fijarlo no es opcional: JUCE cambia la API
# entre minors y un `master` haria que el mismo commit compilase un dia y no al
# siguiente.
include(FetchContent)

if(NOT DEFINED JUCE_DIR)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
        set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/JUCE")
        message(STATUS "Using JUCE from the source tree: ${JUCE_DIR}")
    endif()
endif()

if(DEFINED JUCE_DIR)
    if(NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "JUCE_DIR points at ${JUCE_DIR}, and there is no CMakeLists.txt there.")
    endif()
    message(STATUS "Using JUCE from JUCE_DIR: ${JUCE_DIR}")
    add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)
else()
    message(STATUS "Fetching JUCE 8.0.4...")
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# --- Plugin Definition (Simplified for modern CMake) ---
juce_add_plugin(CZ101Emulator
    COMPANY_NAME "CZ101"
    FORMATS Standalone VST3
    PRODUCT_NAME "CZ-5000_Emulator"
    IS_SYNTH TRUE

    # POR QUE LOS DOS, Y POR QUE ESTE FALTABA. Son cosas distintas y una no
    # sustituye a la otra. Leido en JUCE 8.0.4, `extras/Build/CMake/JUCEUtils.cmake`:
    #
    #   NEEDS_WEBVIEW2    # link WebView2 statically on Windows
    #   NEEDS_WEB_BROWSER  # link webkit on Linux
    #
    # `_juce_link_optional_libraries` solo mira NEEDS_WEBVIEW2 dentro de la rama
    # `elseif(WIN32)`: en Linux no hace nada. El unico camino que enlaza
    # `pkgconfig_JUCE_BROWSER_LINUX_DEPS` -webkit2gtk- es `NEEDS_WEB_BROWSER`. Por
    # eso el runner de Ubuntu se caia en `juce_gui_extra.cpp` con
    # `fatal error: gtk/gtk.h`: `WebBrowserComponent` compila y al incluir
    # `<gtk/gtk.h>` no lo encuentra, porque nadie le dijo que lo enlazara.
    #
    # El plugin usa `WebBrowserComponent` de verdad (`Source/PluginEditor.cpp`), asi
    # que quitar el enlace de `juce_gui_extra` no era una opcion: sin el no hay
    # editor. La otra parte del arreglo es la de abajo, que ya es WebView2.
    #
    # NEEDS_CURL esta por el mismo motivo y lo encontro el paso siguiente del CI.
    # `juce_core` en Linux llama a `curl_easy_*` y `curl_multi_*`, pero JUCE solo
    # las enlaza con esta bandera: sin ella la COMPIACION pasa y el ENLAZADO se
    # cae con `undefined reference to curl_easy_cleanup` y veinte mas. El paquete
    # `libcurl4-openssl-dev` ya estaba en el `apt-get install` del runner, o sea
    # que la dependencia estaba y faltaba la bandera que la usa.
    #
    # Los tres son opcionales a proposito en JUCE, no por descuido: enlazan
    # librerias que no todo plugin necesita, y por eso hay que pedir cada una.
    NEEDS_CURL TRUE
    NEEDS_WEB_BROWSER TRUE
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
