#include "PresetManager.h"
#include "Parameters.h"
#include "ParameterIDs.h"
#include "../Core/VoiceManager.h"
#include "../DSP/Envelopes/ADSRtoStage.h"

namespace CZ101 {
namespace State {

static void sanitizePreset(Preset& p)
{
    // 1. Sanitize DCA 1
    float maxDcaLvl = 0.0f;
    for (int k = 0; k < 8; ++k) maxDcaLvl = std::max(maxDcaLvl, p.dcaEnv.levels[k]);
    if (maxDcaLvl < 0.05f) {
        float att = p.parameters.count("DCA_ATTACK") ? p.parameters["DCA_ATTACK"] : 0.01f;
        float dec = p.parameters.count("DCA_DECAY") ? p.parameters["DCA_DECAY"] : 0.3f;
        float sus = p.parameters.count("DCA_SUSTAIN") ? p.parameters["DCA_SUSTAIN"] : 0.8f;
        float rel = p.parameters.count("DCA_RELEASE") ? p.parameters["DCA_RELEASE"] : 0.3f;
        
        std::array<float, 8> outR, outL;
        int susPt = 2, endPt = 3;
        CZ101::DSP::ADSRtoStageConverter::convertADSR(att * 1000.0f, dec * 1000.0f, sus, rel * 1000.0f, outR, outL, susPt, endPt);
        for (int k = 0; k < 8; ++k) {
            p.dcaEnv.rates[k] = outR[k];
            p.dcaEnv.levels[k] = outL[k];
        }
        p.dcaEnv.sustainPoint = susPt;
        p.dcaEnv.endPoint = endPt;
    } else if (p.dcaEnv.sustainPoint > 0 && p.dcaEnv.sustainPoint < 8) {
        if (p.dcaEnv.levels[p.dcaEnv.sustainPoint] < 0.05f && p.dcaEnv.levels[p.dcaEnv.sustainPoint - 1] > 0.1f) {
            p.dcaEnv.sustainPoint--;
        }
    }

    // 2. Sanitize DCW 1
    float maxDcwLvl = 0.0f;
    for (int k = 0; k < 8; ++k) maxDcwLvl = std::max(maxDcwLvl, p.dcwEnv.levels[k]);
    if (maxDcwLvl < 0.05f) {
        float att = p.parameters.count("DCW_ATTACK") ? p.parameters["DCW_ATTACK"] : 0.01f;
        float dec = p.parameters.count("DCW_DECAY") ? p.parameters["DCW_DECAY"] : 0.3f;
        float sus = p.parameters.count("DCW_SUSTAIN") ? p.parameters["DCW_SUSTAIN"] : 0.7f;
        float rel = p.parameters.count("DCW_RELEASE") ? p.parameters["DCW_RELEASE"] : 0.3f;
        
        std::array<float, 8> outR, outL;
        int susPt = 2, endPt = 3;
        CZ101::DSP::ADSRtoStageConverter::convertADSR(att * 1000.0f, dec * 1000.0f, sus, rel * 1000.0f, outR, outL, susPt, endPt);
        for (int k = 0; k < 8; ++k) {
            p.dcwEnv.rates[k] = outR[k];
            p.dcwEnv.levels[k] = outL[k];
        }
        p.dcwEnv.sustainPoint = susPt;
        p.dcwEnv.endPoint = endPt;
    }

    // 3. Sanitize Pitch 1 & 2
    bool pitchFlat = true;
    for (int k = 0; k < 8; ++k) {
        if (p.pitchEnv.levels[k] > 0.001f) { pitchFlat = false; break; }
    }
    if (pitchFlat) {
        for (int k = 0; k < 8; ++k) {
            p.pitchEnv.levels[k] = 0.5f;
            p.pitchEnv.rates[k] = 0.99f;
        }
    }
    if (p.pitchEnv.endPoint < 0 || p.pitchEnv.endPoint >= 8) p.pitchEnv.endPoint = 0;
    if (p.pitchEnv.sustainPoint < 0 || p.pitchEnv.sustainPoint >= 8) p.pitchEnv.sustainPoint = 0;

    bool pitch2Flat = true;
    for (int k = 0; k < 8; ++k) {
        if (p.pitchEnv2.levels[k] > 0.001f) { pitch2Flat = false; break; }
    }
    if (pitch2Flat) {
        for (int k = 0; k < 8; ++k) {
            p.pitchEnv2.levels[k] = 0.5f;
            p.pitchEnv2.rates[k] = 0.99f;
        }
    }

    // 4. Line 2 DCA / DCW
    float maxDca2 = 0.0f;
    for (int k = 0; k < 8; ++k) maxDca2 = std::max(maxDca2, p.dcaEnv2.levels[k]);
    if (maxDca2 < 0.05f) p.dcaEnv2 = p.dcaEnv;

    float maxDcw2 = 0.0f;
    for (int k = 0; k < 8; ++k) maxDcw2 = std::max(maxDcw2, p.dcwEnv2.levels[k]);
    if (maxDcw2 < 0.05f) p.dcwEnv2 = p.dcwEnv;

    // 5. Sanitize Levels
    float osc1L = p.parameters.count("OSC1_LEVEL") ? p.parameters["OSC1_LEVEL"] : 1.0f;
    float osc2L = p.parameters.count("OSC2_LEVEL") ? p.parameters["OSC2_LEVEL"] : 0.0f;
    if (osc1L < 0.01f && osc2L < 0.01f) {
        p.parameters["OSC1_LEVEL"] = 1.0f;
    }
}

void PresetManager::autoSaveUserBank()
{
    juce::File defaultsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                                .getChildFile("CZ101Emulator");
                                
    if (!defaultsDir.exists()) 
        defaultsDir.createDirectory();
    
    saveBank(defaultsDir.getChildFile(USER_BANK_FILENAME));
}

void PresetManager::saveBank(const juce::File& file)
{
    const juce::ScopedReadLock sl(presetLock);
    juce::Array<juce::var> bankArray;
    
    for (const auto& preset : presets) {
        juce::DynamicObject::Ptr obj = new juce::DynamicObject();
        
        // Name & params (EXISTENTE)
        obj->setProperty("name", juce::String(preset.name));
        if (!preset.author.empty()) obj->setProperty("author", juce::String(preset.author));

        juce::DynamicObject::Ptr paramsObj = new juce::DynamicObject();
        for (const auto& [id, val] : preset.parameters) {
            paramsObj->setProperty(juce::Identifier(id), val);
        }
        obj->setProperty("params", juce::var(paramsObj.get()));
        
        // Helper to serialize Env (Audit Fix 5.1: Int Serialization x10000)
        auto serializeEnv = [&](const EnvelopeData& env, const juce::String& name) {
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
            obj->setProperty(name, juce::var(envObj.get()));
        };

        serializeEnv(preset.dcwEnv, "dcwEnv");
        serializeEnv(preset.dcaEnv, "dcaEnv");
        serializeEnv(preset.pitchEnv, "pitchEnv");
        
        // Serialize Line 2 Envelopes? If structure matches data member
        serializeEnv(preset.dcwEnv2, "dcwEnv2"); // User didn't ask but we should consistency? 
        // Wait, original code didn't save Env2?
        // Checking original saveBank...
        // Original code only saved `dcwEnv`, `dcaEnv`, `pitchEnv`?
        // Ah, original code (Step 4336) lines 641-683 ONLY Saved Line 1 Envelopes!
        // But Line 2 envelopes exist in Preset struct (env2).
        // If I change format, I should add them if they are used.
        // HOWEVER, fixing 5.1 implies *existing* logic. I will stick to existing + the fix.
        // Wait, if I don't save Line 2, dual line patches lose data?
        // This is a bug from before. I should fix it.
        serializeEnv(preset.dcaEnv2, "dcaEnv2");
        serializeEnv(preset.pitchEnv2, "pitchEnv2");
        
        bankArray.add(juce::var(obj.get()));
    }
    
    // Audit Fix 5.2: Wrap in Versioned Object
    juce::DynamicObject::Ptr root = new juce::DynamicObject();
    root->setProperty("version", 1);
    root->setProperty("presets", bankArray);
    
    juce::String jsonString = juce::JSON::toString(juce::var(root), true);
    if (!file.replaceWithText(jsonString))
    {
        juce::Logger::writeToLog("Error: Failed to save bank to " + file.getFullPathName());
    }
}

void PresetManager::savePresetToFile(int index, const juce::File& file)
{
    if (index < 0 || index >= (int)presets.size()) return;
    
    // Ensure data is fresh
    if (index == currentPresetIndex) copyStateFromProcessor();
    
    const auto& preset = presets[index];
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("name", juce::String(preset.name));
    obj->setProperty("author", juce::String(preset.author));
    
    juce::DynamicObject::Ptr paramsObj = new juce::DynamicObject();
    for (auto const& [key, val] : preset.parameters) {
        paramsObj->setProperty(juce::Identifier(key), val);
    }
    obj->setProperty("params", paramsObj.get());
    
    auto serializeEnv = [&](const EnvelopeData& env, const juce::String& propertyName) {
        juce::DynamicObject::Ptr envObj = new juce::DynamicObject();
        juce::Array<juce::var> rates, levels;
        for (int i = 0; i < 8; ++i) {
            rates.add(env.rates[i]);
            levels.add(env.levels[i]);
        }
        envObj->setProperty("rates", rates);
        envObj->setProperty("levels", levels);
        envObj->setProperty("sustainPoint", env.sustainPoint);
        envObj->setProperty("endPoint", env.endPoint);
        obj->setProperty(propertyName, envObj.get());
    };
    
    serializeEnv(preset.dcwEnv, "dcwEnv");
    serializeEnv(preset.dcaEnv, "dcaEnv");
    serializeEnv(preset.pitchEnv, "pitchEnv");
    serializeEnv(preset.dcwEnv2, "dcwEnv2");
    serializeEnv(preset.dcaEnv2, "dcaEnv2");
    serializeEnv(preset.pitchEnv2, "pitchEnv2");
    
    juce::DynamicObject::Ptr root = new juce::DynamicObject();
    root->setProperty("version", 1);
    root->setProperty("type", "single_patch");
    root->setProperty("preset", obj.get());
    
    juce::String jsonString = juce::JSON::toString(juce::var(root), true);
    file.replaceWithText(jsonString);
}

void PresetManager::loadPresetFromFile(const juce::File& file)
{
    if (!file.existsAsFile()) return;
    juce::var data = juce::JSON::parse(file);
    if (!data.isObject() || !data.hasProperty("preset")) return;
    
    int version = data["version"];
    const auto& presetVar = data["preset"];
    
    Preset p;
    p.name = presetVar["name"].toString().toStdString();
    p.author = presetVar["author"].toString().toStdString();
    
    if (auto* paramsObj = presetVar["params"].getDynamicObject()) {
        auto props = paramsObj->getProperties();
        for (auto& prop : props) {
            p.parameters[prop.name.toString().toUpperCase().toStdString()] = static_cast<float>(prop.value);
        }
    }
    
    // Upgrade Legacy Resonance Waveforms
    auto upgradeWaveform = [&](const std::string& waveKey, const std::string& windowKey) {
        if (p.parameters.count(waveKey)) {
            int w = static_cast<int>(p.parameters[waveKey]);
            if (w >= 5 && w <= 7) {
                p.parameters[waveKey] = 6.0f; // MULTI_SINE
                p.parameters[windowKey] = static_cast<float>(w - 4); // 1=SAW, 2=TRI, 3=TRAP
            }
        }
    };
    upgradeWaveform("OSC1_WAVEFORM", "OSC1_WINDOW");
    upgradeWaveform("OSC2_WAVEFORM", "OSC2_WINDOW");
    
    auto loadEnv = [&](const juce::var& envVar, EnvelopeData& env) {
        if (auto* obj = envVar.getDynamicObject()) {
            auto rates = obj->getProperty("rates");
            auto levels = obj->getProperty("levels");
            if (rates.isArray() && levels.isArray()) {
                for (int k=0; k<8; ++k) {
                    env.rates[k] = static_cast<float>(rates[k]);
                    env.levels[k] = static_cast<float>(levels[k]);
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
    
    sanitizePreset(p);
    loadPresetFromStruct(p);
}

void PresetManager::loadBank(const juce::File& file)
{
    if (!file.existsAsFile()) return;
    
    juce::Logger::writeToLog("PresetManager: Parsing JSON...");
    juce::var data = juce::JSON::parse(file);
    juce::Logger::writeToLog("PresetManager: JSON Parsed");
    
    juce::var presetsArray;
    int version = 0;

    if (data.isObject() && data.hasProperty("presets")) {
        version = data["version"];
        presetsArray = data["presets"];
    } else if (data.isArray()) {
        presetsArray = data; 
    } else {
        return;
    }

    if (!presetsArray.isArray()) return;
    
    std::vector<Preset> newPresets;
    juce::Logger::writeToLog("PresetManager: Loading " + juce::String(presetsArray.size()) + " presets");
    
    for (int i = 0; i < presetsArray.size(); ++i) {
        if (i >= 64) break;
        
        const auto& presetVar = presetsArray[i];
        if (presetVar.isObject()) {
            Preset p;
            p.name = presetVar["name"].toString().toStdString();
            if (presetVar.hasProperty("author"))
                p.author = presetVar["author"].toString().toStdString();
            
            // Params
            if (auto* paramsObj = presetVar["params"].getDynamicObject()) {
                auto props = paramsObj->getProperties();
                for (auto& prop : props) {
                    p.parameters[prop.name.toString().toUpperCase().toStdString()] = static_cast<float>(prop.value);
                }
            }
            
            // Upgrade Legacy Resonance Waveforms
            auto upgradeWaveform = [&](const std::string& waveKey, const std::string& windowKey) {
                if (p.parameters.count(waveKey)) {
                    int w = static_cast<int>(p.parameters[waveKey]);
                    if (w >= 5 && w <= 7) {
                        p.parameters[waveKey] = 6.0f; // MULTI_SINE
                        p.parameters[windowKey] = static_cast<float>(w - 4); // 1=SAW, 2=TRI, 3=TRAP
                    }
                }
            };
            upgradeWaveform("OSC1_WAVEFORM", "OSC1_WINDOW");
            upgradeWaveform("OSC2_WAVEFORM", "OSC2_WINDOW");
            
            // Helper to load 8-stage
            auto loadEnv = [&](const juce::var& envVar, EnvelopeData& env) {
                if (auto* obj = envVar.getDynamicObject()) {
                    auto rates = obj->getProperty("rates");
                    auto levels = obj->getProperty("levels");
                    
                    if (rates.isArray() && levels.isArray()) {
                        for (int k = 0; k < 8; ++k) {
                            float r = (k < rates.size()) ? static_cast<float>(rates[k]) : 0.5f;
                            float l = (k < levels.size()) ? static_cast<float>(levels[k]) : 0.0f;
                            
                            // 1. Scaled integers (x10000) e.g. 5000 -> 0.5, 10000 -> 1.0
                            if (r > 100.0f) r /= 10000.0f;
                            else if (r > 1.0f) r = std::min(r, 99.0f) / 99.0f; // 2. Hardware 0-99 scale
                            // 3. Otherwise r <= 1.0f is already normalized (0.0 to 1.0)

                            if (l > 100.0f) l /= 10000.0f;
                            else if (l > 1.0f) l = std::min(l, 99.0f) / 99.0f;
                            // Otherwise l <= 1.0f is already normalized

                            env.rates[k] = juce::jlimit(0.0f, 1.0f, r);
                            env.levels[k] = juce::jlimit(0.0f, 1.0f, l);
                        }
                    }
                    
                    if (obj->hasProperty("sustainPoint"))
                        env.sustainPoint = static_cast<int>(obj->getProperty("sustainPoint"));
                    else
                        env.sustainPoint = 2;

                    if (obj->hasProperty("endPoint"))
                        env.endPoint = static_cast<int>(obj->getProperty("endPoint"));
                    else
                        env.endPoint = 3;
                }
            };
            
            loadEnv(presetVar["dcwEnv"], p.dcwEnv);
            loadEnv(presetVar["dcaEnv"], p.dcaEnv);
            loadEnv(presetVar["pitchEnv"], p.pitchEnv);
            
            // Also load line 2 if present
            if (presetVar.hasProperty("dcwEnv2")) loadEnv(presetVar["dcwEnv2"], p.dcwEnv2);
            if (presetVar.hasProperty("dcaEnv2")) loadEnv(presetVar["dcaEnv2"], p.dcaEnv2);
            if (presetVar.hasProperty("pitchEnv2")) loadEnv(presetVar["pitchEnv2"], p.pitchEnv2);
            
            sanitizePreset(p);
            newPresets.push_back(p);
        }
    }
    
    while (newPresets.size() < 64) {
        Preset initP("Init User " + std::to_string(newPresets.size() + 1));
        sanitizePreset(initP);
        newPresets.push_back(initP);
    }
    
    {
        const juce::ScopedWriteLock swl(presetLock);
        presets = std::move(newPresets);
        currentPresetIndex = 0; 
    }
    
    juce::Logger::writeToLog("PresetManager: Bank applied, loading preset 0");
    loadPreset(currentPresetIndex);
}

std::unique_ptr<juce::XmlElement> PresetManager::exportEnvelopesToXml()
{
    auto root = std::make_unique<juce::XmlElement>("Envelopes");
    
    auto addEnv = [&](const EnvelopeData& env, const juce::String& type, int line) {
        auto* e = root->createNewChildElement("Envelope");
        e->setAttribute("type", type);
        e->setAttribute("line", line);
        
        juce::String rates, levels;
        for (int i=0; i<8; ++i) {
            rates += juce::String(env.rates[i], 4) + ",";
            levels += juce::String(env.levels[i], 4) + ",";
        }
        e->setAttribute("rates", rates.dropLastCharacters(1));
        e->setAttribute("levels", levels.dropLastCharacters(1));
        e->setAttribute("sustain", env.sustainPoint);
        e->setAttribute("end", env.endPoint);
    };

    addEnv(currentPreset.pitchEnv, "Pitch", 1);
    addEnv(currentPreset.dcwEnv, "DCW", 1);
    addEnv(currentPreset.dcaEnv, "DCA", 1);
    
    addEnv(currentPreset.pitchEnv2, "Pitch", 2);
    addEnv(currentPreset.dcwEnv2, "DCW", 2);
    addEnv(currentPreset.dcaEnv2, "DCA", 2);

    return root;
}

void PresetManager::importEnvelopesFromXml(const juce::XmlElement& xml)
{
    if (!xml.hasTagName("Envelopes")) return;
    
    for (auto* e : xml.getChildIterator())
    {
        if (e->hasTagName("Envelope"))
        {
            int line = e->getIntAttribute("line");
            juce::String type = e->getStringAttribute("type");
            
            EnvelopeData* target = nullptr;
            if (line == 1) {
                if (type == "Pitch") target = &currentPreset.pitchEnv;
                else if (type == "DCW") target = &currentPreset.dcwEnv;
                else if (type == "DCA") target = &currentPreset.dcaEnv;
            } else if (line == 2) {
                if (type == "Pitch") target = &currentPreset.pitchEnv2;
                else if (type == "DCW") target = &currentPreset.dcwEnv2;
                else if (type == "DCA") target = &currentPreset.dcaEnv2;
            }
            
            if (target)
            {
                juce::StringArray rates = juce::StringArray::fromTokens(e->getStringAttribute("rates"), ",", "");
                juce::StringArray levels = juce::StringArray::fromTokens(e->getStringAttribute("levels"), ",", "");
                
                for(int i=0; i<8; ++i) {
                    if (i < rates.size()) target->rates[i] = rates[i].getFloatValue();
                    if (i < levels.size()) target->levels[i] = levels[i].getFloatValue();
                }
                target->sustainPoint = e->getIntAttribute("sustain");
                target->endPoint = e->getIntAttribute("end");
            }
        }
    }
    
    // Apply immediately
    applyPresetToProcessor(currentPreset); // APVTS
    loadPresetFromStruct(currentPreset); // Full refresh
}


} // namespace State
} // namespace CZ101
