#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../State/PresetManager.h"

namespace CZ101 {
namespace UI {

class PresetBrowser : public juce::Component
{
public:
    PresetBrowser();
    void setPresetManager(State::PresetManager* pm);
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    std::function<void(int)> onPresetSelected;
    
private:
    State::PresetManager* presetManager = nullptr;
    juce::ComboBox presetCombo;
    juce::TextButton prevButton;
    juce::TextButton nextButton;
    
    void updatePresetList();
    void selectPreset(int index);
};

} // namespace UI
} // namespace CZ101
