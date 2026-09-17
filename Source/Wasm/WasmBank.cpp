// ─── WASM bank + SysEx management ───
// wasm_load_bank / wasm_save_bank (JSON) and wasm_load_sysex /
// wasm_save_sysex (CZ-101 patch dumps), extracted from the old monolithic
// WasmBridge.cpp. Preset CRUD and the WasmPreset helpers live in
// WasmPresets.cpp; this module converts between the core State::Preset shape
// (SysEx path) and the flat WasmPreset shape (bank path).

#include "WasmState.h"
#include "../State/PresetManager.h"
#include "../MIDI/SysExManager.h"
#include <algorithm>

namespace {



CZ101::State::Preset getActivePreset(const char* name) {
    CZ101::State::Preset p;
    p.name = name ? name : "Active Patch";

    // Write parameters
    for (const auto& pair : gWasmParams) {
        const auto* spec = CZ101::ParameterRegistry::findSpec(pair.first);
        const float rawVal = rawFromNormalized(spec, pair.second);
        p.parameters[pair.first] = rawVal;
    }

    // Read envelope values from gVoiceManager
    auto readEnv = [](CZ101::State::EnvelopeData& env, int type, int line) {
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

    readEnv(p.pitchEnv, 0, 1);
    readEnv(p.dcwEnv, 1, 1);
    readEnv(p.dcaEnv, 2, 1);
    readEnv(p.pitchEnv2, 0, 2);
    readEnv(p.dcwEnv2, 1, 2);
    readEnv(p.dcaEnv2, 2, 2);

    return p;
}

} // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE int wasm_load_sysex(const uint8_t* data, int length) {
    if (!gInitialized || !data || length < 264) return 0;

    CZ101::MIDI::SysExManager sysex;
    sysex.setProtectionState(false, true);
    std::vector<CZ101::State::Preset> parsedPresets;

    sysex.onPresetParsed = [&](const CZ101::State::Preset& p) {
        parsedPresets.push_back(p);
    };

    sysex.handleSysEx(data, length, "Imported Preset");

    if (parsedPresets.empty()) return 0;

    if (parsedPresets.size() == 1) {
        applyWasmPreset(convertToWasmPreset(parsedPresets[0]));
        return 1;
    } else {
        int presetsToLoad = std::min((int)parsedPresets.size(), (int)gPresets.size());
        for (int i = 0; i < presetsToLoad; ++i) {
            gPresets[i] = convertToWasmPreset(parsedPresets[i]);
        }
        applyWasmPreset(gPresets[0]);
        gCurrentPresetIndex = 0;
        return presetsToLoad;
    }
}

EMSCRIPTEN_KEEPALIVE int wasm_save_sysex(uint8_t* outBuf, int maxLen, const char* patchName) {
    if (!gInitialized || !outBuf || maxLen < 264) return 0;

    CZ101::State::Preset p = getActivePreset(patchName);
    CZ101::MIDI::SysExManager sysex;
    juce::MemoryBlock block = sysex.createPatchDump(p);

    int bytesToCopy = std::min(maxLen, (int)block.getSize());
    std::memcpy(outBuf, block.getData(), bytesToCopy);
    return bytesToCopy;
}

EMSCRIPTEN_KEEPALIVE void wasm_load_bank(const char* jsonStr) {
    if (!gInitialized || !jsonStr) return;

    juce::var data = juce::JSON::parse(jsonStr);
    juce::var presetsArray;
    if (data.isObject() && data.hasProperty("presets")) {
        presetsArray = data["presets"];
    } else if (data.isArray()) {
        presetsArray = data;
    } else {
        return;
    }

    if (!presetsArray.isArray()) return;

    std::vector<WasmPreset> newPresets;
    for (int i = 0; i < presetsArray.size(); ++i) {
        if (i >= 64) break;
        const auto& presetVar = presetsArray[i];
        if (presetVar.isObject()) {
            WasmPreset p;
            p.name = presetVar["name"].toString().toStdString();

            if (auto* paramsObj = presetVar["params"].getDynamicObject()) {
                auto props = paramsObj->getProperties();
                for (auto& prop : props) {
                    p.parameters[prop.name.toString().toStdString()] = static_cast<float>(prop.value);
                }
            }

            auto loadEnv = [&](const juce::var& envVar, WasmEnvelope& env) {
                if (auto* obj = envVar.getDynamicObject()) {
                    auto rates = obj->getProperty("rates");
                    auto levels = obj->getProperty("levels");
                    if (rates.isArray() && levels.isArray()) {
                        for (int k = 0; k < 8; ++k) {
                            env.rates[k] = static_cast<float>(rates[k]) / 10000.0f;
                            env.levels[k] = static_cast<float>(levels[k]) / 10000.0f;
                        }
                    }
                    env.sustainPoint = static_cast<int>(obj->getProperty("sustainPoint"));
                    env.endPoint = static_cast<int>(obj->getProperty("endPoint"));
                }
            };

            loadEnv(presetVar["dcwEnv"], p.dcwEnv);
            loadEnv(presetVar["dcaEnv"], p.dcaEnv);
            loadEnv(presetVar["pitchEnv"], p.pitchEnv);
            loadEnv(presetVar["dcwEnv2"], p.dcwEnv2);
            loadEnv(presetVar["dcaEnv2"], p.dcaEnv2);
            loadEnv(presetVar["pitchEnv2"], p.pitchEnv2);

            newPresets.push_back(p);
        }
    }

    while (newPresets.size() < 64) {
        WasmPreset initPreset;
        initPreset.name = "Init User " + std::to_string(newPresets.size() + 1);
        initWasmEnvelope(initPreset.dcwEnv);
        initWasmEnvelope(initPreset.dcaEnv);
        initWasmEnvelope(initPreset.pitchEnv);
        initWasmEnvelope(initPreset.dcwEnv2);
        initWasmEnvelope(initPreset.dcaEnv2);
        initWasmEnvelope(initPreset.pitchEnv2);
        newPresets.push_back(initPreset);
    }

    gPresets = std::move(newPresets);
    gCurrentPresetIndex = 0;
    applyWasmPreset(gPresets[0]);
}

EMSCRIPTEN_KEEPALIVE int wasm_save_bank(char* outJson, int maxLen) {
    if (!gInitialized || !outJson || maxLen <= 0) return 0;

    juce::DynamicObject::Ptr rootObj = new juce::DynamicObject();
    rootObj->setProperty("version", 1);

    juce::Array<juce::var> presetsArray;
    for (const auto& preset : gPresets) {
        juce::DynamicObject::Ptr obj = new juce::DynamicObject();
        obj->setProperty("name", juce::String(preset.name));

        juce::DynamicObject::Ptr paramsObj = new juce::DynamicObject();
        for (const auto& [id, val] : preset.parameters) {
            paramsObj->setProperty(juce::Identifier(id), val);
        }
        obj->setProperty("params", juce::var(paramsObj.get()));

        auto serializeEnv = [&](const WasmEnvelope& env) -> juce::var {
            juce::DynamicObject::Ptr envObj = new juce::DynamicObject();
            juce::Array<juce::var> ratesArray, levelsArray;
            for (int i = 0; i < 8; ++i) {
                ratesArray.add(static_cast<int>(env.rates[i] * 10000.0f));
                levelsArray.add(static_cast<int>(env.levels[i] * 10000.0f));
            }
            envObj->setProperty("rates", ratesArray);
            envObj->setProperty("levels", levelsArray);
            envObj->setProperty("sustainPoint", env.sustainPoint);
            envObj->setProperty("endPoint", env.endPoint);
            return juce::var(envObj.get());
        };

        obj->setProperty("dcwEnv", serializeEnv(preset.dcwEnv));
        obj->setProperty("dcaEnv", serializeEnv(preset.dcaEnv));
        obj->setProperty("pitchEnv", serializeEnv(preset.pitchEnv));
        obj->setProperty("dcwEnv2", serializeEnv(preset.dcwEnv2));
        obj->setProperty("dcaEnv2", serializeEnv(preset.dcaEnv2));
        obj->setProperty("pitchEnv2", serializeEnv(preset.pitchEnv2));

        presetsArray.add(juce::var(obj.get()));
    }
    rootObj->setProperty("presets", presetsArray);

    juce::String json = juce::JSON::toString(juce::var(rootObj.get()), true);
    std::strncpy(outJson, json.toRawUTF8(), maxLen - 1);
    outJson[maxLen - 1] = '\0';
    return (int)json.length();
}

} // extern "C"
