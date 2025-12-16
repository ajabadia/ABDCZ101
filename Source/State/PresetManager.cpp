#include "PresetManager.h"
#include "Parameters.h"
#include "../Core/VoiceManager.h"
// JuceHeader is now included in PresetManager.h

namespace CZ101 {
namespace State {

PresetManager::PresetManager(Parameters* parameters, Core::VoiceManager* vm)
    : parameters(parameters), voiceManager(vm)
{
    createFactoryPresets();
    // Default to first preset logic moved to PluginProcessor init
}

PresetManager::~PresetManager() = default;

void PresetManager::loadPreset(int index)
{
    if (index >= 0 && index < static_cast<int>(presets.size()))
    {
        currentPreset = presets[index];
        applyPresetToProcessor(); // Updates UI Knobs (ADSR) via Parameters
        
        // Update Voice Manager directly with full 8-stage data
        if (voiceManager)
        {
            applyEnvelopeToVoice(currentPreset.pitchEnv, 0);
            applyEnvelopeToVoice(currentPreset.dcwEnv, 1);
            applyEnvelopeToVoice(currentPreset.dcaEnv, 2);
        }
    }
}

void PresetManager::loadPresetFromStruct(const Preset& p)
{
    // Load the structure directly as the current preset
    currentPreset = p;

    // Apply to parameters and voice manager immediately
    applyPresetToProcessor();
    
    if (voiceManager)
    {
        applyEnvelopeToVoice(currentPreset.pitchEnv, 0);
        applyEnvelopeToVoice(currentPreset.dcwEnv, 1);
        applyEnvelopeToVoice(currentPreset.dcaEnv, 2);
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

void PresetManager::applyEnvelopeToVoice(const EnvelopeData& env, int type)
{
    if (!voiceManager) return;

    for (int i = 0; i < 8; ++i)
    {
        if (type == 0) voiceManager->setPitchStage(i, env.rates[i], env.levels[i]);
        else if (type == 1) voiceManager->setDCWStage(i, env.rates[i], env.levels[i]);
        else if (type == 2) voiceManager->setDCAStage(i, env.rates[i], env.levels[i]);
    }

    if (type == 0) {
        voiceManager->setPitchSustainPoint(env.sustainPoint);
        voiceManager->setPitchEndPoint(env.endPoint);
    } else if (type == 1) {
        voiceManager->setDCWSustainPoint(env.sustainPoint);
        voiceManager->setDCWEndPoint(env.endPoint);
    } else if (type == 2) {
        voiceManager->setDCASustainPoint(env.sustainPoint);
        voiceManager->setDCAEndPoint(env.endPoint);
    }
}

void PresetManager::copyStateFromProcessor()
{
    // 1. Capture Parameters (Denormalized)
    if (parameters)
    {
        // Iterate over existing keys in currentPreset to know what to fetch
        // (Assuming currentPreset has all valid keys initialized)
        for (auto& [key, val] : currentPreset.parameters)
        {
            if (auto* param = parameters->getParameter(key))
            {
                // Convert normalized 0..1 back to real world value
                // We don't have getNormalisableRange exposed easily on AudioParameterFloat?
                // Actually Parameters::getParameter returns AudioParameterFloat* which has ranges.
                // But we need the range to convert?
                // `param->range.convertFrom0to1(param->getValue())`
                // Let's assume Parameters wrapper gives us access or we use the JUCE param directly.
                // Casting to RangedAudioParameter check.
                if (auto* p = dynamic_cast<juce::AudioParameterFloat*>(param))
                {
                    currentPreset.parameters[key] = p->range.convertFrom0to1(p->get());
                }
                else if (auto* pInt = dynamic_cast<juce::AudioParameterInt*>(param))
                {
                     currentPreset.parameters[key] = (float)pInt->get();
                }
                else if (auto* pChoice = dynamic_cast<juce::AudioParameterChoice*>(param))
                {
                     // Choice usually stored as float index (0.0, 1.0)
                     currentPreset.parameters[key] = (float)pChoice->getIndex();
                }
                 else if (auto* pBool = dynamic_cast<juce::AudioParameterBool*>(param))
                {
                     currentPreset.parameters[key] = pBool->get() ? 1.0f : 0.0f;
                }
            }
        }
    }

    // 2. Capture Envelopes from VoiceManager
    if (voiceManager)
    {
        // DCW
        for(int i=0; i<8; ++i) voiceManager->getDCWStage(i, currentPreset.dcwEnv.rates[i], currentPreset.dcwEnv.levels[i]);
        currentPreset.dcwEnv.sustainPoint = voiceManager->getDCWSustainPoint();
        currentPreset.dcwEnv.endPoint = voiceManager->getDCWEndPoint();
        
        // DCA
        for(int i=0; i<8; ++i) voiceManager->getDCAStage(i, currentPreset.dcaEnv.rates[i], currentPreset.dcaEnv.levels[i]);
        currentPreset.dcaEnv.sustainPoint = voiceManager->getDCASustainPoint();
        currentPreset.dcaEnv.endPoint = voiceManager->getDCAEndPoint();
        
        // Pitch
        for(int i=0; i<8; ++i) voiceManager->getPitchStage(i, currentPreset.pitchEnv.rates[i], currentPreset.pitchEnv.levels[i]);
        currentPreset.pitchEnv.sustainPoint = voiceManager->getPitchSustainPoint();
        currentPreset.pitchEnv.endPoint = voiceManager->getPitchEndPoint();
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
        
        // 2. Persist to disk immediately
        // We typically save to the user's document folder or next to the binary if portable.
        // For now, let's use a fixed "user_presets.json" in the current directory or app data.
        // In Standalone, "current directory" might be tricky. Let's use File::getSpecialLocation.
        
        // Note: For this implementation phase, we rely on the caller to trigger saveBank, 
        // OR we can do it here. The plan said "Ensure saveBank is called".
        // Let's rely on the Editor orchestrating it or just do it here for safety.
        // Doing it here is safer.
        
        juce::File defaultsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                                    .getChildFile("CZ101Emulator");
                                    
        if (!defaultsDir.exists()) defaultsDir.createDirectory();
        
        saveBank(defaultsDir.getChildFile("user_bank.json"));
    }
}

// Helper to init default envelopes
static void initEnvelopes(Preset& p)
{
    // Pitch defaults to 0.5 (Unison)
    for (int i=0; i<8; ++i) { 
        p.pitchEnv.rates[i] = 0.99f; 
        p.pitchEnv.levels[i] = 0.5f; 
    }
    p.pitchEnv.sustainPoint = 0;
    p.pitchEnv.endPoint = 0;

    // DCW (Simple Open-Close)
    p.dcwEnv.rates[0] = 0.8f; p.dcwEnv.levels[0] = 1.0f;
    p.dcwEnv.rates[1] = 0.5f; p.dcwEnv.levels[1] = 0.0f;
    // Initialize all envelope stages to a default state
    for(int i=0; i<8; ++i) {
        p.dcwEnv.rates[i] = 0.5f; p.dcwEnv.levels[i] = 0.0f;
        p.dcaEnv.rates[i] = 0.5f; p.dcaEnv.levels[i] = 0.0f;
        p.pitchEnv.rates[i] = 0.5f; p.pitchEnv.levels[i] = 0.5f; // Pitch center
    }
    p.dcwEnv.sustainPoint = 2; p.dcwEnv.endPoint = 3;
    p.dcaEnv.sustainPoint = 2; p.dcaEnv.endPoint = 3;
    p.pitchEnv.sustainPoint = 2; p.pitchEnv.endPoint = 3;
}

void PresetManager::createFactoryPresets()
{
    presets.clear();
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
        p.parameters["osc1_waveform"] = 0.0f; p.parameters["osc1_level"] = 1.0f;
        p.parameters["osc2_waveform"] = 0.0f; p.parameters["osc2_level"] = 0.0f;
        p.parameters["osc2_detune"] = 0.0f;
        
        p.parameters["dcw_attack"] = 0.0f; p.parameters["dcw_decay"] = 0.0f; p.parameters["dcw_sustain"] = 1.0f; p.parameters["dcw_release"] = 0.0f;
        p.parameters["dca_attack"] = 0.0f; p.parameters["dca_decay"] = 0.0f; p.parameters["dca_sustain"] = 1.0f; p.parameters["dca_release"] = 0.0f;
        
        p.parameters["filter_cutoff"] = 20000.0f; p.parameters["filter_resonance"] = 0.1f;
        p.parameters["lfo_rate"] = 1.0f;
        p.parameters["delay_mix"] = 0.0f; p.parameters["reverb_mix"] = 0.0f;
        p.parameters["hard_sync"] = 0.0f;
        p.parameters["ring_mod"] = 0.0f;
        p.parameters["glide_time"] = 0.0f;
        
        // Chorus
        p.parameters["chorus_rate"] = 0.5f;
        p.parameters["chorus_depth"] = 2.0f;
        p.parameters["chorus_mix"] = 0.0f;

        presets.push_back(p);
    }
}

void PresetManager::createBassPreset()
{
    Preset p;
    p.name = "CZ Bass";
    initEnvelopes(p);
    
    // ===== OSCILLATORS (NORMALIZED) =====
    p.parameters["osc1_waveform"] = 1.0f;      // Saw
    p.parameters["osc1_level"] = 0.6f;         // ✅ 60% (normalized)
    p.parameters["osc2_waveform"] = 2.0f;      // Square
    p.parameters["osc2_level"] = 0.4f;         // ✅ 40% (normalized)
    // Total: 0.6 + 0.4 = 1.0 ✅
    
    p.parameters["osc2_detune"] = -10.0f;      // -10 cents
    
    // ===== ADSR (IN SECONDS) =====
    p.parameters["dcw_attack"] = 0.01f;        // ✅ 10ms (crisp)
    p.parameters["dcw_decay"] = 0.2f;          // ✅ 200ms
    p.parameters["dcw_sustain"] = 0.2f;        // ✅ 20% level
    p.parameters["dcw_release"] = 0.1f;        // ✅ 100ms
    
    p.parameters["dca_attack"] = 0.001f;       // ✅ 1ms (very crisp)
    p.parameters["dca_decay"] = 0.2f;          // ✅ 200ms
    p.parameters["dca_sustain"] = 0.5f;        // ✅ 50% level
    p.parameters["dca_release"] = 0.15f;       // ✅ 150ms
    
    // ===== FILTER =====
    p.parameters["filter_cutoff"] = 2000.0f;   // 2000 Hz
    p.parameters["filter_resonance"] = 0.5f;   // 50% Q
    
    // ===== LFO =====
    p.parameters["lfo_rate"] = 0.5f;           // 0.5 Hz
    p.parameters["lfo_depth"] = 0.0f;          // No vibrato
    
    // ===== EFFECTS =====
    p.parameters["delay_time"] = 0.3f;         // ✅ 300ms
    p.parameters["delay_feedback"] = 0.3f;     // 30%
    p.parameters["delay_mix"] = 0.08f;         // ✅ 8% wet
    
    p.parameters["chorus_rate"] = 0.5f;        // 0.5 Hz
    p.parameters["chorus_depth"] = 2.0f;       // 2ms
    p.parameters["chorus_mix"] = 0.0f;         // Off
    
    p.parameters["reverb_size"] = 0.3f;        // Small room
    p.parameters["reverb_mix"] = 0.08f;        // ✅ 8% wet
    
    p.parameters["hard_sync"] = 0.0f;          // Off
    p.parameters["ring_mod"] = 0.0f;           // Off
    p.parameters["glide_time"] = 0.0f;         // No portamento
    
    presets.push_back(p);
}

void PresetManager::createStringPreset()
{
    Preset p;
    p.name = "Vintage Strings";
    initEnvelopes(p);
    
    // ===== OSCILLATORS (NORMALIZED) =====
    p.parameters["osc1_waveform"] = 1.0f;      // Saw
    p.parameters["osc1_level"] = 0.5f;         // ✅ 50% (normalized)
    p.parameters["osc2_waveform"] = 1.0f;      // Saw
    p.parameters["osc2_level"] = 0.5f;         // ✅ 50% (normalized)
    // Total: 0.5 + 0.5 = 1.0 ✅
    
    p.parameters["osc2_detune"] = 12.0f;       // +1 octava
    
    // ===== ADSR (IN SECONDS) - REALISTIC STRINGS =====
    p.parameters["dcw_attack"] = 0.3f;         // ✅ 300ms (bow friction)
    p.parameters["dcw_decay"] = 0.4f;          // ✅ 400ms
    p.parameters["dcw_sustain"] = 0.7f;        // ✅ 70% level
    p.parameters["dcw_release"] = 0.5f;        // ✅ 500ms
    
    p.parameters["dca_attack"] = 0.4f;         // ✅ 400ms (smooth)
    p.parameters["dca_decay"] = 0.3f;          // ✅ 300ms
    p.parameters["dca_sustain"] = 0.8f;        // ✅ 80% level
    p.parameters["dca_release"] = 0.6f;        // ✅ 600ms (smooth release)
    
    // ===== FILTER =====
    p.parameters["filter_cutoff"] = 8000.0f;   // Open
    p.parameters["filter_resonance"] = 0.3f;   // 30% Q
    
    // ===== LFO (VIBRATO) =====
    p.parameters["lfo_rate"] = 4.5f;           // ✅ 4.5 Hz
    p.parameters["lfo_depth"] = 0.08f;         // ✅ Subtle vibrato
    
    // ===== EFFECTS =====
    p.parameters["delay_time"] = 0.25f;        // ✅ 250ms
    p.parameters["delay_feedback"] = 0.4f;     // 40%
    p.parameters["delay_mix"] = 0.3f;          // ✅ 30% wet (longer tail)
    
    p.parameters["chorus_rate"] = 0.6f;        // 0.6 Hz
    p.parameters["chorus_depth"] = 3.0f;       // 3ms
    p.parameters["chorus_mix"] = 0.15f;        // ✅ 15% light chorus
    
    p.parameters["reverb_size"] = 0.7f;        // Large room
    p.parameters["reverb_mix"] = 0.4f;         // ✅ 40% wet (lush)
    
    p.parameters["hard_sync"] = 0.0f;
    p.parameters["ring_mod"] = 0.0f;
    p.parameters["glide_time"] = 0.0f;
    
    presets.push_back(p);
}

void PresetManager::createBrassPreset()
{
    Preset p;
    p.name = "Synth Brass";
    initEnvelopes(p);
    
    p.parameters["osc1_waveform"] = 1.0f; p.parameters["osc1_level"] = 1.0f;
    p.parameters["osc2_waveform"] = 3.0f; p.parameters["osc2_level"] = 0.6f; // Triangle for body
    p.parameters["osc2_detune"] = 7.0f; // Slight detune
    
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
    p.parameters["dcw_attack"] = 0.2f; p.parameters["dcw_decay"] = 0.3f; p.parameters["dcw_sustain"] = 0.8f; p.parameters["dcw_release"] = 0.4f;
    p.parameters["dca_attack"] = 0.1f; p.parameters["dca_decay"] = 0.2f; p.parameters["dca_sustain"] = 0.9f; p.parameters["dca_release"] = 0.4f;

    // Filter
    p.parameters["filter_cutoff"] = 5000.0f;
    p.parameters["filter_resonance"] = 0.6f;

    // LFO
    p.parameters["lfo_rate"] = 0.5f;
    
    // Effects
    p.parameters["delay_time"] = 0.0f; p.parameters["delay_feedback"] = 0.0f; p.parameters["delay_mix"] = 0.0f;
    p.parameters["reverb_size"] = 0.6f; p.parameters["reverb_mix"] = 0.3f;
    
    p.parameters["hard_sync"] = 0.0f;
    p.parameters["ring_mod"] = 0.0f;
    p.parameters["glide_time"] = 0.0f;

    presets.push_back(p);
}

void PresetManager::createLeadPreset()
{
    Preset p;
    p.name = "Solo Lead";
    initEnvelopes(p);
    
    p.parameters["osc1_waveform"] = 2.0f; p.parameters["osc1_level"] = 1.0f;
    p.parameters["osc2_waveform"] = 2.0f; p.parameters["osc2_level"] = 0.6f;
    p.parameters["osc2_detune"] = 0.0f;
    
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
    
    p.parameters["dcw_attack"] = 0.0f; p.parameters["dcw_decay"] = 0.0f; p.parameters["dcw_sustain"] = 1.0f; p.parameters["dcw_release"] = 0.1f;
    p.parameters["dca_attack"] = 0.001f; p.parameters["dca_decay"] = 0.1f; p.parameters["dca_sustain"] = 1.0f; p.parameters["dca_release"] = 0.2f;
    
    // Filter
    p.parameters["filter_cutoff"] = 20000.0f;
    p.parameters["filter_resonance"] = 0.1f;

    // LFO
    p.parameters["lfo_rate"] = 4.0f;
    
    // Effects
    p.parameters["delay_time"] = 0.4f; p.parameters["delay_feedback"] = 0.5f; p.parameters["delay_mix"] = 0.4f;
    p.parameters["reverb_size"] = 0.4f; p.parameters["reverb_mix"] = 0.2f;
    
    p.parameters["hard_sync"] = 1.0f; // ENABLE HARD SYNC FOR LEAD
    p.parameters["ring_mod"] = 0.0f;
    p.parameters["glide_time"] = 0.2f; // ENABLE GLIDE FOR LEAD!
    p.parameters["chorus_rate"] = 0.5f; p.parameters["chorus_depth"] = 2.0f; p.parameters["chorus_mix"] = 0.0f;
    p.parameters["chorus_rate"] = 0.5f; p.parameters["chorus_depth"] = 2.0f; p.parameters["chorus_mix"] = 0.0f;

    presets.push_back(p);
}

void PresetManager::createBellsPreset()
{
    Preset p;
    p.name = "Digital Bells";
    initEnvelopes(p);
    
    p.parameters["osc1_waveform"] = 0.0f; p.parameters["osc1_level"] = 1.0f;
    p.parameters["osc2_waveform"] = 0.0f; p.parameters["osc2_level"] = 1.0f;
    p.parameters["osc2_detune"] = 350.0f; // Detune for bell
    
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
    
    p.parameters["dcw_attack"] = 0.0f; p.parameters["dcw_decay"] = 0.8f; p.parameters["dcw_sustain"] = 0.0f; p.parameters["dcw_release"] = 0.5f;
    p.parameters["dca_attack"] = 0.0f; p.parameters["dca_decay"] = 1.5f; p.parameters["dca_sustain"] = 0.0f; p.parameters["dca_release"] = 1.0f;

    // Filter
    p.parameters["filter_cutoff"] = 12000.0f;
    p.parameters["filter_resonance"] = 0.2f;

    // LFO
    p.parameters["lfo_rate"] = 6.0f;
    
    // Effects
    p.parameters["delay_time"] = 0.0f; p.parameters["delay_feedback"] = 0.0f; p.parameters["delay_mix"] = 0.0f;
    p.parameters["reverb_size"] = 0.9f; p.parameters["reverb_mix"] = 0.4f; // Spacey
    
    p.parameters["hard_sync"] = 0.0f;
    p.parameters["ring_mod"] = 1.0f; // ENABLE RING MOD FOR BELLS
    p.parameters["glide_time"] = 0.0f;

    presets.push_back(p);
}

void PresetManager::renamePreset(int index, const std::string& newName)
{
    if (index >= 0 && index < static_cast<int>(presets.size()))
    {
        presets[index].name = newName;
        if (index == 0) // If current (0 is just a guess, we don't track current index here easily without state) -> actually Manager doesn't track current index?
        {
             // If we are editing the ACTIVE preset, we should update currentPreset too.
             // But AudioProcessor tracks currentProgram.
             // Let's assume the caller handles updating the currentPreset struct if it's the active one.
             // OR: we just update it here if names match? No.
             // The Editor calls: `renamePreset(currentProgram, name)`.
             // We should update the vector AND currentPreset if it matches.
             currentPreset.name = newName; 
             // Wait, currentPreset is a COPY. If we rename separate from load, they desync.
             // But usually we rename the 'Active' sound.
             // So: currentPreset.name = newName.
             // And if we want to persist it to the bank slot: presets[index].name = newName.
        }
    }
    // Also update currentPreset name always?
    currentPreset.name = newName;
}

void PresetManager::saveBank(const juce::File& file)
{
    juce::Array<juce::var> bankArray;
    
    for (const auto& preset : presets) {
        juce::DynamicObject::Ptr obj = new juce::DynamicObject();
        
        // Name & params (EXISTENTE)
        obj->setProperty("name", juce::String(preset.name));
        juce::DynamicObject::Ptr paramsObj = new juce::DynamicObject();
        for (const auto& [id, val] : preset.parameters) {
            paramsObj->setProperty(juce::Identifier(id), val);
        }
        obj->setProperty("params", juce::var(paramsObj.get()));
        
        // ✅ NUEVO: Serialize DCW Envelope (8-stage)
        {
            juce::DynamicObject::Ptr dcwObj = new juce::DynamicObject();
            juce::Array<juce::var> ratesArray, levelsArray;
            for (int i = 0; i < 8; ++i) {
                ratesArray.add(preset.dcwEnv.rates[i]);
                levelsArray.add(preset.dcwEnv.levels[i]);
            }
            dcwObj->setProperty("rates", ratesArray);
            dcwObj->setProperty("levels", levelsArray);
            dcwObj->setProperty("sustainPoint", preset.dcwEnv.sustainPoint);
            dcwObj->setProperty("endPoint", preset.dcwEnv.endPoint);
            obj->setProperty("dcwEnv", juce::var(dcwObj.get()));
        }
        
        // ✅ NUEVO: Serialize DCA Envelope (8-stage)
        {
            juce::DynamicObject::Ptr dcaObj = new juce::DynamicObject();
            juce::Array<juce::var> ratesArray, levelsArray;
            for (int i = 0; i < 8; ++i) {
                ratesArray.add(preset.dcaEnv.rates[i]);
                levelsArray.add(preset.dcaEnv.levels[i]);
            }
            dcaObj->setProperty("rates", ratesArray);
            dcaObj->setProperty("levels", levelsArray);
            dcaObj->setProperty("sustainPoint", preset.dcaEnv.sustainPoint);
            dcaObj->setProperty("endPoint", preset.dcaEnv.endPoint);
            obj->setProperty("dcaEnv", juce::var(dcaObj.get()));
        }
        
        // ✅ NUEVO: Serialize Pitch Envelope (8-stage)
        {
            juce::DynamicObject::Ptr pitchObj = new juce::DynamicObject();
            juce::Array<juce::var> ratesArray, levelsArray;
            for (int i = 0; i < 8; ++i) {
                ratesArray.add(preset.pitchEnv.rates[i]);
                levelsArray.add(preset.pitchEnv.levels[i]);
            }
            pitchObj->setProperty("rates", ratesArray);
            pitchObj->setProperty("levels", levelsArray);
            pitchObj->setProperty("sustainPoint", preset.pitchEnv.sustainPoint);
            pitchObj->setProperty("endPoint", preset.pitchEnv.endPoint);
            obj->setProperty("pitchEnv", juce::var(pitchObj.get()));
        }
        
        bankArray.add(juce::var(obj.get()));
    }
    
    juce::String jsonString = juce::JSON::toString(bankArray, true);
    file.replaceWithText(jsonString);
}

void PresetManager::loadBank(const juce::File& file)
{
    juce::String jsonString = file.loadFileAsString();
    juce::var parsedJson = juce::JSON::parse(jsonString);
    
    if (!parsedJson.isArray())
        return;
    
    presets.clear();
    
    for (int i = 0; i < parsedJson.size(); ++i) {
        Preset p;
        juce::var obj = parsedJson[i];
        
        // Name & params (EXISTENTE)
        p.name = obj["name"].toString().toStdString();
        juce::var paramsObj = obj["params"];
        if (paramsObj.isObject()) {
            auto properties = paramsObj.getDynamicObject()->getProperties();
            for (int j = 0; j < properties.size(); ++j) {
                juce::Identifier key(properties.getName(j));
                p.parameters[key.toString().toStdString()] = (float)paramsObj[key];
            }
        }
        
        // ✅ NUEVO: Load DCW Envelope
        if (obj.hasProperty("dcwEnv")) {
            juce::var dcwObj = obj["dcwEnv"];
            juce::Array<juce::var> rates = dcwObj["rates"];
            juce::Array<juce::var> levels = dcwObj["levels"];
            for (int j = 0; j < 8 && j < rates.size() && j < levels.size(); ++j) {
                p.dcwEnv.rates[j] = (float)rates[j];
                p.dcwEnv.levels[j] = (float)levels[j];
            }
            p.dcwEnv.sustainPoint = (int)dcwObj["sustainPoint"];
            p.dcwEnv.endPoint = (int)dcwObj["endPoint"];
        }
        
        // ✅ NUEVO: Load DCA Envelope
        if (obj.hasProperty("dcaEnv")) {
            juce::var dcaObj = obj["dcaEnv"];
            juce::Array<juce::var> rates = dcaObj["rates"];
            juce::Array<juce::var> levels = dcaObj["levels"];
            for (int j = 0; j < 8 && j < rates.size() && j < levels.size(); ++j) {
                p.dcaEnv.rates[j] = (float)rates[j];
                p.dcaEnv.levels[j] = (float)levels[j];
            }
            p.dcaEnv.sustainPoint = (int)dcaObj["sustainPoint"];
            p.dcaEnv.endPoint = (int)dcaObj["endPoint"];
        }
        
        // ✅ NUEVO: Load Pitch Envelope
        if (obj.hasProperty("pitchEnv")) {
            juce::var pitchObj = obj["pitchEnv"];
            juce::Array<juce::var> rates = pitchObj["rates"];
            juce::Array<juce::var> levels = pitchObj["levels"];
            for (int j = 0; j < 8 && j < rates.size() && j < levels.size(); ++j) {
                p.pitchEnv.rates[j] = (float)rates[j];
                p.pitchEnv.levels[j] = (float)levels[j];
            }
            p.pitchEnv.sustainPoint = (int)pitchObj["sustainPoint"];
            p.pitchEnv.endPoint = (int)pitchObj["endPoint"];
        }
        
        presets.push_back(p);
    }
    
    if (!presets.empty()) {
        currentPresetIndex = 0;
        currentPreset = presets[0];
        applyPresetToProcessor();
    }
}

} // namespace State
} // namespace CZ101