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

#include <juce_audio_processors/juce_audio_processors.h>

namespace CZ101 {
namespace UI {

class LCDDisplay : public ScaledComponent,
                   public juce::ChangeListener,
                   public juce::AudioProcessorValueTreeState::Listener
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
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // Audit Fix [2.2a]: Modern Mode Color
    void setOperationMode(int modeIdx); // 0: 101, 1: 5000, 2: Modern
    // Deprecated wrapper for compatibility if needed internally
    void setModernMode(bool isModern) { setOperationMode(isModern ? 2 : 0); }

private:
    bool modernMode = false;
    int currentModeIndex = 0; // 0=101, 1=5000, 2=Modern
    LCDStateManager* stateManager = nullptr;
    juce::Label topLineLabel;
    juce::Label bottomLineLabel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LCDDisplay)
};

} // namespace UI
} // namespace CZ101
