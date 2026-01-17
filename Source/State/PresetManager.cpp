#include "PresetManager.h"
#include "Parameters.h"
#include "../Core/VoiceManager.h"
// JuceHeader is now included in PresetManager.h

namespace CZ101 {
namespace State {

PresetManager::PresetManager(Parameters* parameters, Core::VoiceManager* vm)
    : parameters(parameters), voiceManager(vm)
{
    // Validate pointers
    jassert(parameters != nullptr);
    jassert(voiceManager != nullptr);

    createFactoryPresets();
    // Default to first preset logic moved to PluginProcessor init
}

PresetManager::~PresetManager() = default;

void PresetManager::beginCompare()
{
    const juce::ScopedWriteLock sl(presetLock);
    if (comparing) return;
    
    // Save current EDITED state to buffer
    copyStateFromProcessor();
    compareBuffer = currentPreset;
    comparing = true;
    
    // Reload original state from bank
    if (currentPresetIndex >= 0 && currentPresetIndex < (int)presets.size())
    {
        loadPreset(currentPresetIndex, true);
    }
}

void PresetManager::endCompare()
{
    const juce::ScopedWriteLock sl(presetLock);
    if (!comparing) return;
    
    // Restore the edited state from buffer
    loadPresetFromStruct(compareBuffer, true);
    comparing = false;
}

void PresetManager::loadPreset(int index, bool updateVoice)
{
    const juce::ScopedWriteLock sl(presetLock);
    if (index >= 0 && index < static_cast<int>(presets.size()))
    {
        currentPresetIndex = index; // Correctly track the current index
        currentPreset = presets[index];
        applyPresetToProcessor(); // Updates UI Knobs (ADSR) via Parameters
        
        // Update Voice Manager directly with full 8-stage data
        if (updateVoice && voiceManager)
        {
            applyEnvelopeToVoice(currentPreset.pitchEnv, 0, 1);
            applyEnvelopeToVoice(currentPreset.dcwEnv, 1, 1);
            applyEnvelopeToVoice(currentPreset.dcaEnv, 2, 1);
            
            applyEnvelopeToVoice(currentPreset.pitchEnv2, 0, 2);
            applyEnvelopeToVoice(currentPreset.dcwEnv2, 1, 2);
            applyEnvelopeToVoice(currentPreset.dcaEnv2, 2, 2);
        }
    }
}

void PresetManager::loadPresetFromStruct(const Preset& p, bool updateVoice)
{
    const juce::ScopedWriteLock sl(presetLock);
    // Load the structure directly as the current preset
    currentPreset = p;

    // Apply to parameters and voice manager immediately
    applyPresetToProcessor();
    
    if (updateVoice && voiceManager)
    {
        applyEnvelopeToVoice(currentPreset.pitchEnv, 0, 1);
        applyEnvelopeToVoice(currentPreset.dcwEnv, 1, 1);
        applyEnvelopeToVoice(currentPreset.dcaEnv, 2, 1);
        
        applyEnvelopeToVoice(currentPreset.pitchEnv2, 0, 2);
        applyEnvelopeToVoice(currentPreset.dcwEnv2, 1, 2);
        applyEnvelopeToVoice(currentPreset.dcaEnv2, 2, 2);
    }
}

void PresetManager::applyPresetToProcessor()
{
    if (parameters)
    {
        for (const auto& [paramId, value] : currentPreset.parameters)
        {
            if (auto* param = parameters->getParameter(paramId))
            {
                float normalized = param->convertTo0to1(value);
                param->setValueNotifyingHost(normalized);
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

void PresetManager::copyStateFromProcessor()
{
    // 1. Capture Parameters (Denormalized)
    if (parameters)
    {
        // Iterate over ALL defined parameters using the new getter
        const auto& map = parameters->getParameterMap();
        for (const auto& pair : map) // use pair to avoid structured binding confusion if const ref issues
        {
            const juce::String& key = pair.first;
            juce::RangedAudioParameter* param = pair.second;
            
            // Convert juce::String key to std::string for the std::map index
            std::string stdKey = key.toStdString();

            if (auto* p = dynamic_cast<juce::AudioParameterFloat*>(param))
            {
                currentPreset.parameters[stdKey] = p->get();
            }
            else if (auto* pInt = dynamic_cast<juce::AudioParameterInt*>(param))
            {
                 currentPreset.parameters[stdKey] = (float)pInt->get();
            }
            else if (auto* pChoice = dynamic_cast<juce::AudioParameterChoice*>(param))
            {
                 currentPreset.parameters[stdKey] = (float)pChoice->getIndex();
            }
             else if (auto* pBool = dynamic_cast<juce::AudioParameterBool*>(param))
            {
                 currentPreset.parameters[stdKey] = pBool->get() ? 1.0f : 0.0f;
            }
        }
    }

    // 2. Capture Envelopes from VoiceManager
    if (voiceManager)
    {
        // --- LINE 1 ---
        // DCW
        for(int i=0; i<8; ++i) voiceManager->getDCWStage(1, i, currentPreset.dcwEnv.rates[i], currentPreset.dcwEnv.levels[i]);
        currentPreset.dcwEnv.sustainPoint = voiceManager->getDCWSustainPoint(1);
        currentPreset.dcwEnv.endPoint = voiceManager->getDCWEndPoint(1);
        
        // DCA
        for(int i=0; i<8; ++i) voiceManager->getDCAStage(1, i, currentPreset.dcaEnv.rates[i], currentPreset.dcaEnv.levels[i]);
        currentPreset.dcaEnv.sustainPoint = voiceManager->getDCASustainPoint(1);
        currentPreset.dcaEnv.endPoint = voiceManager->getDCAEndPoint(1);
        
        // Pitch
        for(int i=0; i<8; ++i) voiceManager->getPitchStage(1, i, currentPreset.pitchEnv.rates[i], currentPreset.pitchEnv.levels[i]);
        currentPreset.pitchEnv.sustainPoint = voiceManager->getPitchSustainPoint(1);
        currentPreset.pitchEnv.endPoint = voiceManager->getPitchEndPoint(1);

        // --- LINE 2 ---
        // DCW
        for(int i=0; i<8; ++i) voiceManager->getDCWStage(2, i, currentPreset.dcwEnv2.rates[i], currentPreset.dcwEnv2.levels[i]);
        currentPreset.dcwEnv2.sustainPoint = voiceManager->getDCWSustainPoint(2);
        currentPreset.dcwEnv2.endPoint = voiceManager->getDCWEndPoint(2);
        
        // DCA
        for(int i=0; i<8; ++i) voiceManager->getDCAStage(2, i, currentPreset.dcaEnv2.rates[i], currentPreset.dcaEnv2.levels[i]);
        currentPreset.dcaEnv2.sustainPoint = voiceManager->getDCASustainPoint(2);
        currentPreset.dcaEnv2.endPoint = voiceManager->getDCAEndPoint(2);
        
        // Pitch
        for(int i=0; i<8; ++i) voiceManager->getPitchStage(2, i, currentPreset.pitchEnv2.rates[i], currentPreset.pitchEnv2.levels[i]);
        currentPreset.pitchEnv2.sustainPoint = voiceManager->getPitchSustainPoint(2);
        currentPreset.pitchEnv2.endPoint = voiceManager->getPitchEndPoint(2);
    }
}

void PresetManager::savePreset(int index, const std::string& name)
{
    if (index >= 0 && index < static_cast<int>(presets.size()))
    {
        // 1. Update the internal vector with the current state (which should have been captured before calling this found needs)
        // Actually, let's ensure we capture it here to be safe, OR assume caller did copyStateFromProcessor.
        // Better: caller (Editor) calls copyStateFromProcessor first.
        
        presets[index] = currentPreset;
        presets[index].name = name;
        
        autoSaveUserBank();
    }
}

// Helper to init default envelopes
static void initEnvelopes(Preset& p)
{
    // Initialize all envelope stages to a default state
    for(int i=0; i<8; ++i) {
        p.dcwEnv.rates[i] = 0.5f; p.dcwEnv.levels[i] = 1.0f; // Timbre open
        p.dcaEnv.rates[i] = 0.5f; p.dcaEnv.levels[i] = 1.0f; // Volume up
        p.pitchEnv.rates[i] = 0.5f; p.pitchEnv.levels[i] = 0.5f; // Pitch center
    }
    p.dcwEnv.sustainPoint = 2; p.dcwEnv.endPoint = 3;
    p.dcaEnv.sustainPoint = 2; p.dcaEnv.endPoint = 3;
    p.pitchEnv.sustainPoint = 2; p.pitchEnv.endPoint = 3;
}

void PresetManager::createFactoryPresets()
{
    presets.clear();
    
    // --- PRESTIGIOUS USER CONTRIBUTIONS ---
    // Patch 1: Moog-Like (Classic Bass/Lead)
    {
        Preset p;
        p.name = "Moog-Like";
        p.author = "User";
        initEnvelopes(p);
        
        p.parameters["OSC1_WAVEFORM"] = 1.0f; // Saw
        p.parameters["OSC1_LEVEL"] = 1.0f;
        p.parameters["OSC2_WAVEFORM"] = 1.0f; 
        p.parameters["OSC2_LEVEL"] = 1.0f;
        p.parameters["OSC2_DETUNE"] = -7.0f; 
        
        // DCW
         p.dcwEnv.levels[0] = 0.48f; p.dcwEnv.rates[0] = 0.78f;
         p.dcwEnv.levels[1] = 0.75f; p.dcwEnv.rates[1] = 0.5f;
         p.dcwEnv.levels[2] = 0.83f; p.dcwEnv.rates[2] = 0.37f;
        p.dcwEnv.sustainPoint = 2;
         p.dcwEnv.levels[3] = 0.0f; p.dcwEnv.rates[3] = 0.41f;
        p.dcwEnv.endPoint = 3;

        // DCA
        p.dcaEnv.levels[0] = 0.87f; p.dcaEnv.rates[0] = 0.84f;
        p.dcaEnv.sustainPoint = 0; 
        p.dcaEnv.levels[1] = 0.0f; p.dcaEnv.rates[1] = 0.39f;
        p.dcaEnv.endPoint = 1;
        
        p.parameters["LFO_WAVE"] = 3.0f; 
        p.parameters["LFO_RATE"] = 0.49f;
        p.parameters["LFO_DEPTH"] = 0.59f;

        presets.push_back(p);
    }

    // Patch 2: Polyanalogue (Juno-106)
    {
        Preset p;
        p.name = "Polyanalogue";
        p.author = "User";
        initEnvelopes(p);

        p.parameters["OSC1_WAVEFORM"] = 1.0f;
        p.parameters["OSC1_LEVEL"] = 0.5f;
        p.parameters["OSC2_WAVEFORM"] = 1.0f; 
        p.parameters["OSC2_LEVEL"] = 0.5f;
        p.parameters["OSC2_DETUNE"] = 6.0f; 

        // DCW
        p.dcwEnv.levels[0] = 0.99f; p.dcwEnv.rates[0] = 0.99f; 
        p.dcwEnv.levels[1] = 0.96f; p.dcwEnv.rates[1] = 0.4f;
        p.dcwEnv.levels[2] = 0.52f; p.dcwEnv.rates[2] = 0.3f;
        p.dcwEnv.sustainPoint = 2;
        p.dcwEnv.levels[3] = 0.0f; p.dcwEnv.rates[3] = 0.3f;
        p.dcwEnv.endPoint = 3;

        // DCA
        p.dcaEnv.levels[0] = 0.5f; p.dcaEnv.rates[0] = 1.0f; 
        p.dcaEnv.levels[1] = 0.99f; p.dcaEnv.rates[1] = 0.77f; 
        p.dcaEnv.levels[2] = 0.91f; p.dcaEnv.rates[2] = 0.67f;
        p.dcaEnv.sustainPoint = 2; 
        p.dcaEnv.levels[3] = 0.59f; p.dcaEnv.rates[3] = 0.79f;
        p.dcaEnv.levels[4] = 0.0f; p.dcaEnv.rates[4] = 0.33f;
        p.dcaEnv.endPoint = 4;

        presets.push_back(p);
    }
    
    // Patch 3: Sonic Bubbles (FX)
    {
        Preset p;
        p.name = "Sonic Bubbles";
        p.author = "User";
        initEnvelopes(p);

        p.parameters["LFO_WAVE"] = 1.0f; 
        p.parameters["LFO_DEPTH"] = 1.0f; 
        p.parameters["LFO_RATE"] = 0.6f; 

        p.pitchEnv.levels[0] = 0.5f; p.pitchEnv.rates[0] = 0.5f; 
        p.pitchEnv.levels[1] = 0.0f; p.pitchEnv.rates[1] = 0.5f;
        
        presets.push_back(p);
    }

    createBassPreset();
    createStringPreset();
    createBrassPreset();
    createLeadPreset();
    createBellsPreset(); 
    
    // Fill rest with Init
    for (int i = 5; i < 64; ++i)
    {
        Preset p;
        p.name = "Init User " + std::to_string(i);
        initEnvelopes(p);
        
        // Defaults
        p.parameters["OSC1_WAVEFORM"] = 0.0f; p.parameters["OSC1_LEVEL"] = 1.0f;
        p.parameters["OSC2_WAVEFORM"] = 0.0f; p.parameters["OSC2_LEVEL"] = 0.0f;
        p.parameters["OSC2_DETUNE"] = 0.0f;
        
        p.parameters["DCW_ATTACK"] = 0.0f; p.parameters["DCW_DECAY"] = 0.0f; p.parameters["DCW_SUSTAIN"] = 1.0f; p.parameters["DCW_RELEASE"] = 0.0f;
        p.parameters["DCA_ATTACK"] = 0.0f; p.parameters["DCA_DECAY"] = 0.0f; p.parameters["DCA_SUSTAIN"] = 1.0f; p.parameters["DCA_RELEASE"] = 0.0f;
        
        p.parameters["FILTER_CUTOFF"] = 20000.0f; p.parameters["FILTER_RESONANCE"] = 0.1f;
        p.parameters["LFO_RATE"] = 1.0f;
        p.parameters["DELAY_MIX"] = 0.0f; p.parameters["REVERB_MIX"] = 0.0f;
        p.parameters["HARD_SYNC"] = 0.0f;
        p.parameters["RING_MOD"] = 0.0f;
        p.parameters["GLIDE"] = 0.0f;
        
        // Audit Fix 3.2: Initialize "Phantom" Parameters
        p.parameters["LINE_SELECT"] = 1.0f; // Default Line 1
        p.parameters["SYSTEM_PRG"] = 0.0f;
        p.parameters["PROTECT_SWITCH"] = 0.0f;
        
        // Chorus
        p.parameters["CHORUS_RATE"] = 0.5f;
        p.parameters["CHORUS_DEPTH"] = 2.0f;
        p.parameters["CHORUS_MIX"] = 0.0f;

        presets.push_back(p);
    }
}

void PresetManager::createBassPreset()
{
    Preset p;
    p.name = "CZ Bass";
    initEnvelopes(p);
    
    // ===== OSCILLATORS (NORMALIZED) =====
    p.parameters["OSC1_WAVEFORM"] = 1.0f;      // Saw
    p.parameters["OSC1_LEVEL"] = 0.6f;         // âœ… 60% (normalized)
    p.parameters["OSC2_WAVEFORM"] = 2.0f;      // Square
    p.parameters["OSC2_LEVEL"] = 0.4f;         // âœ… 40% (normalized)
    // Total: 0.6 + 0.4 = 1.0 âœ…
    
    p.parameters["OSC2_DETUNE"] = -10.0f;      // -10 cents
    
    // ===== ADSR (IN SECONDS) =====
    p.parameters["DCW_ATTACK"] = 0.01f;        // âœ… 10ms (crisp)
    p.parameters["DCW_DECAY"] = 0.2f;          // âœ… 200ms
    p.parameters["DCW_SUSTAIN"] = 0.2f;        // âœ… 20% level
    p.parameters["DCW_RELEASE"] = 0.1f;        // âœ… 100ms
    
    p.parameters["DCA_ATTACK"] = 0.001f;       // âœ… 1ms (very crisp)
    p.parameters["DCA_DECAY"] = 0.2f;          // âœ… 200ms
    p.parameters["DCA_SUSTAIN"] = 0.5f;        // âœ… 50% level
    p.parameters["DCA_RELEASE"] = 0.15f;       // âœ… 150ms
    
    // ===== FILTER =====
    p.parameters["FILTER_CUTOFF"] = 2000.0f;   // 2000 Hz
    p.parameters["FILTER_RESONANCE"] = 0.5f;   // 50% Q
    
    // ===== LFO =====
    p.parameters["LFO_RATE"] = 0.5f;           // 0.5 Hz
    p.parameters["LFO_DEPTH"] = 0.0f;          // No vibrato
    
    // ===== EFFECTS =====
    p.parameters["DELAY_TIME"] = 0.3f;         // âœ… 300ms
    p.parameters["DELAY_FEEDBACK"] = 0.3f;     // 30%
    p.parameters["DELAY_MIX"] = 0.08f;         // âœ… 8% wet
    
    p.parameters["CHORUS_RATE"] = 0.5f;        // 0.5 Hz
    p.parameters["CHORUS_DEPTH"] = 2.0f;       // 2ms
    p.parameters["CHORUS_MIX"] = 0.0f;         // Off
    
    p.parameters["REVERB_SIZE"] = 0.3f;        // Small room
    p.parameters["REVERB_MIX"] = 0.08f;        // âœ… 8% wet
    
    p.parameters["HARD_SYNC"] = 0.0f;          // Off
    p.parameters["RING_MOD"] = 0.0f;           // Off
    p.parameters["GLIDE"] = 0.0f;         // No portamento
    
    presets.push_back(p);
}

void PresetManager::createStringPreset()
{
    Preset p;
    p.name = "Vintage Strings";
    initEnvelopes(p);
    
    // ===== OSCILLATORS (NORMALIZED) =====
    p.parameters["OSC1_WAVEFORM"] = 1.0f;      // Saw
    p.parameters["OSC1_LEVEL"] = 0.5f;         // âœ… 50% (normalized)
    p.parameters["OSC2_WAVEFORM"] = 1.0f;      // Saw
    p.parameters["OSC2_LEVEL"] = 0.5f;         // âœ… 50% (normalized)
    // Total: 0.5 + 0.5 = 1.0 âœ…
    
    p.parameters["OSC2_DETUNE"] = 12.0f;       // +1 octava
    
    // ===== ADSR (IN SECONDS) - REALISTIC STRINGS =====
    p.parameters["DCW_ATTACK"] = 0.3f;         // âœ… 300ms (bow friction)
    p.parameters["DCW_DECAY"] = 0.4f;          // âœ… 400ms
    p.parameters["DCW_SUSTAIN"] = 0.7f;        // âœ… 70% level
    p.parameters["DCW_RELEASE"] = 0.5f;        // âœ… 500ms
    
    p.parameters["DCA_ATTACK"] = 0.4f;         // âœ… 400ms (smooth)
    p.parameters["DCA_DECAY"] = 0.3f;          // âœ… 300ms
    p.parameters["DCA_SUSTAIN"] = 0.8f;        // âœ… 80% level
    p.parameters["DCA_RELEASE"] = 0.6f;        // âœ… 600ms (smooth release)
    
    // ===== FILTER =====
    p.parameters["FILTER_CUTOFF"] = 8000.0f;   // Open
    p.parameters["FILTER_RESONANCE"] = 0.3f;   // 30% Q
    
    // ===== LFO (VIBRATO) =====
    p.parameters["LFO_RATE"] = 4.5f;           // âœ… 4.5 Hz
    p.parameters["LFO_DEPTH"] = 0.08f;         // âœ… Subtle vibrato
    
    // ===== EFFECTS =====
    p.parameters["DELAY_TIME"] = 0.25f;        // âœ… 250ms
    p.parameters["DELAY_FEEDBACK"] = 0.4f;     // 40%
    p.parameters["DELAY_MIX"] = 0.3f;          // âœ… 30% wet (longer tail)
    
    p.parameters["CHORUS_RATE"] = 0.6f;        // 0.6 Hz
    p.parameters["CHORUS_DEPTH"] = 3.0f;       // 3ms
    p.parameters["CHORUS_MIX"] = 0.15f;        // âœ… 15% light chorus
    
    p.parameters["REVERB_SIZE"] = 0.7f;        // Large room
    p.parameters["REVERB_MIX"] = 0.4f;         // âœ… 40% wet (lush)
    
    p.parameters["HARD_SYNC"] = 0.0f;
    p.parameters["RING_MOD"] = 0.0f;
    p.parameters["GLIDE"] = 0.0f;
    
    presets.push_back(p);
}

void PresetManager::createBrassPreset()
{
    Preset p;
    p.name = "Synth Brass";
    initEnvelopes(p);
    
    p.parameters["OSC1_WAVEFORM"] = 1.0f; p.parameters["OSC1_LEVEL"] = 1.0f;
    p.parameters["OSC2_WAVEFORM"] = 3.0f; p.parameters["OSC2_LEVEL"] = 0.6f; // Triangle for body
    p.parameters["OSC2_DETUNE"] = 7.0f; // Slight detune
    
    // Pitch Envelope (Brass Attack: slight drop-up)
    // Stage 0: Fast drop to slightly fla (-2 semitones approx)
    p.pitchEnv.rates[0] = 0.9f; p.pitchEnv.levels[0] = 0.48f; 
    // Stage 1: Rise to slightly sharp (overshoot)
    p.pitchEnv.rates[1] = 0.6f; p.pitchEnv.levels[1] = 0.52f;
    // Stage 2: Settle to Unison
    p.pitchEnv.rates[2] = 0.4f; p.pitchEnv.levels[2] = 0.5f;
    // Stage 3: Sustain at Unison
    p.pitchEnv.rates[3] = 0.99f;p.pitchEnv.levels[3] = 0.5f;
    p.pitchEnv.sustainPoint = 3;
    p.pitchEnv.endPoint = 3;

    // DCW (Brass Swell)
    // Stage 0: Sharp attack
    p.dcwEnv.rates[0] = 0.85f; p.dcwEnv.levels[0] = 0.9f; 
    // Stage 1: Decay slightly to body
    p.dcwEnv.rates[1] = 0.7f;  p.dcwEnv.levels[1] = 0.7f;
    // Stage 2: Swell up a bit (breath)
    p.dcwEnv.rates[2] = 0.4f;  p.dcwEnv.levels[2] = 0.85f;
    // Stage 3: Sustain
    p.dcwEnv.rates[3] = 0.99f; p.dcwEnv.levels[3] = 0.85f;
    p.dcwEnv.sustainPoint = 3;
    p.dcwEnv.endPoint = 4; // Use stage 4 for release
    
    // Stage 4: Release
    p.dcwEnv.rates[4] = 0.6f; p.dcwEnv.levels[4] = 0.0f;
    
    // DCA (Standard ADSR-ish)
    p.dcaEnv.rates[0] = 0.85f; p.dcaEnv.levels[0] = 1.0f;
    p.dcaEnv.rates[1] = 0.7f;  p.dcaEnv.levels[1] = 0.9f;
    p.dcaEnv.rates[2] = 0.99f; p.dcaEnv.levels[2] = 0.9f;
    p.dcaEnv.rates[3] = 0.6f;  p.dcaEnv.levels[3] = 0.0f;
    p.dcaEnv.sustainPoint = 2;
    p.dcaEnv.endPoint = 3;
    
    // UI Params (Approximate for display)
    p.parameters["DCW_ATTACK"] = 0.2f; p.parameters["DCW_DECAY"] = 0.3f; p.parameters["DCW_SUSTAIN"] = 0.8f; p.parameters["DCW_RELEASE"] = 0.4f;
    p.parameters["DCA_ATTACK"] = 0.1f; p.parameters["DCA_DECAY"] = 0.2f; p.parameters["DCA_SUSTAIN"] = 0.9f; p.parameters["DCA_RELEASE"] = 0.4f;

    // Filter
    p.parameters["FILTER_CUTOFF"] = 5000.0f;
    p.parameters["FILTER_RESONANCE"] = 0.6f;

    // LFO
    p.parameters["LFO_RATE"] = 0.5f;
    
    // Effects
    p.parameters["DELAY_TIME"] = 0.0f; p.parameters["DELAY_FEEDBACK"] = 0.0f; p.parameters["DELAY_MIX"] = 0.0f;
    p.parameters["REVERB_SIZE"] = 0.6f; p.parameters["REVERB_MIX"] = 0.3f;
    
    p.parameters["HARD_SYNC"] = 0.0f;
    p.parameters["RING_MOD"] = 0.0f;
    p.parameters["GLIDE"] = 0.0f;

    presets.push_back(p);
}

void PresetManager::createLeadPreset()
{
    Preset p;
    p.name = "Solo Lead";
    initEnvelopes(p);
    
    p.parameters["OSC1_WAVEFORM"] = 2.0f; p.parameters["OSC1_LEVEL"] = 1.0f;
    p.parameters["OSC2_WAVEFORM"] = 2.0f; p.parameters["OSC2_LEVEL"] = 0.6f;
    p.parameters["OSC2_DETUNE"] = 0.0f;
    
    // DCW: Open
    p.dcwEnv.rates[0] = 0.99f; p.dcwEnv.levels[0] = 1.0f;
    p.dcwEnv.rates[1] = 0.99f; p.dcwEnv.levels[1] = 1.0f; // Sustain High
    p.dcwEnv.rates[2] = 0.99f; p.dcwEnv.levels[2] = 1.0f;
    p.dcwEnv.rates[3] = 0.5f;  p.dcwEnv.levels[3] = 0.0f;
    
    // DCA
    p.dcaEnv.rates[0] = 0.99f; p.dcaEnv.levels[0] = 1.0f; // Click attack
    p.dcaEnv.rates[1] = 0.9f;  p.dcaEnv.levels[1] = 1.0f;
    p.dcaEnv.rates[2] = 0.99f; p.dcaEnv.levels[2] = 1.0f;
    p.dcaEnv.rates[3] = 0.7f;  p.dcaEnv.levels[3] = 0.0f;
    
    p.parameters["DCW_ATTACK"] = 0.0f; p.parameters["DCW_DECAY"] = 0.0f; p.parameters["DCW_SUSTAIN"] = 1.0f; p.parameters["DCW_RELEASE"] = 0.1f;
    p.parameters["DCA_ATTACK"] = 0.001f; p.parameters["DCA_DECAY"] = 0.1f; p.parameters["DCA_SUSTAIN"] = 1.0f; p.parameters["DCA_RELEASE"] = 0.2f;
    
    // Filter
    p.parameters["FILTER_CUTOFF"] = 20000.0f;
    p.parameters["FILTER_RESONANCE"] = 0.1f;

    // LFO
    p.parameters["LFO_RATE"] = 4.0f;
    
    // Effects
    p.parameters["DELAY_TIME"] = 0.4f; p.parameters["DELAY_FEEDBACK"] = 0.5f; p.parameters["DELAY_MIX"] = 0.4f;
    p.parameters["REVERB_SIZE"] = 0.4f; p.parameters["REVERB_MIX"] = 0.2f;
    
    p.parameters["HARD_SYNC"] = 1.0f; // ENABLE HARD SYNC FOR LEAD
    p.parameters["RING_MOD"] = 0.0f;
    p.parameters["GLIDE"] = 0.2f; // ENABLE GLIDE FOR LEAD!
    p.parameters["CHORUS_RATE"] = 0.5f; p.parameters["CHORUS_DEPTH"] = 2.0f; p.parameters["CHORUS_MIX"] = 0.0f;
    p.parameters["CHORUS_RATE"] = 0.5f; p.parameters["CHORUS_DEPTH"] = 2.0f; p.parameters["CHORUS_MIX"] = 0.0f;

    presets.push_back(p);
}

void PresetManager::createBellsPreset()
{
    Preset p;
    p.name = "Digital Bells";
    initEnvelopes(p);
    
    p.parameters["OSC1_WAVEFORM"] = 0.0f; p.parameters["OSC1_LEVEL"] = 1.0f;
    p.parameters["OSC2_WAVEFORM"] = 0.0f; p.parameters["OSC2_LEVEL"] = 1.0f;
    p.parameters["OSC2_DETUNE"] = 350.0f; // Detune for bell
    
    // DCW: Short
    p.dcwEnv.rates[0] = 0.99f; p.dcwEnv.levels[0] = 1.0f;
    p.dcwEnv.rates[1] = 0.4f;  p.dcwEnv.levels[1] = 0.0f; // Long decay to 0
    p.dcwEnv.rates[2] = 0.99f; p.dcwEnv.levels[2] = 0.0f; // Hold 0
    p.dcwEnv.rates[3] = 0.5f;  p.dcwEnv.levels[3] = 0.0f;
    p.dcwEnv.sustainPoint = 2; // Sustain silence
    
    // DCA
    p.dcaEnv.rates[0] = 0.99f; p.dcaEnv.levels[0] = 1.0f;
    p.dcaEnv.rates[1] = 0.3f;  p.dcaEnv.levels[1] = 0.0f; // Long decay
    p.dcaEnv.rates[2] = 0.99f; p.dcaEnv.levels[2] = 0.0f;
    p.dcaEnv.rates[3] = 0.5f;  p.dcaEnv.levels[3] = 0.0f;
    
    p.parameters["DCW_ATTACK"] = 0.0f; p.parameters["DCW_DECAY"] = 0.8f; p.parameters["DCW_SUSTAIN"] = 0.0f; p.parameters["DCW_RELEASE"] = 0.5f;
    p.parameters["DCA_ATTACK"] = 0.0f; p.parameters["DCA_DECAY"] = 1.5f; p.parameters["DCA_SUSTAIN"] = 0.0f; p.parameters["DCA_RELEASE"] = 1.0f;

    // Filter
    p.parameters["FILTER_CUTOFF"] = 12000.0f;
    p.parameters["FILTER_RESONANCE"] = 0.2f;

    // LFO
    p.parameters["LFO_RATE"] = 6.0f;
    
    // Effects
    p.parameters["DELAY_TIME"] = 0.0f; p.parameters["DELAY_FEEDBACK"] = 0.0f; p.parameters["DELAY_MIX"] = 0.0f;
    p.parameters["REVERB_SIZE"] = 0.9f; p.parameters["REVERB_MIX"] = 0.4f; // Spacey
    
    p.parameters["HARD_SYNC"] = 0.0f;
    p.parameters["RING_MOD"] = 1.0f; // ENABLE RING MOD FOR BELLS
    p.parameters["GLIDE"] = 0.0f;

    presets.push_back(p);
}

void PresetManager::renamePreset(int index, const std::string& newName)
{
    const juce::ScopedWriteLock sl(presetLock);
    if (index >= 0 && index < static_cast<int>(presets.size()))
    {
        presets[index].name = newName;
        
        // If we are renaming the currently active preset, update the currentPreset state too
        if (index == currentPresetIndex)
        {
            currentPreset.name = newName;
        }
        
        autoSaveUserBank();
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
    
    loadPresetFromStruct(p);
}

void PresetManager::loadBank(const juce::File& file)
{
    if (!file.existsAsFile()) return;
    
    juce::var data = juce::JSON::parse(file);
    juce::var presetsArray;
    
    int version = 0; // Default legacy

    // Audit Fix 5.2: Check Version
    if (data.isObject() && data.hasProperty("presets")) {
        version = data["version"];
        if (version < 1) {
            juce::Logger::writeToLog("Warning: Loading old or unknown bank version");
        }
        presetsArray = data["presets"];
    } else if (data.isArray()) {
        presetsArray = data; // Legacy support
    } else {
        return;
    }

    if (!presetsArray.isArray()) return;
    
    presets.clear(); // Important: Clear definition
    
    // We expect 64 presets
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
            
            // Helper to load 8-stage
            auto loadEnv = [&](const juce::var& envVar, EnvelopeData& env) {
                if (auto* obj = envVar.getDynamicObject()) {
                    auto rates = obj->getProperty("rates");
                    auto levels = obj->getProperty("levels");
                    
                    if (rates.isArray() && levels.isArray()) {
                        for (int k=0; k<8; ++k) {
                            float r = static_cast<float>(rates[k]);
                            float l = static_cast<float>(levels[k]);
                            
                            // Audit Fix 5.1: Use Version check for scaled values
                            if (version >= 1 || r > 100.0f) r /= 10000.0f; 
                            if (version >= 1 || l > 10.0f) l /= 10000.0f; 

                            env.rates[k] = r;
                            env.levels[k] = l;
                        }
                    }
                    env.sustainPoint = static_cast<int>(obj->getProperty("sustainPoint"));
                    env.endPoint = static_cast<int>(obj->getProperty("endPoint"));
                }
            };
            
            loadEnv(presetVar["dcwEnv"], p.dcwEnv);
            loadEnv(presetVar["dcaEnv"], p.dcaEnv);
            loadEnv(presetVar["pitchEnv"], p.pitchEnv);
            
            // Also load line 2 if present
            if (presetVar.hasProperty("dcwEnv2")) loadEnv(presetVar["dcwEnv2"], p.dcwEnv2);
            if (presetVar.hasProperty("dcaEnv2")) loadEnv(presetVar["dcaEnv2"], p.dcaEnv2);
            if (presetVar.hasProperty("pitchEnv2")) loadEnv(presetVar["pitchEnv2"], p.pitchEnv2);
            
            presets.push_back(p);
        }
    }
    
    // Ensure 64 slots
    while (presets.size() < 64) {
        presets.push_back(Preset("Init User " + std::to_string(presets.size() + 1)));
    }
    
    // Audit Fix 3.1: Reset index after bank load to prevent offset mismatch
    currentPresetIndex = 0; 
    loadPreset(currentPresetIndex);
}

void PresetManager::resetToFactory()
{
    // Clear existing presets and recreate factory defaults
    presets.clear();
    createFactoryPresets();

    // Ensure we have at least one preset and set it as active
    if (!presets.empty())
    {
        currentPresetIndex = 0;
        currentPreset = presets[0];
        applyPresetToProcessor();
    }
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
    applyPresetToProcessor(); // APVTS
    loadPresetFromStruct(currentPreset); // Full refresh
}


int PresetManager::addPreset(const Preset& p)
{
    const juce::ScopedWriteLock sl(presetLock);
    presets.push_back(p);
    autoSaveUserBank();
    return (int)presets.size() - 1;
}

void PresetManager::deletePreset(int index)
{
    const juce::ScopedWriteLock sl(presetLock);
    if (index >= 0 && index < (int)presets.size())
    {
        presets.erase(presets.begin() + index);
        
        if (presets.empty())
        {
            Preset init;
            init.name = "Init";
            presets.push_back(init);
        }
        
        if (currentPresetIndex >= (int)presets.size())
            currentPresetIndex = (int)presets.size() - 1;
            
        autoSaveUserBank();
    }
}

void PresetManager::movePreset(int fromIndex, int toIndex)
{
    const juce::ScopedWriteLock sl(presetLock);
    int size = (int)presets.size();
    if (fromIndex >= 0 && fromIndex < size && toIndex >= 0 && toIndex < size)
    {
        if (fromIndex == toIndex) return;
        
        auto p = presets[fromIndex];
        presets.erase(presets.begin() + fromIndex);
        presets.insert(presets.begin() + toIndex, p);
        
        if (currentPresetIndex == fromIndex)
            currentPresetIndex = toIndex;
        else if (fromIndex < currentPresetIndex && toIndex >= currentPresetIndex)
            currentPresetIndex--;
        else if (fromIndex > currentPresetIndex && toIndex <= currentPresetIndex)
            currentPresetIndex++;
            
        autoSaveUserBank();
    }
}

} // namespace State
} // namespace CZ101
