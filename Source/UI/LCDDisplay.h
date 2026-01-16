/*
  ==============================================================================

    LCDDisplay.h
    Created: 15 Dec 2025
    Author:  Antigravity

  ==============================================================================
*/

#pragma once

#include "SkinManager.h"
#include "LCDStateManager.h"
#include "ScaledComponent.h"

namespace CZ101 {
namespace UI {

class LCDDisplay : public ScaledComponent,
                   public juce::ChangeListener
{
public:
    LCDDisplay();
    ~LCDDisplay() override;

    void updateSkin();

    void setStateManager(LCDStateManager* mgr);
    
    // Helper to match existing usage in PluginEditor.cpp
    void updateFromManager(LCDStateManager* mgr) { setStateManager(mgr); }

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

private:
    LCDStateManager* stateManager = nullptr;
    juce::Label topLineLabel;
    juce::Label bottomLineLabel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LCDDisplay)
};

} // namespace UI
} // namespace CZ101
