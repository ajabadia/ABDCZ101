#include "PresetBrowser.h"

namespace CZ101 {
namespace UI {

PresetBrowser::PresetBrowser()
{
    addAndMakeVisible(presetCombo);
    addAndMakeVisible(prevButton);
    addAndMakeVisible(nextButton);
    
    prevButton.setButtonText("<");
    nextButton.setButtonText(">");
    
    // updatePresetList() cannot be called here yet as manager is null
    
    presetCombo.onChange = [this]()
    {
        selectPreset(presetCombo.getSelectedItemIndex());
    };
    
    prevButton.onClick = [this]()
    {
        int current = presetCombo.getSelectedItemIndex();
        if (current > 0)
            presetCombo.setSelectedItemIndex(current - 1);
    };
    
    nextButton.onClick = [this]()
    {
        int current = presetCombo.getSelectedItemIndex();
        if (current < presetCombo.getNumItems() - 1)
            presetCombo.setSelectedItemIndex(current + 1);
    };
}

void PresetBrowser::setPresetManager(State::PresetManager* pm)
{
    presetManager = pm;
    if (presetManager)
        updatePresetList();
}

void PresetBrowser::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2a2a2a));
}

void PresetBrowser::resized()
{
    auto bounds = getLocalBounds().reduced(5);
    
    prevButton.setBounds(bounds.removeFromLeft(30));
    bounds.removeFromLeft(5);
    nextButton.setBounds(bounds.removeFromRight(30));
    bounds.removeFromRight(5);
    
    presetCombo.setBounds(bounds);
}

void PresetBrowser::updatePresetList()
{
    presetCombo.clear();
    
    if (presetManager == nullptr) return;
    
    const auto& presets = presetManager->getPresets();
    for (size_t i = 0; i < presets.size(); ++i)
    {
        presetCombo.addItem(presets[i].name, static_cast<int>(i) + 1);
    }
    
    presetCombo.setSelectedItemIndex(0);
}

void PresetBrowser::selectPreset(int index)
{
    if (presetManager)
        presetManager->loadPreset(index);

    if (onPresetSelected)
        onPresetSelected(index);
}

} // namespace UI
} // namespace CZ101
