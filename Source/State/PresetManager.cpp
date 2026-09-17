#include "PresetManager.h"
#include "Parameters.h"
#include "ParameterIDs.h"
#include "../Core/VoiceManager.h"

namespace CZ101 {
namespace State {

PresetManager::PresetManager(Parameters* parameters, Core::VoiceManager* vm)
    : parameters(parameters), voiceManager(vm)
{
    // Validate pointers (parameters can be null in headless/WASM context)
    jassert(voiceManager != nullptr);

    createFactoryPresets();
    // Default to first preset logic moved to PluginProcessor init
}

PresetManager::~PresetManager() = default;

void PresetManager::addListener(Listener* l) { listeners.add(l); }

void PresetManager::removeListener(Listener* l) { listeners.remove(l); }

void PresetManager::setCompareMode(bool enabled)
{
    const juce::ScopedWriteLock sl(presetLock);
    
    if (enabled && !isComparing)
    {
        // Enter Compare: Backup current -> Load Saved
        copyStateFromProcessor(); // Save current edits to 'currentPreset'
        compareBuffer = currentPreset; // Backup edits to 'compareBuffer'
        isComparing = true;
        
        // Load original from bank
        if (currentPresetIndex >= 0 && currentPresetIndex < (int)presets.size())
        {
             // We load into 'currentPreset' so the engine & UI reflect the "Saved" state
             // But we DON'T update the index or anything else.
             currentPreset = presets[currentPresetIndex];
        }
        // Apply "Saved" state to engine
        applyPresetToProcessor(currentPreset);
        // We do typically execute updateVoice logic here too
        // Reuse loadPresetFromStruct internal logic but without the lock re-entry potential?
        // Let's safe-call apply:
    }
    else if (!enabled && isComparing)
    {
        // Exit Compare: Restore Backup -> Engine
        currentPreset = compareBuffer;
        isComparing = false;
        applyPresetToProcessor(currentPreset);
    }
    
    // Update Voice Envelopes
    if (voiceManager)
    {
        applyEnvelopeToVoice(currentPreset.pitchEnv, 0, 1);
        applyEnvelopeToVoice(currentPreset.dcwEnv, 1, 1);
        applyEnvelopeToVoice(currentPreset.dcaEnv, 2, 1);
        
        applyEnvelopeToVoice(currentPreset.pitchEnv2, 0, 2);
        applyEnvelopeToVoice(currentPreset.dcwEnv2, 1, 2);
        applyEnvelopeToVoice(currentPreset.dcaEnv2, 2, 2);
    }
}

void PresetManager::loadPreset(int index, bool updateVoice)
{
    int notifyIndex = -1;
    Preset pToLoad;
    {
        const juce::ScopedWriteLock sl(presetLock);
        if (index >= 0 && index < static_cast<int>(presets.size()))
        {
            currentPresetIndex = index;
            currentPreset = presets[index];
            pToLoad = currentPreset;
            notifyIndex = index;
        }
    } 

    if (notifyIndex >= 0)
    {
        applyPresetToProcessor(pToLoad);
        
        if (updateVoice && voiceManager)
        {
            voiceManager->allSoundOff();
            
            applyEnvelopeToVoice(pToLoad.pitchEnv, 0, 1);
            applyEnvelopeToVoice(pToLoad.dcwEnv, 1, 1);
            applyEnvelopeToVoice(pToLoad.dcaEnv, 2, 1);
            
            applyEnvelopeToVoice(pToLoad.pitchEnv2, 0, 2);
            applyEnvelopeToVoice(pToLoad.dcwEnv2, 1, 2);
            applyEnvelopeToVoice(pToLoad.dcaEnv2, 2, 2);
        }

        // Notify listeners OUTSIDE the lock
        listeners.call([notifyIndex](Listener& l) { l.presetLoaded(notifyIndex); });
    }
} 

void PresetManager::loadPresetFromStruct(const Preset& p, bool updateVoice, bool notifyHost)
{
    {
        const juce::ScopedWriteLock sl(presetLock);
        currentPreset = p;
    }
    
    applyPresetToProcessor(p);
    
    if (updateVoice && voiceManager)
    {
        voiceManager->allSoundOff();
        
        applyEnvelopeToVoice(p.pitchEnv, 0, 1);
        applyEnvelopeToVoice(p.dcwEnv, 1, 1);
        applyEnvelopeToVoice(p.dcaEnv, 2, 1);
        
        applyEnvelopeToVoice(p.pitchEnv2, 0, 2);
        applyEnvelopeToVoice(p.dcwEnv2, 1, 2);
        applyEnvelopeToVoice(p.dcaEnv2, 2, 2);
    }
    
    // Audit Fix [2.1]: Ensure Host Display and APVTS are in sync
    if (notifyHost && parameters)
    {
         // Note: We cannot call audioProcessor.updateHostDisplay() directly here as we only have Parameters.
         // However, applyPresetToProcessor calls setValueNotifyingHost which triggers listeners.
    }
}

void PresetManager::applyPresetToProcessor(const Preset& p)
{
    if (parameters)
    {
        for (const auto& [paramId, param] : parameters->getParameterMap())
        {
            if (param != nullptr)
            {
                auto it = p.parameters.find(paramId.toStdString());
                if (it != p.parameters.end())
                {
                    float normalized = param->convertTo0to1(it->second);
                    param->setValueNotifyingHost(normalized);
                }
                else
                {
                    // Ensure filters are fully open by default for classic presets/imports
                    if (paramId == ParameterIDs::lpfCutoff)
                        param->setValueNotifyingHost(param->convertTo0to1(20000.0f));
                    else if (paramId == ParameterIDs::lpfReso)
                        param->setValueNotifyingHost(param->convertTo0to1(0.0f));
                    else if (paramId == ParameterIDs::hpfCutoff)
                        param->setValueNotifyingHost(param->convertTo0to1(20.0f));
                    else
                        param->setValueNotifyingHost(param->getDefaultValue());
                }
            }
        }
    }
}

void PresetManager::applyEnvelopeToVoice(const EnvelopeData& env, int type, int line)
{
    if (!voiceManager) return;

    for (int i = 0; i < 8; ++i)
    {
        if (type == 0) voiceManager->setPitchStage(line, i, env.rates[i], env.levels[i]);
        else if (type == 1) voiceManager->setDCWStage(line, i, env.rates[i], env.levels[i]);
        else if (type == 2) voiceManager->setDCAStage(line, i, env.rates[i], env.levels[i]);
    }

    if (type == 0) {
        voiceManager->setPitchSustainPoint(line, env.sustainPoint);
        voiceManager->setPitchEndPoint(line, env.endPoint);
    } else if (type == 1) {
        voiceManager->setDCWSustainPoint(line, env.sustainPoint);
        voiceManager->setDCWEndPoint(line, env.endPoint);
    } else if (type == 2) {
        voiceManager->setDCASustainPoint(line, env.sustainPoint);
        voiceManager->setDCAEndPoint(line, env.endPoint);
    }
}

void PresetManager::copyStateFromProcessor(bool notifyListeners)
{
    {
        const juce::ScopedWriteLock swl(presetLock);
        // 1. Capture Parameters (Denormalized)
        if (parameters)
        {
            // Iterate over ALL defined parameters using the new getter
            const auto& map = parameters->getParameterMap();
            for (const auto& pair : map) 
            {
                const juce::String& key = pair.first;
                juce::RangedAudioParameter* param = pair.second;
                std::string stdKey = key.toStdString();

                if (auto* p = dynamic_cast<juce::AudioParameterFloat*>(param))
                    currentPreset.parameters[stdKey] = p->get();
                else if (auto* pInt = dynamic_cast<juce::AudioParameterInt*>(param))
                    currentPreset.parameters[stdKey] = (float)pInt->get();
                else if (auto* pChoice = dynamic_cast<juce::AudioParameterChoice*>(param))
                    currentPreset.parameters[stdKey] = (float)pChoice->getIndex();
                else if (auto* pBool = dynamic_cast<juce::AudioParameterBool*>(param))
                    currentPreset.parameters[stdKey] = pBool->get() ? 1.0f : 0.0f;
            }
        }

        // 2. Capture Envelopes from VoiceManager
        if (voiceManager)
        {
            // --- LINE 1 ---
            for(int i=0; i<8; ++i) voiceManager->getDCWStage(1, i, currentPreset.dcwEnv.rates[i], currentPreset.dcwEnv.levels[i]);
            currentPreset.dcwEnv.sustainPoint = voiceManager->getDCWSustainPoint(1);
            currentPreset.dcwEnv.endPoint = voiceManager->getDCWEndPoint(1);
            
            for(int i=0; i<8; ++i) voiceManager->getDCAStage(1, i, currentPreset.dcaEnv.rates[i], currentPreset.dcaEnv.levels[i]);
            currentPreset.dcaEnv.sustainPoint = voiceManager->getDCASustainPoint(1);
            currentPreset.dcaEnv.endPoint = voiceManager->getDCAEndPoint(1);
            
            for(int i=0; i<8; ++i) voiceManager->getPitchStage(1, i, currentPreset.pitchEnv.rates[i], currentPreset.pitchEnv.levels[i]);
            currentPreset.pitchEnv.sustainPoint = voiceManager->getPitchSustainPoint(1);
            currentPreset.pitchEnv.endPoint = voiceManager->getPitchEndPoint(1);

            // --- LINE 2 ---
            for(int i=0; i<8; ++i) voiceManager->getDCWStage(2, i, currentPreset.dcwEnv2.rates[i], currentPreset.dcwEnv2.levels[i]);
            currentPreset.dcwEnv2.sustainPoint = voiceManager->getDCWSustainPoint(2);
            currentPreset.dcwEnv2.endPoint = voiceManager->getDCWEndPoint(2);
            
            for(int i=0; i<8; ++i) voiceManager->getDCAStage(2, i, currentPreset.dcaEnv2.rates[i], currentPreset.dcaEnv2.levels[i]);
            currentPreset.dcaEnv2.sustainPoint = voiceManager->getDCASustainPoint(2);
            currentPreset.dcaEnv2.endPoint = voiceManager->getDCAEndPoint(2);
            
            for(int i=0; i<8; ++i) voiceManager->getPitchStage(2, i, currentPreset.pitchEnv2.rates[i], currentPreset.pitchEnv2.levels[i]);
            currentPreset.pitchEnv2.sustainPoint = voiceManager->getPitchSustainPoint(2);
            currentPreset.pitchEnv2.endPoint = voiceManager->getPitchEndPoint(2);
        }
    }
    
    // Notify Listeners OUTSIDE the lock
    if (notifyListeners) {
        listeners.call([this](Listener& l) { l.presetLoaded(currentPresetIndex); });
    }
}

void PresetManager::savePreset(int index, const std::string& name)
{
    {
        const juce::ScopedWriteLock sl(presetLock);
        copyStateFromProcessor(false); // Don't trigger a reload on the UI
        currentPreset.name = name; // Update the in-memory active patch name
        
        if (index >= 0 && index < static_cast<int>(presets.size()))
        {
            presets[index] = currentPreset;
        }
    }
    
    autoSaveUserBank();
    listeners.call(&Listener::bankUpdated);
}


} // namespace State
} // namespace CZ101
