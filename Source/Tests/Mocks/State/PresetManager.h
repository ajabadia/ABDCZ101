#pragma once
#include <string>
#include <vector>
#include <map>

// Structs must match original
namespace CZ101 {
namespace State {

struct EnvelopeData
{
    float rates[8];
    float levels[8];
    int sustainPoint;
    int endPoint;
    EnvelopeData() {
        for (int i=0; i<8; ++i) { rates[i] = 0.5f; levels[i] = 0.0f; }
        sustainPoint = 2; endPoint = 3;
    }
};

struct Preset
{
    std::string name;
    std::map<std::string, float> parameters;
    EnvelopeData dcwEnv;
    EnvelopeData dcaEnv;
    EnvelopeData pitchEnv;
    Preset() : name("Init") {}
    Preset(const std::string& n) : name(n) {}
};

class Parameters; 
} 
namespace Core { class VoiceManager; } 
namespace State {

class PresetManager
{
public:
    PresetManager(Parameters* parameters, Core::VoiceManager* vm);
    ~PresetManager(); // Add destr
    
    void loadPreset(int index);
    void savePreset(int index, const std::string& name);
    void loadPresetFromStruct(const Preset& p); // Load directly (SysEx)
    
    // Stub getters
    const Preset& getCurrentPreset() const { return currentPreset; }
    const std::vector<Preset>& getPresets() const { return presets; }
    
private:
    std::vector<Preset> presets;
    Preset currentPreset;
    
    void createFactoryPresets();
    void createBassPreset();
    void createLeadPreset();
    void createBrassPreset();
    void createStringPreset();
    void createBellsPreset();
    
    void applyPresetToProcessor();
    void applyEnvelopeToVoice(const EnvelopeData& env, int type);
};

} 
} 
