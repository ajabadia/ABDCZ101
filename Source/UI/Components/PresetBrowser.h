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
    std::function<void()> onSaveRequested;
    
    // Exposed for Editor
    void updatePresetList();
    int getSelectedItemIndex() const;
    void setSelectedItemIndex(int index);
    
private:
    State::PresetManager* presetManager = nullptr;
    juce::ComboBox presetCombo;
    juce::TextButton prevButton;
    juce::TextButton nextButton;
    juce::TextButton saveButton;
    
    // Bank Management
    juce::TextButton menuButton;
    std::unique_ptr<juce::FileChooser> fileChooser;
    
    void showMenu();
    void initBank();
    void loadBank();
    void saveBank();
    
    // void updatePresetList(); // Moved to public
    void selectPreset(int index);
};

} // namespace UI
} // namespace CZ101
