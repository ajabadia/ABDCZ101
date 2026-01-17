#pragma once

#include <string>
#include <vector>
#include <map>
#include <string>
#include <vector>
#include <map>
#include <juce_core/juce_core.h> // Instead of JuceHeader.h
// #include <JuceHeader.h> // Fallback if juce_core is not enough
// Actually, let's include the full header to avoid 'not a member' weirdness
#include <JuceHeader.h>

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
    
    // Envelopes Line 1
    EnvelopeData pitchEnv;
    EnvelopeData dcwEnv;
    EnvelopeData dcaEnv;

    // Envelopes Line 2
    EnvelopeData pitchEnv2;
    EnvelopeData dcwEnv2;
    EnvelopeData dcaEnv2;

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
    static constexpr int MAX_PRESETS = 64;
    static constexpr const char* USER_BANK_FILENAME = "user_bank.json";
    
    PresetManager(Parameters* parameters, Core::VoiceManager* vm);
    ~PresetManager();
    
    void loadPreset(int index, bool updateVoice = true);
    void savePreset(int index, const std::string& name);
    void loadPresetFromStruct(const Preset& p, bool updateVoice = true); // Load directly (SysEx)
    void copyStateFromProcessor(); // Capture current parameters/envelopes
    
    // Thread safety for preset operations
    juce::ReadWriteLock& getLock() { return presetLock; }
    
    // Management
    void renamePreset(int index, const std::string& newName);
    void saveBank(const juce::File& file);
    void loadBank(const juce::File& file);
    
    void savePresetToFile(int index, const juce::File& file);
    void loadPresetFromFile(const juce::File& file);
    
    // Bank management
    int addPreset(const Preset& p);          // Appends to bank, returns index
    void deletePreset(int index);            // Removes from bank
    void movePreset(int fromIndex, int toIndex); // Reorders
    

    // Reset entire bank to factory defaults
    void resetToFactory();
    void createFactoryPresets(); // Exposed for PluginProcessor fallback
    
    const Preset& getCurrentPreset() const { return currentPreset; }
    const std::vector<Preset>& getPresets() const { return presets; }
    int getCurrentPresetIndex() const { return currentPresetIndex; }
    
    // Compare Support
    void beginCompare();
    void endCompare();
    bool isComparisonActive() const { return comparing; }
    
    
private:
    std::vector<Preset> presets;
    Preset currentPreset;
    int currentPresetIndex = 0; // Added for tracking
    Parameters* parameters = nullptr;
    Core::VoiceManager* voiceManager = nullptr;
    juce::ReadWriteLock presetLock;
    
    // Compare State
    bool comparing = false;
    Preset compareBuffer;
    
    // void createFactoryPresets(); // Moved to public
    void createBassPreset();
    void createLeadPreset();
    void createBrassPreset();
    void createStringPreset();
    void createBellsPreset();
    
    void applyPresetToProcessor();
    // Helper to push 8-stage data to VoiceManager
    void applyEnvelopeToVoice(const EnvelopeData& env, int type, int line); // 0=Pitch, 1=DCW, 2=DCA, line=1/2
    void autoSaveUserBank();

public:
    // Helper for PluginProcessor State
    std::unique_ptr<juce::XmlElement> exportEnvelopesToXml();
    void importEnvelopesFromXml(const juce::XmlElement& xml);
};

} // namespace State
} // namespace CZ101
