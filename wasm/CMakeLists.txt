cmake_minimum_required(VERSION 3.22)
project(ABDCZ101_WASM VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Rutas ---
set(JUCE_PATH "C:/JUCE" CACHE PATH "Path to JUCE")
set(SRC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../Source")
set(JUCE_MODULES "${JUCE_PATH}/modules")

# --- Lista compartida single-source (anti-drift) con el build nativo ---
# DspSources.cmake define CZ101_DSP_SOURCES y CZ101_DSP_SOURCES_WASM; la
# diferencia (plugin/standalone/APVTS...) esta documentada alli.
include("${CMAKE_CURRENT_LIST_DIR}/../DspSources.cmake")

# --- Verificar JUCE ---
if(NOT EXISTS "${JUCE_MODULES}/juce_core/juce_core.h")
    message(FATAL_ERROR "JUCE modules not found at ${JUCE_MODULES}")
endif()

add_executable(cz101_dsp
    # --- JUCE Module compilation units ---
    ${CMAKE_CURRENT_SOURCE_DIR}/juce_shim/juce_core/juce_core.cpp
    ${JUCE_MODULES}/juce_audio_basics/juce_audio_basics.cpp
    ${JUCE_MODULES}/juce_events/juce_events.cpp
    ${JUCE_MODULES}/juce_data_structures/juce_data_structures.cpp
    ${JUCE_MODULES}/juce_audio_formats/juce_audio_formats.cpp
    ${JUCE_MODULES}/juce_dsp/juce_dsp.cpp

    # --- CZ-101 (lista compartida single-source, anti-drift) ---
    # La diferencia WASM frente al nativo (plugin/standalone/APVTS, etc.) esta
    # documentada en DspSources.cmake — NO es drift.
    ${CZ101_DSP_SOURCES_WASM}

    # --- WASM Bridge (split into focused modules) ---
    ${SRC_ROOT}/Wasm/WasmBridge.cpp
    ${SRC_ROOT}/Wasm/WasmParams.cpp
    ${SRC_ROOT}/Wasm/WasmPresets.cpp
    ${SRC_ROOT}/Wasm/WasmBank.cpp
    ${SRC_ROOT}/Wasm/WasmEnvelopes.cpp
    ${SRC_ROOT}/Wasm/WasmMacros.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/juce_shim/wasm_link_stubs.cpp
)

# --- Include directories ---
target_include_directories(cz101_dsp PRIVATE
    ${SRC_ROOT}
    ${SRC_ROOT}/Core
    ${SRC_ROOT}/DSP
    ${SRC_ROOT}/DSP/Effects
    ${SRC_ROOT}/DSP/Envelopes
    ${SRC_ROOT}/DSP/Filters
    ${SRC_ROOT}/DSP/Modulation
    ${SRC_ROOT}/DSP/Oscillators
    ${SRC_ROOT}/State
    ${SRC_ROOT}/MIDI
    ${SRC_ROOT}/Wasm
    ${JUCE_MODULES}
    ${CMAKE_CURRENT_SOURCE_DIR}/juce_shim
)

# --- JUCE module configuration defines ---
target_compile_definitions(cz101_dsp PRIVATE
    JUCE_MODULE_AVAILABLE_juce_core=1
    JUCE_MODULE_AVAILABLE_juce_audio_basics=1
    JUCE_MODULE_AVAILABLE_juce_events=1
    JUCE_MODULE_AVAILABLE_juce_data_structures=1
    JUCE_MODULE_AVAILABLE_juce_audio_formats=1
    JUCE_MODULE_AVAILABLE_juce_audio_processors=0
    JUCE_MODULE_AVAILABLE_juce_dsp=1
    JUCE_MODULE_AVAILABLE_juce_graphics=0
    JUCE_MODULE_AVAILABLE_juce_gui_basics=0
    JUCE_MODULE_AVAILABLE_juce_gui_extra=0
    JUCE_MODULE_AVAILABLE_juce_audio_devices=0
    JUCE_MODULE_AVAILABLE_juce_audio_utils=0
    JUCE_MODULE_AVAILABLE_juce_audio_plugin_client=0
    JUCE_MODULE_AVAILABLE_juce_opengl=0
    JUCE_MODULE_AVAILABLE_juce_video=0
    JUCE_MODULE_AVAILABLE_juce_cryptography=0

    JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
    JUCE_STANDALONE_APPLICATION=0
    # Con __SSE__ definido (receta SIMD, arriba) JUCE deduce JUCE_INTEL y
    # emite asm x86 (bswap en juce_ByteOrder) que WASM no soporta. Este es el
    # interruptor oficial de JUCE para targets sin inline asm.
    JUCE_NO_INLINE_ASM=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CAMERA=0
    JUCE_PLUGINHOST_VST=0
    JUCE_PLUGINHOST_VST3=0
    JUCE_PLUGINHOST_AU=0
    JUCE_PLUGINHOST_LADSPA=0
    JUCE_PLUGINHOST_LV2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_USE_FLAC=0
    JUCE_USE_OGGVORBIS=0
    JUCE_USE_MP3AUDIOFORMAT=0
    JUCE_USE_LAME_AUDIO_FORMAT=0
    JUCE_USE_WINDOWS_MEDIA_FORMAT=0
    JUCE_WASAPI=0
    JUCE_DIRECTSOUND=0
    JUCE_ALSA=0
    JUCE_JACK=0
    JUCE_ASIO=0
    JUCE_USE_ANDROID_OBOE=0
    JUCE_USE_WINRT_MIDI=0
    JUCE_DISABLE_NATIVE_FILECHOOSERS=1
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_CONTENT_SHARING=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_CATCH_UNHANDLED_EXCEPTIONS=0
    JUCE_LOG_ASSERTIONS=0

    JUCE_APPLICATION_NAME_STRING="ABDCZ101_WASM"
    JUCE_APPLICATION_VERSION_STRING="0.1.0"
    
    NDEBUG
)

target_compile_options(cz101_dsp PRIVATE
    -Wno-zero-length-array
    # ABDSharedCode/LutDSP/LutEvaluatorSimd.h usa intrinsics SSE (__m128).
    # Receta Emscripten: -msimd128 activa WASM SIMD128 y los shims
    # system/include/compat (immintrin.h etc.) traducen los intrinsics SSE;
    # exigen __SSE__/__SSE2__ definidos, que emcc no pone por defecto.
    -msimd128
    "SHELL:-D__SSE__ -D__SSE2__"
    "SHELL:-include immintrin.h"
    "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/juce_shim/wasm_compat.h"
)

# --- Emscripten linker flags ---
set_target_properties(cz101_dsp PROPERTIES
    SUFFIX ".js"
    LINK_FLAGS "\
        -s WASM=1 \
        -s MODULARIZE=1 \
        -s EXPORT_ES6=1 \
        -s EXPORT_NAME=CZ101DSP \
        -s EXPORTED_FUNCTIONS=[\"_wasm_init\",\"_wasm_process\",\"_wasm_set_param\",\"_wasm_get_param\",\"_wasm_note_on\",\"_wasm_note_off\",\"_wasm_midi_message\",\"_wasm_load_sysex\",\"_wasm_save_sysex\",\"_wasm_load_preset\",\"_wasm_save_preset\",\"_wasm_get_preset_name\",\"_wasm_load_bank\",\"_wasm_save_bank\",\"_wasm_rename_preset\",\"_wasm_delete_preset\",\"_wasm_move_preset\",\"_wasm_get_current_preset_index\",\"_wasm_load_factory_bank\",\"_malloc\",\"_free\"] \
        -s EXPORTED_RUNTIME_METHODS=[\"ccall\",\"cwrap\",\"setValue\",\"getValue\",\"HEAPF32\",\"HEAPU8\"] \
        -s ALLOW_MEMORY_GROWTH=1 \
        -s INITIAL_MEMORY=33554432 \
        -s STACK_SIZE=1048576 \
        -s NO_EXIT_RUNTIME=1 \
        -s ENVIRONMENT=web,worker \
        -s SINGLE_FILE=1 \
        -s MALLOC=emmalloc \
        -s ASSERTIONS=0 \
        --strip-all \
        -O3 \
        --no-entry \
    "
)

# --- Output directory: directly into WebUI/wasm/ ---
set_target_properties(cz101_dsp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../WebUI/wasm"
)

set_source_files_properties(
    "${JUCE_MODULES}/juce_data_structures/juce_data_structures.cpp"
    PROPERTIES COMPILE_DEFINITIONS "JUCE_LINUX=1"
)

message(STATUS "CZ-101 WASM build configured.")
message(STATUS "  Output: WebUI/wasm/cz101_dsp.{js,wasm}")
