/*
  ==============================================================================

    SkinManager.h
    Created: 13 Jan 2026
    Author:  JUCESynthMaster + UX-SynthDesigner

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "DesignTokens.h"

namespace CZ101 {
namespace UI {

class SkinManager : public juce::ChangeBroadcaster
{
public:
    enum class Theme {
        Dark,
        Light,
        Vintage,
        RetroBeige,
        CyberGlow,
        NeonRetro,
        Steampunk,
        AppleSilicon,
        RetroTerminal
    };

    static SkinManager& getInstance()
    {
        static SkinManager instance;
        return instance;
    }

    void setTheme(Theme t)
    {
        if (currentTheme != t)
        {
            currentTheme = t;
            sendChangeMessage(); // Notify UI
        }
    }

    Theme getTheme() const { return currentTheme; }

    juce::Colour getColour(int colourId) const
    {
        const auto& palette = DesignTokens::Colors::getCurrentPalette((int)currentTheme);

        // Custom Mapping of JUCE IDs to Our Design System
        switch (colourId)
        {
            case juce::DocumentWindow::backgroundColourId:
                return palette.background;
            
            case juce::Label::textColourId:
            case juce::TextButton::textColourOffId:
            case juce::TextButton::textColourOnId: // Added
            case juce::ComboBox::textColourId:
            case juce::GroupComponent::textColourId: 
            case juce::TabbedButtonBar::tabTextColourId:
            case juce::TabbedButtonBar::frontTextColourId: // Added correctly
            case juce::Slider::textBoxTextColourId:
                return palette.textPrimary;
                
            case juce::Slider::textBoxBackgroundColourId:
                return palette.surface;
                
            case juce::Slider::textBoxOutlineColourId:
                return palette.border;

            case juce::Label::outlineColourId:
                return juce::Colours::transparentBlack;

            case juce::TextButton::buttonColourId:
            case juce::ComboBox::backgroundColourId:
            case juce::Slider::rotarySliderOutlineColourId:
                return palette.surface;
                
            case juce::TextButton::buttonOnColourId:
                return palette.accentCyan.withAlpha(0.6f);
                
            case juce::ComboBox::outlineColourId:
            case juce::GroupComponent::outlineColourId: 
            case juce::TabbedButtonBar::tabOutlineColourId:
                return palette.border;
                
            case juce::Slider::thumbColourId:
            case juce::Slider::rotarySliderFillColourId:
                return palette.accentCyan;

            // LCD Specific IDs (using our own convention if needed, or mapping generic)
            case juce::TextEditor::backgroundColourId: // Using TextEditor BG as LCD BG fallback?
                return palette.lcdBg;
            case juce::TextEditor::textColourId:
                return palette.lcdText;

            default:
                return palette.textSecondary; 
        }
    }

    const DesignTokens::Colors::Palette& getCurrentPalette() const {
        return DesignTokens::Colors::getCurrentPalette((int)currentTheme);
    }

private:
    SkinManager() = default; // Singleton
    Theme currentTheme = Theme::Dark; // Default
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkinManager)
};

} // namespace UI
} // namespace CZ101
