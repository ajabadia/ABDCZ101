/*
  ==============================================================================

    LCDDisplay.h
    Created: 15 Dec 2025
    Author:  Antigravity

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace CZ101 {
namespace UI {

class LCDDisplay : public juce::Component
{
public:
    LCDDisplay()
    {
        setOpaque(true);
    }

    void setText(const juce::String& line1, const juce::String& line2)
    {
        m_line1 = line1;
        m_line2 = line2;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Background (Dark Blue/Black)
        g.setFillType(juce::Colour(0xFF101520)); 
        g.fillRoundedRectangle(bounds, 4.0f);
        
        // Inner Bezel / Glow
        g.setColour(juce::Colour(0xFF203040));
        g.drawRoundedRectangle(bounds.reduced(1.0f), 4.0f, 2.0f);
        
        // Text (LCD Blue)
        g.setColour(juce::Colour(0xFF00BFFF));
        g.setFont(juce::FontOptions("Courier New", 16.0f, juce::Font::bold));
        
        // Draw lines
        g.drawText(m_line1, bounds.reduced(10, 0).removeFromTop(bounds.getHeight() / 2), juce::Justification::centredLeft, true);
        g.drawText(m_line2, bounds.reduced(10, 0).removeFromBottom(bounds.getHeight() / 2), juce::Justification::centredLeft, true);
        
        // Imitate pixel grid (optional scanlines)
        g.setColour(juce::Colours::black.withAlpha(0.1f));
        for (int y = 0; y < getHeight(); y += 2)
            g.fillRect(0, y, getWidth(), 1);
    }

    void setPresetName(const juce::String& name)
    {
        // Update line 2 with preset name
        m_line2 = "PRESET: " + name;
        repaint();
    }

    std::function<void()> onClick;

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (onClick) onClick();
    }

private:
    juce::String m_line1 = "CZ-101   CPU: 0.0%";
    juce::String m_line2 = "PRESET: Init";
};

} // namespace UI
} // namespace CZ101
