#pragma once

#include <string>
#include <vector>
#include <map>
#include <string>
#include <vector>
#include <map>
#include <juce_core/juce_core.h> // Instead of JuceHeader.h
#include <juce_data_structures/juce_data_structures.h> // For juce::var (if in data_structures) or core
// juce::var is in core usually, let's verify. Yes, juce_core. 
// But let's include both safely.
// Actually juce module headers are guarded.

namespace CZ101 {
namespace State {

// Values for an 8-stage envelope
struct EnvelopeData
{
    float rates[8];
    float levels[8];
    int sustainPoint;
    int endPoint;
    
    EnvelopeData()
    {
        for (int i=0; i<8; ++i) { rates[i] = 0.5f; levels[i] = 0.0f; }
        sustainPoint = 2;
        endPoint = 3;
    }
};

struct Preset
{
    std::string name;
    std::string author; // Added author field
    
    // Parameter map (string ID -> normalized value 0.0-1.0 or specific range)
    std::map<std::string, float> parameters;
    
    // Envelopes
    EnvelopeData pitchEnv;
    EnvelopeData dcwEnv;
    EnvelopeData dcaEnv;

    Preset() : name("Init"), author("Factory") {} 
    Preset(const std::string& n) : name(n), author("Factory") {}
};

class Parameters; 
} // namespace State
namespace Core { class VoiceManager; } // Forward declaration outside State
namespace State {

class PresetManager
{
public:
    PresetManager(Parameters* parameters, Core::VoiceManager* vm);
    ~PresetManager();
    
    void loadPreset(int index);
    void savePreset(int index, const std::string& name);
    void loadPresetFromStruct(const Preset& p); // Load directly (SysEx)
    void copyStateFromProcessor(); // Capture current parameters/envelopes
    
    // Management
    void renamePreset(int index, const std::string& newName);
    void saveBank(const juce::File& file);
    void loadBank(const juce::File& file);
    

    // Reset entire bank to factory defaults
    void resetToFactory();
    void createFactoryPresets(); // Exposed for PluginProcessor fallback
    
    const Preset& getCurrentPreset() const { return currentPreset; }
    const std::vector<Preset>& getPresets() const { return presets; }
    int getCurrentPresetIndex() const { return currentPresetIndex; }
    
    
private:
    std::vector<Preset> presets;
    Preset currentPreset;
    int currentPresetIndex = 0; // Added for tracking
    Parameters* parameters = nullptr;
    Core::VoiceManager* voiceManager = nullptr;
    
    // void createFactoryPresets(); // Moved to public
    void createBassPreset();
    void createLeadPreset();
    void createBrassPreset();
    void createStringPreset();
    void createBellsPreset();
    
    void applyPresetToProcessor();
    // Helper to push 8-stage data to VoiceManager
    void applyEnvelopeToVoice(const EnvelopeData& env, int type); // 0=Pitch, 1=DCW, 2=DCA
};

} // namespace State
} // namespace CZ101
