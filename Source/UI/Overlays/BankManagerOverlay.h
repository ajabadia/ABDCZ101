#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../State/PresetManager.h"

namespace CZ101 {
namespace UI {

class BankManagerOverlay : public juce::Component, public juce::ListBoxModel {
public:
    BankManagerOverlay();
    
    void setPresetManager(CZ101::State::PresetManager& manager) { pm = &manager; }
    void setOnUpdate(std::function<void()> callback) { onUpdate = callback; }

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // ListBoxModel Overrides
    int getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent& e) override;

    juce::ListBox& getListBox() { return listBox; }

private:
    CZ101::State::PresetManager* pm = nullptr;
    juce::ListBox listBox { "BankList", this };
    juce::TextButton closeButton { "CLOSE" };
    std::function<void()> onUpdate;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BankManagerOverlay)
};

} // namespace UI
} // namespace CZ101
