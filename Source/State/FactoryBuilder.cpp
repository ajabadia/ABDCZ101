#include "PresetManager.h"
#include "Parameters.h"
#include "ParameterIDs.h"
#include "FactoryPresets.h"
#include "../MIDI/SysExManager.h"
#include "Presets/preset_helpers.h"

namespace CZ101 {
namespace State {

void PresetManager::createFactoryPresets()
{
    const juce::ScopedWriteLock swl(presetLock);
    presets.clear();
    presets.reserve(FACTORY_PRESET_COUNT);
    
    for (int i = 0; i < FACTORY_PRESET_COUNT; ++i)
    {
        const uint8_t* data = getFactoryPresetData(i);
        if (data != nullptr)
        {
            Preset p;
            if (FACTORY_PRESET_NAMES[i] != nullptr)
                p.name = FACTORY_PRESET_NAMES[i];
            else
                p.name = "Preset " + std::to_string(i + 1);
            p.author = "Casio";
            initEnvelopes(p);
            
            MIDI::SysExManager::decodePatch(data, p);
            
            // Ensure official factory display name is preserved
            if (FACTORY_PRESET_NAMES[i] != nullptr)
                p.name = FACTORY_PRESET_NAMES[i];
                
            presets.push_back(p);
        }
        else
        {
            Preset p;
            p.name = "Init " + std::to_string(i + 1);
            p.author = "Init";
            initEnvelopes(p);
            presets.push_back(p);
        }
    }
}

} // namespace State
} // namespace CZ101
