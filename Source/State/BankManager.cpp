#include "PresetManager.h"
#include "Parameters.h"
#include "ParameterIDs.h"
#include "../Core/VoiceManager.h"

namespace CZ101 {
namespace State {

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
        applyPresetToProcessor(currentPreset);
    }
}

int PresetManager::addPreset(const Preset& p)
{
    int newIndex = -1;
    {
        const juce::ScopedWriteLock sl(presetLock);
        presets.push_back(p);
        newIndex = (int)presets.size() - 1;
    } 
    
    autoSaveUserBank();
    listeners.call(&Listener::bankUpdated);
    return newIndex;
}

void PresetManager::deletePreset(int index)
{
    bool shouldNotify = false;
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
                
            shouldNotify = true;
        }
    } 
    
    if (shouldNotify)
    {
        autoSaveUserBank();
        listeners.call(&Listener::bankUpdated);
    }
}

void PresetManager::movePreset(int fromIndex, int toIndex)
{
    bool changed = false;
    {
        const juce::ScopedWriteLock sl(presetLock);
        int size = (int)presets.size();
        if (fromIndex >= 0 && fromIndex < size && toIndex >= 0 && toIndex < size)
        {
            if (fromIndex != toIndex)
            {
                auto p = presets[fromIndex];
                presets.erase(presets.begin() + fromIndex);
                presets.insert(presets.begin() + toIndex, p);
                
                if (currentPresetIndex == fromIndex)
                    currentPresetIndex = toIndex;
                else if (fromIndex < currentPresetIndex && toIndex >= currentPresetIndex)
                    currentPresetIndex--;
                else if (fromIndex > currentPresetIndex && toIndex <= currentPresetIndex)
                    currentPresetIndex++;
                    
                changed = true;
            }
        }
    }

    if (changed)
    {
        autoSaveUserBank();
        listeners.call(&Listener::bankUpdated);
    }
}

int PresetManager::getNumPresets() const
{
    const juce::ScopedReadLock sl(presetLock);
    return (int)presets.size();
}

std::string PresetManager::getPresetName(int index) const
{
    const juce::ScopedReadLock sl(presetLock);
    if (index >= 0 && index < (int)presets.size())
        return presets[index].name;
    return "";
}


} // namespace State
} // namespace CZ101
