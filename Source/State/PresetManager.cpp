#include "PresetManager.h"
#include "Parameters.h"
#include "../Core/VoiceManager.h"

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

void PresetManager::savePreset(int index, const std::string& name)
{
    if (index >= 0 && index < static_cast<int>(presets.size()))
    {
        presets[index] = currentPreset;
        presets[index].name = name;
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
    p.dcwEnv.sustainPoint = 0;
    p.dcwEnv.endPoint = 1; // Loop/End at 1 -> Release? 
    // CZ Logic: EndPoint is where it stops after KeyOff? No, EndPoint is end of Release?
    // Let's use standard ADSR-ish defaults.
    // Stage 0: Attack -> L1. Stage 1: Decay -> SusL. Stage 2: SusHold. Stage 3: Release -> 0.
    
    // DCW Init
    p.dcwEnv.rates[0] = 0.9f; p.dcwEnv.levels[0] = 1.0f;
    p.dcwEnv.rates[1] = 0.5f; p.dcwEnv.levels[1] = 0.5f; // Sustain
    p.dcwEnv.rates[2] = 0.9f; p.dcwEnv.levels[2] = 0.5f; // Hold
    p.dcwEnv.rates[3] = 0.5f; p.dcwEnv.levels[3] = 0.0f; // Release
    p.dcwEnv.sustainPoint = 2;
    p.dcwEnv.endPoint = 3;
    
    // DCA Init
    p.dcaEnv = p.dcwEnv; // Same shape
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
    
    // Oscillators: Saw + Square
    p.parameters["osc1_waveform"] = 1.0f; p.parameters["osc1_level"] = 1.0f;
    p.parameters["osc2_waveform"] = 2.0f; p.parameters["osc2_level"] = 0.8f;
    p.parameters["osc2_detune"] = -10.0f;
    
    // DCW (Plucky)
    p.dcwEnv.rates[0] = 0.95f; p.dcwEnv.levels[0] = 1.0f;
    p.dcwEnv.rates[1] = 0.6f;  p.dcwEnv.levels[1] = 0.2f; // Low sustain
    p.dcwEnv.rates[2] = 0.99f; p.dcwEnv.levels[2] = 0.2f; // Hold
    p.dcwEnv.rates[3] = 0.7f;  p.dcwEnv.levels[3] = 0.0f; // Release
    
    // DCA (Plucky)
    p.dcaEnv.rates[0] = 0.99f; p.dcaEnv.levels[0] = 1.0f;
    p.dcaEnv.rates[1] = 0.6f;  p.dcaEnv.levels[1] = 0.6f;
    p.dcaEnv.rates[2] = 0.99f; p.dcaEnv.levels[2] = 0.6f;
    p.dcaEnv.rates[3] = 0.6f;  p.dcaEnv.levels[3] = 0.0f;
    
    // Set parameters for UI sync
    p.parameters["dcw_attack"] = 0.01f; p.parameters["dcw_decay"] = 0.3f; p.parameters["dcw_sustain"] = 0.2f; p.parameters["dcw_release"] = 0.1f;
    p.parameters["dca_attack"] = 0.001f; p.parameters["dca_decay"] = 0.3f; p.parameters["dca_sustain"] = 0.6f; p.parameters["dca_release"] = 0.2f;

    // Filter
    p.parameters["filter_cutoff"] = 2000.0f;
    p.parameters["filter_resonance"] = 0.5f;

    // LFO
    p.parameters["lfo_rate"] = 0.5f;
    
    // Effects
    p.parameters["delay_time"] = 0.4f; p.parameters["delay_feedback"] = 0.3f; p.parameters["delay_mix"] = 0.1f;
    p.parameters["reverb_size"] = 0.3f; p.parameters["reverb_mix"] = 0.1f;
    
    p.parameters["hard_sync"] = 0.0f; // Off
    p.parameters["ring_mod"] = 0.0f; // Off
    p.parameters["glide_time"] = 0.0f;

    presets.push_back(p);
}

void PresetManager::createStringPreset()
{
    Preset p;
    p.name = "Vintage Strings";
    initEnvelopes(p);
    
    // Osc
    p.parameters["osc1_waveform"] = 1.0f; p.parameters["osc1_level"] = 0.7f; // Saw
    p.parameters["osc2_waveform"] = 1.0f; p.parameters["osc2_level"] = 0.7f; // Saw
    p.parameters["osc2_detune"] = 12.0f;
    
    // DCW: Slow Rise -> Dip -> Sustain
    // Gives a "bowing" friction articulation
    p.dcwEnv.rates[0] = 0.4f; p.dcwEnv.levels[0] = 0.8f; // Slow attack to bright
    p.dcwEnv.rates[1] = 0.3f; p.dcwEnv.levels[1] = 0.6f; // Fade slightly
    p.dcwEnv.rates[2] = 0.99f;p.dcwEnv.levels[2] = 0.6f; // Sustain
    p.dcwEnv.sustainPoint = 2;
    p.dcwEnv.endPoint = 3;
    
    // Release
    p.dcwEnv.rates[3] = 0.3f; p.dcwEnv.levels[3] = 0.0f; // Slow release

    
    // DCA: Similar shape but smoother
    p.dcaEnv = p.dcwEnv; 
    p.dcaEnv.rates[0] = 0.35f; p.dcaEnv.levels[0] = 1.0f;
    p.dcaEnv.rates[1] = 0.4f;  p.dcaEnv.levels[1] = 0.9f;
    p.dcaEnv.rates[2] = 0.99f; p.dcaEnv.levels[2] = 0.9f;
    p.dcaEnv.rates[3] = 0.3f; // Release matches DCW
    
    p.dcaEnv.sustainPoint = 2;
    p.dcaEnv.endPoint = 3;

    p.parameters["dcw_attack"] = 0.5f; p.parameters["dcw_decay"] = 0.5f; p.parameters["dcw_sustain"] = 0.6f; p.parameters["dcw_release"] = 0.8f;
    p.parameters["dca_attack"] = 0.6f; p.parameters["dca_decay"] = 0.5f; p.parameters["dca_sustain"] = 0.9f; p.parameters["dca_release"] = 0.8f;

    // Filter
    p.parameters["filter_cutoff"] = 8000.0f; // Open
    p.parameters["filter_resonance"] = 0.3f;

    // LFO (Vibrato)
    p.parameters["lfo_rate"] = 5.0f;
    p.parameters["lfo_depth"] = 0.1f; // Slight vibrato
    
    // Effects
    p.parameters["delay_time"] = 0.3f; p.parameters["delay_feedback"] = 0.5f; p.parameters["delay_mix"] = 0.4f;
    p.parameters["reverb_size"] = 0.8f; p.parameters["reverb_mix"] = 0.5f; // Lush reverb

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

} // namespace State
} // namespace CZ101