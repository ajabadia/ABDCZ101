// ─── WASM preset management ───
// Load/save/rename/delete/move presets plus the WasmPreset <-> core Preset
// conversion and the envelope init/apply helpers, extracted from the old
// monolithic WasmBridge.cpp. Bank JSON + SysEx import/export live in
// WasmBank.cpp; both share the gPresets vector + applyWasmPreset defined here.

#include "WasmState.h"
#include "../State/PresetManager.h"
#include "../State/FactoryPresets.h"
#include "../MIDI/SysExManager.h"
#include <algorithm>

void initWasmEnvelope(WasmEnvelope& env) {
    for (int i = 0; i < 8; ++i) {
        env.rates[i] = 0.5f;
        env.levels[i] = 1.0f;
    }
    env.sustainPoint = 2;
    env.endPoint = 3;
}

void initWasmPitchEnvelope(WasmEnvelope& env) {
    for (int i = 0; i < 8; ++i) {
        env.rates[i] = 0.99f;
        env.levels[i] = 0.5f; // Unison / 0 offset
    }
    env.sustainPoint = 0;
    env.endPoint = 0;
}

WasmPreset convertToWasmPreset(const CZ101::State::Preset& src) {
    WasmPreset dest;
    dest.name = src.name;
    dest.parameters = src.parameters;

    auto copyEnv = [](const CZ101::State::EnvelopeData& srcEnv, WasmEnvelope& destEnv) {
        for (int i = 0; i < 8; ++i) {
            destEnv.rates[i] = srcEnv.rates[i];
            destEnv.levels[i] = srcEnv.levels[i];
        }
        destEnv.sustainPoint = srcEnv.sustainPoint;
        destEnv.endPoint = srcEnv.endPoint;
    };

    copyEnv(src.pitchEnv, dest.pitchEnv);
    copyEnv(src.dcwEnv, dest.dcwEnv);
    copyEnv(src.dcaEnv, dest.dcaEnv);
    copyEnv(src.pitchEnv2, dest.pitchEnv2);
    copyEnv(src.dcwEnv2, dest.dcwEnv2);
    copyEnv(src.dcaEnv2, dest.dcaEnv2);
    return dest;
}

void initWasmPresets() {
    gPresets.clear();
    gPresets.reserve(CZ101::State::FACTORY_PRESET_COUNT);
    for (int i = 0; i < CZ101::State::FACTORY_PRESET_COUNT; ++i) {
        const uint8_t* data = CZ101::State::getFactoryPresetData(i);
        if (data != nullptr) {
            CZ101::State::Preset p;
            if (CZ101::State::FACTORY_PRESET_NAMES[i] != nullptr)
                p.name = CZ101::State::FACTORY_PRESET_NAMES[i];
            else
                p.name = "Preset " + std::to_string(i + 1);
            p.author = "Casio";
            
            CZ101::MIDI::SysExManager::decodePatch(data, p);
            
            if (CZ101::State::FACTORY_PRESET_NAMES[i] != nullptr)
                p.name = CZ101::State::FACTORY_PRESET_NAMES[i];
                
            gPresets.push_back(convertToWasmPreset(p));
        } else {
            WasmPreset init;
            init.name = "Init " + std::to_string(i + 1);
            initWasmEnvelope(init.dcwEnv);
            initWasmEnvelope(init.dcaEnv);
            initWasmPitchEnvelope(init.pitchEnv);
            initWasmEnvelope(init.dcwEnv2);
            initWasmEnvelope(init.dcaEnv2);
            initWasmPitchEnvelope(init.pitchEnv2);
            gPresets.push_back(init);
        }
    }
    if (!gPresets.empty()) {
        gCurrentPresetIndex = 0;
        applyWasmPreset(gPresets[0]);
    }
}

void applyWasmPreset(const WasmPreset& preset) {
    // 1. Set parameter values
    for (const auto& pair : preset.parameters) {
        float normalized = pair.second;
        const auto* spec = CZ101::ParameterRegistry::findSpec(pair.first);
        if (spec && (spec->maxValue > spec->minValue)) {
            normalized = normalizedFromRaw(spec, pair.second);
        }
        wasm_set_param(pair.first.c_str(), normalized);
    }

    // Ensure filters are fully open by default for classic presets/imports
    if (preset.parameters.find("MODERN_LPF_CUTOFF") == preset.parameters.end()) {
        wasm_set_param("MODERN_LPF_CUTOFF", 1.0f); // 20000 Hz (fully open)
    }
    if (preset.parameters.find("MODERN_LPF_RESO") == preset.parameters.end()) {
        wasm_set_param("MODERN_LPF_RESO", 0.0f); // 0.0 (no resonance)
    }
    if (preset.parameters.find("MODERN_HPF_CUTOFF") == preset.parameters.end()) {
        wasm_set_param("MODERN_HPF_CUTOFF", 0.0f); // 20 Hz (fully open)
    }

    // 2. Set 8-stage envelopes
    auto applyEnv = [](const WasmEnvelope& env, int type, int line) {
        for (int i = 0; i < 8; ++i) {
            if (type == 0) gVoiceManager.setPitchStage(line, i, env.rates[i], env.levels[i]);
            else if (type == 1) gVoiceManager.setDCWStage(line, i, env.rates[i], env.levels[i]);
            else if (type == 2) gVoiceManager.setDCAStage(line, i, env.rates[i], env.levels[i]);
        }
        if (type == 0) {
            gVoiceManager.setPitchSustainPoint(line, env.sustainPoint);
            gVoiceManager.setPitchEndPoint(line, env.endPoint);
        } else if (type == 1) {
            gVoiceManager.setDCWSustainPoint(line, env.sustainPoint);
            gVoiceManager.setDCWEndPoint(line, env.endPoint);
        } else if (type == 2) {
            gVoiceManager.setDCASustainPoint(line, env.sustainPoint);
            gVoiceManager.setDCAEndPoint(line, env.endPoint);
        }
    };

    applyEnv(preset.pitchEnv, 0, 1);
    applyEnv(preset.dcwEnv, 1, 1);
    applyEnv(preset.dcaEnv, 2, 1);
    applyEnv(preset.pitchEnv2, 0, 2);
    applyEnv(preset.dcwEnv2, 1, 2);
    applyEnv(preset.dcaEnv2, 2, 2);
}

extern "C" {

EMSCRIPTEN_KEEPALIVE void wasm_load_factory_bank() {
    if (!gInitialized) return;
    initWasmPresets();
}

EMSCRIPTEN_KEEPALIVE void wasm_load_preset(int index) {
    if (!gInitialized || index < 0 || index >= (int)gPresets.size()) return;
    gCurrentPresetIndex = index;
    applyWasmPreset(gPresets[index]);
}

EMSCRIPTEN_KEEPALIVE void wasm_save_preset(int index, const char* name) {
    if (!gInitialized || index < 0 || index >= (int)gPresets.size() || !name) return;
    WasmPreset& p = gPresets[index];
    p.name = name;

    // Snapshot parameters
    for (const auto& pair : gWasmParams) {
        const auto* spec = CZ101::ParameterRegistry::findSpec(pair.first);
        const float rawVal = rawFromNormalized(spec, pair.second);
        p.parameters[pair.first] = rawVal;
    }

    // Snapshot envelopes
    auto snapshotEnv = [](WasmEnvelope& env, int type, int line) {
        for (int i = 0; i < 8; ++i) {
            float r = 0.0f, l = 0.0f;
            if (type == 0) gVoiceManager.getPitchStage(line, i, r, l);
            else if (type == 1) gVoiceManager.getDCWStage(line, i, r, l);
            else if (type == 2) gVoiceManager.getDCAStage(line, i, r, l);
            env.rates[i] = r;
            env.levels[i] = l;
        }
        if (type == 0) {
            env.sustainPoint = gVoiceManager.getPitchSustainPoint(line);
            env.endPoint = gVoiceManager.getPitchEndPoint(line);
        } else if (type == 1) {
            env.sustainPoint = gVoiceManager.getDCWSustainPoint(line);
            env.endPoint = gVoiceManager.getDCWEndPoint(line);
        } else if (type == 2) {
            env.sustainPoint = gVoiceManager.getDCASustainPoint(line);
            env.endPoint = gVoiceManager.getDCAEndPoint(line);
        }
    };

    snapshotEnv(p.pitchEnv, 0, 1);
    snapshotEnv(p.dcwEnv, 1, 1);
    snapshotEnv(p.dcaEnv, 2, 1);
    snapshotEnv(p.pitchEnv2, 0, 2);
    snapshotEnv(p.dcwEnv2, 1, 2);
    snapshotEnv(p.dcaEnv2, 2, 2);
}

// ─── Bank Manager preset manipulation (rename / delete / move) ───
// The WebUI drives these from the Bank Manager context menu (right-click).

EMSCRIPTEN_KEEPALIVE void wasm_rename_preset(int index, const char* name) {
    if (!gInitialized || index < 0 || index >= (int)gPresets.size() || !name) return;
    gPresets[index].name = name;
}

EMSCRIPTEN_KEEPALIVE void wasm_delete_preset(int index) {
    if (!gInitialized || index < 0 || index >= (int)gPresets.size()) return;
    gPresets.erase(gPresets.begin() + index);
    if (gPresets.empty()) {
        WasmPreset init;
        init.name = "Init";
        initWasmPitchEnvelope(init.pitchEnv);
        initWasmEnvelope(init.dcwEnv);
        initWasmEnvelope(init.dcaEnv);
        initWasmPitchEnvelope(init.pitchEnv2);
        initWasmEnvelope(init.dcwEnv2);
        initWasmEnvelope(init.dcaEnv2);
        gPresets.push_back(init);
    }
    if (gCurrentPresetIndex >= (int)gPresets.size())
        gCurrentPresetIndex = (int)gPresets.size() - 1;
}

EMSCRIPTEN_KEEPALIVE void wasm_move_preset(int fromIndex, int toIndex) {
    if (!gInitialized) return;
    int size = (int)gPresets.size();
    if (fromIndex < 0 || fromIndex >= size || toIndex < 0 || toIndex >= size) return;
    if (fromIndex == toIndex) return;

    auto p = gPresets[fromIndex];
    gPresets.erase(gPresets.begin() + fromIndex);
    gPresets.insert(gPresets.begin() + toIndex, p);

    // Track the active preset across the move, same as PresetManager::movePreset.
    if (gCurrentPresetIndex == fromIndex) {
        gCurrentPresetIndex = toIndex;
    } else if (fromIndex < gCurrentPresetIndex && toIndex >= gCurrentPresetIndex) {
        gCurrentPresetIndex--;
    } else if (fromIndex > gCurrentPresetIndex && toIndex <= gCurrentPresetIndex) {
        gCurrentPresetIndex++;
    }
}

EMSCRIPTEN_KEEPALIVE int wasm_get_preset_count() {
    if (!gInitialized) return 0;
    return (int)gPresets.size();
}

EMSCRIPTEN_KEEPALIVE void wasm_get_preset_name(int index, char* outName, int maxLen) {
    if (!gInitialized || index < 0 || index >= (int)gPresets.size() || !outName || maxLen <= 0) return;
    std::strncpy(outName, gPresets[index].name.c_str(), maxLen - 1);
    outName[maxLen - 1] = '\0';
}

EMSCRIPTEN_KEEPALIVE int wasm_get_current_preset_index() {
    if (!gInitialized) return 0;
    return gCurrentPresetIndex;
}

} // extern "C"
