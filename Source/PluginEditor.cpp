#include "PluginEditor.h"
#include "PluginEditor_ResourceProvider.h"

CZ101AudioProcessorEditor::CZ101AudioProcessorEditor(CZ101AudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setupWebBrowserBindings();

    if (webComponent != nullptr)
        addAndMakeVisible(webComponent.get());

    // Register as Preset Listener
    audioProcessor.getPresetManager().addListener(this);

    // Register as APVTS Parameter Listener for all parameters
    for (auto* param : audioProcessor.AudioProcessor::getParameters())
    {
        if (auto* paramWithID = dynamic_cast<juce::AudioProcessorParameterWithID*>(param))
        {
            audioProcessor.getParameters().getAPVTS().addParameterListener(paramWithID->paramID, this);
        }
    }

    // Standard sizing for the web interface
    setSize(1409, 768); 
    setResizable(true, true);
    
    startTimerHz(15);
}

CZ101AudioProcessorEditor::~CZ101AudioProcessorEditor()
{
    stopTimer();
    audioProcessor.getPresetManager().removeListener(this);

    for (auto* param : audioProcessor.AudioProcessor::getParameters())
    {
        if (auto* paramWithID = dynamic_cast<juce::AudioProcessorParameterWithID*>(param))
        {
            audioProcessor.getParameters().getAPVTS().removeParameterListener(paramWithID->paramID, this);
        }
    }
}

void CZ101AudioProcessorEditor::setupWebBrowserBindings()
{
    juce::WebBrowserComponent::Options options;
    
    options = options
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withNativeIntegrationEnabled()
        .withResourceProvider(pluginResourceProvider)
        .withNativeFunction("setParameterValue", [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion) {
            if (args.size() >= 2 && args[0].isString())
            {
                auto paramID = args[0].toString();
                auto value = static_cast<float>(args[1]);
                juce::Logger::writeToLog("Native setParameterValue: " + paramID + " = " + juce::String(value));
                
                isUpdatingFromWeb = true;
                if (auto* param = audioProcessor.getParameters().getAPVTS().getParameter(paramID))
                {
                    // value is normalized 0.0 to 1.0 from WebUI
                    param->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, value));
                }
                isUpdatingFromWeb = false;
            }
            completion(juce::var());
        })
        .withNativeFunction("getParameterValue", [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion) {
            if (args.size() >= 1 && args[0].isString())
            {
                auto paramID = args[0].toString();
                if (auto* param = audioProcessor.getParameters().getAPVTS().getParameter(paramID))
                {
                    completion(param->getValue()); // Normalized 0..1
                    return;
                }
            }
            completion(0.0f);
        })
        .withNativeFunction("loadPreset", [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion) {
            if (args.size() >= 1 && (args[0].isInt() || args[0].isInt64() || args[0].isDouble()))
            {
                int idx = static_cast<int>(args[0]);
                juce::Logger::writeToLog("Native loadPreset: " + juce::String(idx));
                audioProcessor.getPresetManager().loadPreset(idx);
            }
            completion(juce::var());
        })
        .withNativeFunction("sendMidiMessage", [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion) {
            uint8_t b0 = 0, b1 = 0, b2 = 0;
            bool valid = false;
            if (args.size() >= 1 && args[0].isArray()) {
                auto array = args[0].getArray();
                if (array && array->size() >= 3) {
                    b0 = static_cast<uint8_t>(int((*array)[0]));
                    b1 = static_cast<uint8_t>(int((*array)[1]));
                    b2 = static_cast<uint8_t>(int((*array)[2]));
                    valid = true;
                }
            } else if (args.size() >= 3) {
                b0 = static_cast<uint8_t>(int(args[0]));
                b1 = static_cast<uint8_t>(int(args[1]));
                b2 = static_cast<uint8_t>(int(args[2]));
                valid = true;
            }
            if (valid) {
                juce::Logger::writeToLog("Native sendMidiMessage: 0x" + juce::String::toHexString(b0) + " " + juce::String(b1) + " " + juce::String(b2));
                audioProcessor.addUIMidiMessage(juce::MidiMessage(b0, b1, b2));
            }
            completion(juce::var());
        })
        .withNativeFunction("setEnvelopeStage", [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion) {
            if (args.size() >= 5)
            {
                int envType = static_cast<int>(args[0]);
                int line = static_cast<int>(args[1]);
                int stage = static_cast<int>(args[2]);
                float rate = static_cast<float>(args[3]);
                float level = static_cast<float>(args[4]);
                
                EnvelopeUpdateCommand::Type cmdType = EnvelopeUpdateCommand::DCA_STAGE;
                if (envType == 0) cmdType = EnvelopeUpdateCommand::PITCH_STAGE;
                else if (envType == 1) cmdType = EnvelopeUpdateCommand::DCW_STAGE;
                
                audioProcessor.scheduleEnvelopeUpdate({ cmdType, line, stage, rate, level });
            }
            completion(juce::var());
        })
        .withNativeFunction("setEnvelopeSustain", [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion) {
            if (args.size() >= 3)
            {
                int envType = static_cast<int>(args[0]);
                int line = static_cast<int>(args[1]);
                int point = static_cast<int>(args[2]);
                
                EnvelopeUpdateCommand::Type cmdType = EnvelopeUpdateCommand::DCA_SUSTAIN;
                if (envType == 0) cmdType = EnvelopeUpdateCommand::PITCH_SUSTAIN;
                else if (envType == 1) cmdType = EnvelopeUpdateCommand::DCW_SUSTAIN;
                
                audioProcessor.scheduleEnvelopeUpdate({ cmdType, line, point, 0.0f, 0.0f });
            }
            completion(juce::var());
        })
        .withNativeFunction("setEnvelopeEnd", [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion) {
            if (args.size() >= 3)
            {
                int envType = static_cast<int>(args[0]);
                int line = static_cast<int>(args[1]);
                int point = static_cast<int>(args[2]);
                
                EnvelopeUpdateCommand::Type cmdType = EnvelopeUpdateCommand::DCA_END;
                if (envType == 0) cmdType = EnvelopeUpdateCommand::PITCH_END;
                else if (envType == 1) cmdType = EnvelopeUpdateCommand::DCW_END;
                
                audioProcessor.scheduleEnvelopeUpdate({ cmdType, line, point, 0.0f, 0.0f });
            }
            completion(juce::var());
        })
        .withNativeFunction("jsLog", [](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion) {
            if (args.size() > 0)
                juce::Logger::writeToLog("[JS LOG] " + args[0].toString());
            completion(juce::var());
        })
        .withEventListener("sendMidiMessage", [this](const juce::var& data) {
            uint8_t b0 = 0, b1 = 0, b2 = 0;
            if (data.isArray()) {
                auto* arr = data.getArray();
                if (arr && arr->size() >= 3) {
                    b0 = static_cast<uint8_t>(int((*arr)[0]));
                    b1 = static_cast<uint8_t>(int((*arr)[1]));
                    b2 = static_cast<uint8_t>(int((*arr)[2]));
                    juce::Logger::writeToLog("Native event sendMidiMessage: 0x" + juce::String::toHexString(b0) + " " + juce::String(b1) + " " + juce::String(b2));
                    audioProcessor.addUIMidiMessage(juce::MidiMessage(b0, b1, b2));
                }
            }
        })
        .withEventListener("loadPreset", [this](const juce::var& data) {
            if (data.isInt() || data.isInt64() || data.isDouble()) {
                int idx = static_cast<int>(data);
                juce::Logger::writeToLog("Native event loadPreset: " + juce::String(idx));
                audioProcessor.getPresetManager().loadPreset(idx);
            }
        })
        .withEventListener("setParameterValue", [this](const juce::var& data) {
            if (data.isObject()) {
                auto* obj = data.getDynamicObject();
                if (obj && obj->hasProperty("id") && obj->hasProperty("value")) {
                    auto paramID = obj->getProperty("id").toString();
                    auto value = static_cast<float>(obj->getProperty("value"));
                    isUpdatingFromWeb = true;
                    if (auto* param = audioProcessor.getParameters().getAPVTS().getParameter(paramID))
                        param->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, value));
                    isUpdatingFromWeb = false;
                }
            }
        })
        .withEventListener("setEnvelopeStage", [this](const juce::var& data) {
            if (data.isObject()) {
                auto* obj = data.getDynamicObject();
                if (obj && obj->hasProperty("envType") && obj->hasProperty("line") && obj->hasProperty("stage")) {
                    int envType = static_cast<int>(obj->getProperty("envType"));
                    int line = static_cast<int>(obj->getProperty("line"));
                    int stage = static_cast<int>(obj->getProperty("stage"));
                    float rate = static_cast<float>(obj->getProperty("rate"));
                    float level = static_cast<float>(obj->getProperty("level"));
                    
                    EnvelopeUpdateCommand::Type cmdType = EnvelopeUpdateCommand::DCA_STAGE;
                    if (envType == 0) cmdType = EnvelopeUpdateCommand::PITCH_STAGE;
                    else if (envType == 1) cmdType = EnvelopeUpdateCommand::DCW_STAGE;
                    
                    audioProcessor.scheduleEnvelopeUpdate({ cmdType, line, stage, rate, level });
                }
            }
        })
        .withEventListener("setEnvelopeSustain", [this](const juce::var& data) {
            if (data.isObject()) {
                auto* obj = data.getDynamicObject();
                if (obj && obj->hasProperty("envType") && obj->hasProperty("line") && obj->hasProperty("point")) {
                    int envType = static_cast<int>(obj->getProperty("envType"));
                    int line = static_cast<int>(obj->getProperty("line"));
                    int point = static_cast<int>(obj->getProperty("point"));
                    
                    EnvelopeUpdateCommand::Type cmdType = EnvelopeUpdateCommand::DCA_SUSTAIN;
                    if (envType == 0) cmdType = EnvelopeUpdateCommand::PITCH_SUSTAIN;
                    else if (envType == 1) cmdType = EnvelopeUpdateCommand::DCW_SUSTAIN;
                    
                    audioProcessor.scheduleEnvelopeUpdate({ cmdType, line, point, 0.0f, 0.0f });
                }
            }
        })
        .withEventListener("setEnvelopeEnd", [this](const juce::var& data) {
            if (data.isObject()) {
                auto* obj = data.getDynamicObject();
                if (obj && obj->hasProperty("envType") && obj->hasProperty("line") && obj->hasProperty("point")) {
                    int envType = static_cast<int>(obj->getProperty("envType"));
                    int line = static_cast<int>(obj->getProperty("line"));
                    int point = static_cast<int>(obj->getProperty("point"));
                    
                    EnvelopeUpdateCommand::Type cmdType = EnvelopeUpdateCommand::DCA_END;
                    if (envType == 0) cmdType = EnvelopeUpdateCommand::PITCH_END;
                    else if (envType == 1) cmdType = EnvelopeUpdateCommand::DCW_END;
                    
                    audioProcessor.scheduleEnvelopeUpdate({ cmdType, line, point, 0.0f, 0.0f });
                }
            }
        })
        .withEventListener("requestSysex", [this](const juce::var&) {
            const juce::ScopedReadLock srl(audioProcessor.getPresetManager().getLock());
            const auto& preset = audioProcessor.getPresetManager().getCurrentPreset();
            auto sysexBlock = audioProcessor.getSysExManager().createPatchDump(preset);
            juce::String hexStr;
            const uint8_t* bytes = static_cast<const uint8_t*>(sysexBlock.getData());
            for (size_t i = 0; i < sysexBlock.getSize(); ++i)
            {
                if (i > 0) hexStr << " ";
                hexStr << juce::String::toHexString((int)bytes[i]).toUpperCase().paddedLeft('0', 2);
            }
            if (webComponent != nullptr)
                webComponent->emitEventIfBrowserIsVisible("sysexDump", hexStr);
        })
        .withEventListener("jsLog", [](const juce::var& data) {
            juce::Logger::writeToLog("[JS EVENT LOG] " + data.toString());
        });
        
    webComponent = std::make_unique<juce::WebBrowserComponent>(options);
    
    // Load via the custom ResourceProvider scheme
    webComponent->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
}

void CZ101AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void CZ101AudioProcessorEditor::resized()
{
    if (webComponent != nullptr)
        webComponent->setBounds(getLocalBounds());
}

bool CZ101AudioProcessorEditor::keyPressed(const juce::KeyPress& key)
{
    return false;
}

void CZ101AudioProcessorEditor::parameterChanged(const juce::String& parameterID, float /*newValue*/)
{
    if (isUpdatingFromWeb) return;
    
    // Broadcast back to web if it changes from host/MIDI/preset
    if (webComponent != nullptr)
    {
        float normVal = 0.0f;
        if (auto* param = audioProcessor.getParameters().getAPVTS().getParameter(parameterID))
            normVal = param->getValue(); // Always normalized 0.0 to 1.0 for WebUI

        juce::MessageManager::callAsync([this, parameterID, normVal]() {
            if (webComponent != nullptr)
            {
                juce::DynamicObject::Ptr obj = new juce::DynamicObject();
                obj->setProperty("id", parameterID);
                obj->setProperty("value", normVal);
                webComponent->emitEventIfBrowserIsVisible("parameterChanged", juce::var(obj.get()));
            }
        });
    }
}

void CZ101AudioProcessorEditor::presetLoaded(int index)
{
    if (webComponent != nullptr)
    {
        juce::MessageManager::callAsync([this, index]() {
            if (webComponent != nullptr)
            {
                const juce::ScopedReadLock srl(audioProcessor.getPresetManager().getLock());
                const auto& preset = audioProcessor.getPresetManager().getCurrentPreset();
                
                juce::DynamicObject::Ptr obj = new juce::DynamicObject();
                obj->setProperty("index", index);
                obj->setProperty("name", juce::String(preset.name));
                
                juce::DynamicObject::Ptr paramsObj = new juce::DynamicObject();
                for (const auto& [paramId, val] : preset.parameters)
                {
                    if (auto* param = audioProcessor.getParameters().getAPVTS().getParameter(paramId))
                    {
                        paramsObj->setProperty(juce::Identifier(paramId), param->getValue());
                    }
                }
                obj->setProperty("params", juce::var(paramsObj.get()));

                // Pack Envelopes for WebUI
                juce::DynamicObject::Ptr envsObj = new juce::DynamicObject();
                auto packEnv = [](const CZ101::State::EnvelopeData& env) -> juce::var {
                    juce::DynamicObject::Ptr e = new juce::DynamicObject();
                    juce::Array<juce::var> rates, levels;
                    for (int i = 0; i < 8; ++i) {
                        rates.add(env.rates[i]);
                        levels.add(env.levels[i]);
                    }
                    e->setProperty("rates", rates);
                    e->setProperty("levels", levels);
                    e->setProperty("sustainPoint", env.sustainPoint);
                    e->setProperty("endPoint", env.endPoint);
                    return juce::var(e.get());
                };
                
                juce::DynamicObject::Ptr dcaObj = new juce::DynamicObject();
                dcaObj->setProperty("line1", packEnv(preset.dcaEnv));
                dcaObj->setProperty("line2", packEnv(preset.dcaEnv2));
                envsObj->setProperty("dca", juce::var(dcaObj.get()));

                juce::DynamicObject::Ptr dcwObj = new juce::DynamicObject();
                dcwObj->setProperty("line1", packEnv(preset.dcwEnv));
                dcwObj->setProperty("line2", packEnv(preset.dcwEnv2));
                envsObj->setProperty("dcw", juce::var(dcwObj.get()));

                juce::DynamicObject::Ptr pitchObj = new juce::DynamicObject();
                pitchObj->setProperty("line1", packEnv(preset.pitchEnv));
                pitchObj->setProperty("line2", packEnv(preset.pitchEnv2));
                envsObj->setProperty("pitch", juce::var(pitchObj.get()));

                obj->setProperty("envelopes", juce::var(envsObj.get()));
                
                auto sysexBlock = audioProcessor.getSysExManager().createPatchDump(preset);
                juce::String hexStr;
                const uint8_t* bytes = static_cast<const uint8_t*>(sysexBlock.getData());
                for (size_t i = 0; i < sysexBlock.getSize(); ++i)
                {
                    if (i > 0) hexStr << " ";
                    hexStr << juce::String::toHexString((int)bytes[i]).toUpperCase().paddedLeft('0', 2);
                }
                obj->setProperty("sysexHex", hexStr);
                
                webComponent->emitEventIfBrowserIsVisible("presetLoaded", juce::var(obj.get()));
                webComponent->emitEventIfBrowserIsVisible("sysexDump", hexStr);
            }
        });
    }
}

void CZ101AudioProcessorEditor::bankUpdated() { }
void CZ101AudioProcessorEditor::presetRenamed(int index, const std::string& newName) { }

void CZ101AudioProcessorEditor::timerCallback()
{
    if (webComponent == nullptr) return;
    
    auto& visBuffer = audioProcessor.getVisTripleBuffer();
    if (visBuffer.hasNewData.exchange(false, std::memory_order_acquire))
    {
        int mid = visBuffer.midIndex.exchange(visBuffer.frontIndex.load(std::memory_order_relaxed), std::memory_order_acq_rel);
        visBuffer.frontIndex.store(mid, std::memory_order_relaxed);
        
        auto& frontBuf = visBuffer.buffers[mid];
        int validCount = visBuffer.sampleCounts[mid];
        if (validCount <= 0) validCount = 128;
        
        juce::Array<juce::var> jsArray;
        const int numSamples = 128; // Decimated to 128 samples to prevent WebUI IPC freezing
        jsArray.ensureStorageAllocated(numSamples);
        
        float step = static_cast<float>(validCount) / numSamples;
        for (int i = 0; i < numSamples; ++i)
        {
            int idx = std::min(validCount - 1, static_cast<int>(i * step));
            jsArray.add(juce::var(frontBuf[idx]));
        }
        
        juce::DynamicObject::Ptr obj = new juce::DynamicObject();
        obj->setProperty("samples", juce::var(jsArray));
        obj->setProperty("note", juce::var(audioProcessor.getLastMidiNotePlayed()));
        
        webComponent->emitEventIfBrowserIsVisible("scopeUpdate", juce::var(obj.get()));
    }
}
