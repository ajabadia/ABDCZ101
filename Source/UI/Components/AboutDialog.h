/*
  ==============================================================================

    AboutDialog.h
    Created: 13 Jan 2026
    Description: Simple overlay for plugin information.

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Core/BuildVersion.h"

namespace CZ101 {
namespace UI {

class AboutDialog : public juce::Component
{
public:
    AboutDialog()
    {
        addAndMakeVisible(closeButton);
        closeButton.setButtonText("Close");
        closeButton.onClick = [this] { setVisible(false); };
        
        // Shadow - Removed due to build issues
        // juce::DropShadow ds; ...
    }

    void paint(juce::Graphics& g) override
    {
        // Glassy Background
        g.fillAll(juce::Colours::black.withAlpha(0.8f));
        
        auto area = getLocalBounds().reduced(2);
        
        g.setColour(juce::Colour(0xFF2D2D2D));
        g.fillRoundedRectangle(area.toFloat(), 10.0f);
        
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawRoundedRectangle(area.toFloat(), 10.0f, 1.0f);

        // Content
        g.setColour(juce::Colours::white);
        g.setFont(24.0f);
        g.drawText("ABD Z5001", area.removeFromTop(60), juce::Justification::centred, true);
        
        g.setFont(14.0f);
        g.setColour(juce::Colours::lightgrey);
        g.drawText("Version 1.0.0 (Build " + juce::String(CZ_BUILD_VERSION) + ")", area.removeFromTop(20), juce::Justification::centred, true);
        
        g.setFont(10.0f);
        g.setColour(juce::Colours::grey);
        g.drawText("Compiled on: " + juce::String(CZ_BUILD_TIMESTAMP), area.removeFromTop(20), juce::Justification::centred, true);
        
        g.setFont(14.0f);
        g.setColour(juce::Colours::lightgrey);
        g.drawText("Enhanced Hybrid Phase Distortion Synthesis", area.removeFromTop(30), juce::Justification::centred, true);
        
        g.setColour(juce::Colours::grey);
        g.drawMultiLineText("Designed & Developed by\nABD\n\nBased on Casio CZ Architecture", 
                          area.getX(), area.getY() + 20, area.getWidth(), juce::Justification::centred);
    }

    void resized() override
    {
        closeButton.setBounds(getWidth() / 2 - 40, getHeight() - 40, 80, 24);
    }

private:
    juce::TextButton closeButton;
    // std::unique_ptr<juce::DropShadowEffect> shadowEffect;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutDialog)
};

} // namespace UI
} // namespace CZ101
