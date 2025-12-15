#pragma once

#include <string>
#include <vector>
#include <map>

namespace CZ101 {
namespace State {

struct Preset
{
    std::string name;
    std::map<std::string, float> parameters;
    
    Preset() : name("Init") {}
    Preset(const std::string& n) : name(n) {}
};

class Parameters; // Forward declaration

class PresetManager
{
public:
    PresetManager();
    
    void setParameters(Parameters* paramsToUse) { parameters = paramsToUse; }
    
    void loadPreset(int index);
    void savePreset(int index, const std::string& name);
    
    const Preset& getCurrentPreset() const { return currentPreset; }
    const std::vector<Preset>& getPresets() const { return presets; }
    
private:
    std::vector<Preset> presets;
    Preset currentPreset;
    Parameters* parameters = nullptr;
    
    void createFactoryPresets();
    void createInitPreset();
    void createBassPreset();
    void createLeadPreset();
    void createPadPreset();
    void createBrassPreset();
    void createStringsPreset();
    void createBellsPreset();
    void createFXPreset();
};

} // namespace State
} // namespace CZ101
