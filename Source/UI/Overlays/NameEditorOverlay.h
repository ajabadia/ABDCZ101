#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace CZ101 {
namespace UI {

class NameEditorOverlay : public juce::Component,
                          private juce::Button::Listener
{
public:
    NameEditorOverlay();
    ~NameEditorOverlay() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Setup
    void startRename(const juce::String& currentName, std::function<void(const juce::String&)> onSaveCallback);
    
    // Listener
    void buttonClicked(juce::Button* b) override;

private:
    juce::Label titleLabel;
    juce::TextEditor nameEditor;
    juce::TextButton saveButton { "SAVE" };
    juce::TextButton cancelButton { "CANCEL" };
    
    std::function<void(const juce::String&)> onSave;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NameEditorOverlay)
};

} // namespace UI
} // namespace CZ101
