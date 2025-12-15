#include "PresetManager.h"
#include "Parameters.h"

namespace CZ101 {
namespace State {

PresetManager::PresetManager()
{
    createFactoryPresets();
}

void PresetManager::createFactoryPresets()
{
    createInitPreset();
    createBassPreset();
    createLeadPreset();
    createPadPreset();
    createBrassPreset();
    createStringsPreset();
    createBellsPreset();
    createFXPreset();
}

void PresetManager::createInitPreset()
{
    Preset preset("Init");
    preset.parameters["osc1_level"] = 0.5f;
    preset.parameters["osc2_level"] = 0.5f;
    preset.parameters["dca_attack"] = 0.01f;
    preset.parameters["dca_release"] = 0.2f;
    preset.parameters["filter_cutoff"] = 1000.0f;
    presets.push_back(preset);
}

void PresetManager::createBassPreset()
{
    Preset preset("Bass");
    preset.parameters["osc1_level"] = 0.8f;
    preset.parameters["osc2_level"] = 0.2f;
    preset.parameters["osc2_detune"] = -12.0f;
    preset.parameters["filter_cutoff"] = 500.0f;
    preset.parameters["filter_resonance"] = 2.0f;
    preset.parameters["dca_attack"] = 0.001f;
    preset.parameters["dca_decay"] = 0.3f;
    preset.parameters["dca_sustain"] = 0.7f;
    preset.parameters["dca_release"] = 0.1f;
    presets.push_back(preset);
}

void PresetManager::createLeadPreset()
{
    Preset preset("Lead");
    preset.parameters["osc1_level"] = 0.6f;
    preset.parameters["osc2_level"] = 0.4f;
    preset.parameters["osc2_detune"] = 7.0f;
    preset.parameters["filter_cutoff"] = 2000.0f;
    preset.parameters["filter_resonance"] = 1.5f;
    preset.parameters["dca_attack"] = 0.01f;
    preset.parameters["dca_decay"] = 0.2f;
    preset.parameters["dca_sustain"] = 0.6f;
    preset.parameters["dca_release"] = 0.3f;
    preset.parameters["dcw_attack"] = 0.05f;
    preset.parameters["dcw_decay"] = 0.3f;
    presets.push_back(preset);
}

void PresetManager::createPadPreset()
{
    Preset preset("Pad");
    preset.parameters["osc1_level"] = 0.5f;
    preset.parameters["osc2_level"] = 0.5f;
    preset.parameters["osc2_detune"] = 12.0f;
    preset.parameters["filter_cutoff"] = 1500.0f;
    preset.parameters["dca_attack"] = 0.5f;
    preset.parameters["dca_decay"] = 0.8f;
    preset.parameters["dca_sustain"] = 0.7f;
    preset.parameters["dca_release"] = 1.0f;
    preset.parameters["delay_time"] = 0.5f;
    preset.parameters["delay_mix"] = 0.4f;
    presets.push_back(preset);
}

void PresetManager::createBrassPreset()
{
    Preset preset("Brass");
    preset.parameters["osc1_level"] = 0.7f;
    preset.parameters["osc2_level"] = 0.3f;
    preset.parameters["filter_cutoff"] = 1200.0f;
    preset.parameters["filter_resonance"] = 3.0f;
    preset.parameters["dca_attack"] = 0.05f;
    preset.parameters["dca_decay"] = 0.2f;
    preset.parameters["dca_sustain"] = 0.8f;
    preset.parameters["dca_release"] = 0.2f;
    preset.parameters["dcw_attack"] = 0.1f;
    preset.parameters["dcw_decay"] = 0.4f;
    presets.push_back(preset);
}

void PresetManager::createStringsPreset()
{
    Preset preset("Strings");
    preset.parameters["osc1_level"] = 0.5f;
    preset.parameters["osc2_level"] = 0.5f;
    preset.parameters["osc2_detune"] = 5.0f;
    preset.parameters["filter_cutoff"] = 2500.0f;
    preset.parameters["dca_attack"] = 0.3f;
    preset.parameters["dca_decay"] = 0.5f;
    preset.parameters["dca_sustain"] = 0.8f;
    preset.parameters["dca_release"] = 0.8f;
    preset.parameters["delay_time"] = 0.3f;
    preset.parameters["delay_mix"] = 0.2f;
    presets.push_back(preset);
}

void PresetManager::createBellsPreset()
{
    Preset preset("Bells");
    preset.parameters["osc1_level"] = 0.6f;
    preset.parameters["osc2_level"] = 0.4f;
    preset.parameters["osc2_detune"] = 24.0f;
    preset.parameters["filter_cutoff"] = 3000.0f;
    preset.parameters["filter_resonance"] = 2.5f;
    preset.parameters["dca_attack"] = 0.001f;
    preset.parameters["dca_decay"] = 1.0f;
    preset.parameters["dca_sustain"] = 0.3f;
    preset.parameters["dca_release"] = 1.5f;
    preset.parameters["delay_time"] = 0.4f;
    preset.parameters["delay_feedback"] = 0.6f;
    preset.parameters["delay_mix"] = 0.5f;
    presets.push_back(preset);
}

void PresetManager::createFXPreset()
{
    Preset preset("FX Sweep");
    preset.parameters["osc1_level"] = 0.5f;
    preset.parameters["osc2_level"] = 0.5f;
    preset.parameters["osc2_detune"] = 50.0f;
    preset.parameters["filter_cutoff"] = 500.0f;
    preset.parameters["filter_resonance"] = 5.0f;
    preset.parameters["dca_attack"] = 0.1f;
    preset.parameters["dca_release"] = 2.0f;
    preset.parameters["delay_time"] = 0.6f;
    preset.parameters["delay_feedback"] = 0.7f;
    preset.parameters["delay_mix"] = 0.6f;
    preset.parameters["lfo_rate"] = 0.5f;
    presets.push_back(preset);
}

void PresetManager::loadPreset(int index)
{
    if (index >= 0 && index < static_cast<int>(presets.size()))
    {
        currentPreset = presets[index];
        
        // Apply parameters to DSP if connected
        if (parameters != nullptr)
        {
            for (const auto& [paramId, value] : currentPreset.parameters)
            {
                if (auto* param = parameters->getParameter(paramId))
                {
                    // Convert real-world value (e.g., Hz, ms) to normalized 0.0-1.0
                    float normalized = param->convertTo0to1(value);
                    param->setValueNotifyingHost(normalized);
                }
            }
        }
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

} // namespace State
} // namespace CZ101
