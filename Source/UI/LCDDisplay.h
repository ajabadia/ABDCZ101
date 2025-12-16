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

class LCDDisplay : public juce::Component, private juce::Timer
{
public:
    LCDDisplay()
    {
        setOpaque(true);
        startTimer(3000); // Cycle every 3 seconds
    }

    void setText(const juce::String& cpu, const juce::String& preset)
    {
        m_cpuInfo = cpu;
        m_presetName = preset;
        repaint();
    }
    
    void setSampleRate(double sr) { m_sampleRate = sr; }
    void setLastNote(int note) { m_lastNote = note; }

    void timerCallback() override
    {
        m_carouselIndex = (m_carouselIndex + 1) % 3;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Background (Dark Blue/Black)
        g.setFillType(juce::Colour(0xFF081018)); 
        g.fillRoundedRectangle(bounds, 8.0f); // More rounded
        
        // Inner Bezel / Glow
        g.setColour(juce::Colour(0xFF304050));
        g.drawRoundedRectangle(bounds.reduced(1.0f), 8.0f, 3.0f);
        
        // Text (LCD Bright Blue)
        g.setColour(juce::Colour(0xFF00E0FF)); 
        g.setFont(juce::FontOptions("Courier New", 22.0f, juce::Font::bold)); // Bigger font
        
        juce::String line1, line2;
        
        // Always show Preset on Line 2? 
        // Or cycle?
        // User asked for "Carousel with data".
        
        // Line 1: Main Title or Context
        line1 = "CZ-101 EMULATOR";
        
        // Line 2: Carousel
        switch (m_carouselIndex)
        {
            case 0: // Preset
                line2 = m_presetName;
                break;
            case 1: // Stats
                line2 = m_cpuInfo + "  " + juce::String(m_sampleRate/1000.0, 1) + "k";
                break;
            case 2: // MIDI
                line2 = (m_lastNote > 0) ? "NOTE: " + juce::String(m_lastNote) : "MIDI: Idle";
                break;
        }

        // Draw lines
        auto textField = bounds.reduced(15, 5);
        g.drawText(line1, textField.removeFromTop(bounds.getHeight() / 2), juce::Justification::centred, true);
        g.drawText(line2, textField, juce::Justification::centred, true);
        
        // Imitate pixel grid (scanlines)
        g.setColour(juce::Colours::black.withAlpha(0.15f));
        for (int y = 0; y < getHeight(); y += 3)
            g.fillRect(0, y, getWidth(), 1);
        for (int x = 0; x < getWidth(); x += 3)
            g.fillRect(x, 0, 1, getHeight());
    }

    // Deprecated helpers maintained for compatibility but redirected
    void setPresetName(const juce::String& name) { m_presetName = "PRESET: " + name; repaint(); }

    std::function<void()> onClick;

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (onClick) onClick();
    }

private:
private:
    juce::String m_cpuInfo = "CPU: 0%";
    juce::String m_presetName = "PRESET: Init";
    double m_sampleRate = 44100.0;
    int m_lastNote = -1;
    int m_carouselIndex = 0;
};

} // namespace UI
} // namespace CZ101
