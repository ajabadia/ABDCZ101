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
        // Audit Fix 10.3: Default Gate Shape (Prevent silence)
        for (int i=0; i<8; ++i) { rates[i] = 50.0f; levels[i] = 0.0f; }
        levels[0] = 1.0f; // Stage 0 Target = Full Level
        // Stage 1 Target = 0.0f (Release)
        sustainPoint = 0; // Sustain at end of Stage 0
        endPoint = 1;     // End at end of Stage 1
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

class PresetManager : public juce::ChangeBroadcaster
{
public:
    static constexpr int MAX_PRESETS = 64;
    static constexpr const char* USER_BANK_FILENAME = "user_bank.json";
    
    // Observer Interface
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void presetLoaded(int index) {}
        virtual void bankUpdated() {}
        virtual void presetRenamed(int index, const std::string& newName) {}
    };
    
    void addListener(Listener* l);
    void removeListener(Listener* l);
    
    PresetManager(Parameters* parameters, Core::VoiceManager* vm);
    ~PresetManager();
    
    void loadPreset(int index, bool updateVoice = true);
    void savePreset(int index, const std::string& name);
    void loadPresetFromStruct(const Preset& p, bool updateVoice = true, bool notifyHost = true); 
    void copyStateFromProcessor(); 
    void applyPresetToProcessor(const Preset& p);
    
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
    const std::vector<Preset>& getPresets() const { return presets; } // Warning: Not thread-safe without calling getLock()
    int getCurrentPresetIndex() const { return currentPresetIndex; }
    
    // Thread-Safe Accessors (Audit Fix)
    int getNumPresets() const;
    std::string getPresetName(int index) const;
    
    // Compare Support
    void setCompareMode(bool enabled);
    bool isComparisonActive() const { return isComparing; }
    
    
private:
    std::vector<Preset> presets;
    Preset currentPreset;
    int currentPresetIndex = 0; // Added for tracking
    Parameters* parameters = nullptr;
    Core::VoiceManager* voiceManager = nullptr;
    juce::ReadWriteLock presetLock;
    
    // Compare State
    bool isComparing = false;
    
    juce::ListenerList<Listener> listeners;
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
