// ─── WASM envelope accessors ───
// The wasm_get/set_envelope_* entry points the WebUI envelope editors call,
// extracted from the old monolithic WasmBridge.cpp. All access is routed
// through VoiceManager, exactly as before.

#include "WasmState.h"

extern "C" {

EMSCRIPTEN_KEEPALIVE float wasm_get_envelope_rate(int envType, int line, int stage) {
    if (!gInitialized || stage < 0 || stage >= 8) return 0.0f;
    float r = 0.0f, l = 0.0f;
    if (envType == 0) gVoiceManager.getPitchStage(line, stage, r, l);
    else if (envType == 1) gVoiceManager.getDCWStage(line, stage, r, l);
    else if (envType == 2) gVoiceManager.getDCAStage(line, stage, r, l);
    return r;
}

EMSCRIPTEN_KEEPALIVE float wasm_get_envelope_level(int envType, int line, int stage) {
    if (!gInitialized || stage < 0 || stage >= 8) return 0.0f;
    float r = 0.0f, l = 0.0f;
    if (envType == 0) gVoiceManager.getPitchStage(line, stage, r, l);
    else if (envType == 1) gVoiceManager.getDCWStage(line, stage, r, l);
    else if (envType == 2) gVoiceManager.getDCAStage(line, stage, r, l);
    return l;
}

EMSCRIPTEN_KEEPALIVE int wasm_get_envelope_sustain(int envType, int line) {
    if (!gInitialized) return -1;
    if (envType == 0) return gVoiceManager.getPitchSustainPoint(line);
    if (envType == 1) return gVoiceManager.getDCWSustainPoint(line);
    if (envType == 2) return gVoiceManager.getDCASustainPoint(line);
    return -1;
}

EMSCRIPTEN_KEEPALIVE int wasm_get_envelope_end(int envType, int line) {
    if (!gInitialized) return -1;
    if (envType == 0) return gVoiceManager.getPitchEndPoint(line);
    if (envType == 1) return gVoiceManager.getDCWEndPoint(line);
    if (envType == 2) return gVoiceManager.getDCAEndPoint(line);
    return -1;
}

EMSCRIPTEN_KEEPALIVE void wasm_set_envelope_stage(int envType, int line, int stage, float rate, float level) {
    if (!gInitialized || stage < 0 || stage >= 8) return;
    if (envType == 0) gVoiceManager.setPitchStage(line, stage, rate, level);
    else if (envType == 1) gVoiceManager.setDCWStage(line, stage, rate, level);
    else if (envType == 2) gVoiceManager.setDCAStage(line, stage, rate, level);
}

EMSCRIPTEN_KEEPALIVE void wasm_set_envelope_sustain(int envType, int line, int point) {
    if (!gInitialized) return;
    if (envType == 0) gVoiceManager.setPitchSustainPoint(line, point);
    else if (envType == 1) gVoiceManager.setDCWSustainPoint(line, point);
    else if (envType == 2) gVoiceManager.setDCASustainPoint(line, point);
}

EMSCRIPTEN_KEEPALIVE void wasm_set_envelope_end(int envType, int line, int point) {
    if (!gInitialized) return;
    if (envType == 0) gVoiceManager.setPitchEndPoint(line, point);
    else if (envType == 1) gVoiceManager.setDCWEndPoint(line, point);
    else if (envType == 2) gVoiceManager.setDCAEndPoint(line, point);
}

} // extern "C"
