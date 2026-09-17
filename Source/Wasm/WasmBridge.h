#pragma once

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

#include <cstdint>

// ─── WASM bridge public interface ───
// Every wasm_* entry point the JS/WebUI layer can call. WasmBridge.cpp is the
// slim audio core (init/process/notes/MIDI); the parameter dispatch, preset &
// bank management, envelope access and performance macros live in their own
// TUs (WasmParams / WasmPresets / WasmBank / WasmEnvelopes / WasmMacros) that
// share state through WasmState.h. All of them define (and therefore need to
// see) these declarations.

extern "C" {
    // ── Audio core (WasmBridge.cpp) ──
    EMSCRIPTEN_KEEPALIVE void wasm_init(double sampleRate);
    EMSCRIPTEN_KEEPALIVE void wasm_process(float* outputL, float* outputR, int numSamples);
    EMSCRIPTEN_KEEPALIVE void wasm_note_on(int note, float velocity);
    EMSCRIPTEN_KEEPALIVE void wasm_note_off(int note);
    EMSCRIPTEN_KEEPALIVE void wasm_midi_message(const uint8_t* data, int length);

    // ── Parameters (WasmParams.cpp) ──
    EMSCRIPTEN_KEEPALIVE void wasm_set_param(const char* paramId, float normalizedVal);
    EMSCRIPTEN_KEEPALIVE float wasm_get_param(const char* paramIdStr);

    // ── Presets (WasmPresets.cpp) ──
    EMSCRIPTEN_KEEPALIVE void wasm_load_preset(int index);
    EMSCRIPTEN_KEEPALIVE void wasm_save_preset(int index, const char* name);
    EMSCRIPTEN_KEEPALIVE void wasm_rename_preset(int index, const char* name);
    EMSCRIPTEN_KEEPALIVE void wasm_delete_preset(int index);
    EMSCRIPTEN_KEEPALIVE void wasm_move_preset(int fromIndex, int toIndex);
    EMSCRIPTEN_KEEPALIVE int wasm_get_current_preset_index();
    EMSCRIPTEN_KEEPALIVE void wasm_get_preset_name(int index, char* outName, int maxLen);

    // ── Bank + SysEx (WasmBank.cpp) ──
    EMSCRIPTEN_KEEPALIVE int wasm_load_sysex(const uint8_t* data, int length);
    EMSCRIPTEN_KEEPALIVE int wasm_save_sysex(uint8_t* outBuf, int maxLen, const char* patchName);
    EMSCRIPTEN_KEEPALIVE void wasm_load_bank(const char* jsonStr);
    EMSCRIPTEN_KEEPALIVE int wasm_save_bank(char* outJson, int maxLen);

    // ── Envelopes (WasmEnvelopes.cpp) ──
    EMSCRIPTEN_KEEPALIVE float wasm_get_envelope_rate(int envType, int line, int stage);
    EMSCRIPTEN_KEEPALIVE float wasm_get_envelope_level(int envType, int line, int stage);
    EMSCRIPTEN_KEEPALIVE int wasm_get_envelope_sustain(int envType, int line);
    EMSCRIPTEN_KEEPALIVE int wasm_get_envelope_end(int envType, int line);
    EMSCRIPTEN_KEEPALIVE void wasm_set_envelope_stage(int envType, int line, int stage, float rate, float level);
    EMSCRIPTEN_KEEPALIVE void wasm_set_envelope_sustain(int envType, int line, int point);
    EMSCRIPTEN_KEEPALIVE void wasm_set_envelope_end(int envType, int line, int point);
}
