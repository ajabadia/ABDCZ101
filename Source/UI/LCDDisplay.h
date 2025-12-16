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
        startTimerHz(60); // 60 FPS for smooth animation
    }

    void setText(const juce::String& cpu, const juce::String& preset)
    {
        m_cpuInfo = cpu;
        m_presetName = preset;
        // Don't repaint here unconditionally to avoid interfering with animation timing, 
        // but updating data is fine.
    }
    
    void setSampleRate(double sr) { m_sampleRate = sr; }
    void setLastNote(int note) { m_lastNote = note; }

    void timerCallback() override
    {
        if (m_transitionProgress < 1.0f)
        {
            m_transitionProgress += 0.05f;
            if (m_transitionProgress >= 1.0f) {
                m_transitionProgress = 1.0f;
                m_currentIndex = m_nextIndex;
            }
            repaint();
        }
        else
        {
            m_framesCounts++;
            if (m_framesCounts > 240) // 4 seconds hold
            {
                m_framesCounts = 0;
                m_nextIndex = (m_currentIndex + 1) % 3;
                m_transitionProgress = 0.0f;
                repaint();
            }
        }
    }
    
    juce::String getTextForIndex(int index)
    {
        switch (index)
        {
            case 0: return m_presetName;
            case 1: return m_cpuInfo + "  " + juce::String(m_sampleRate/1000.0, 1) + "k";
            case 2: return (m_lastNote > 0) ? "NOTE: " + juce::String(m_lastNote) : "MIDI: Idle";
            default: return {};
        }
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Background (Dark Blue/Black)
        g.setFillType(juce::Colour(0xFF081018)); 
        g.fillRoundedRectangle(bounds, 8.0f); 
        
        // Inner Bezel / Glow
        g.setColour(juce::Colour(0xFF304050));
        g.drawRoundedRectangle(bounds.reduced(1.0f), 8.0f, 3.0f);
        
        // Text (LCD Bright Blue)
        g.setColour(juce::Colour(0xFF00E0FF)); 
        g.setFont(juce::FontOptions("Courier New", 22.0f, juce::Font::bold));
        
        auto textField = bounds.reduced(15, 5);
        auto line1Rect = textField.removeFromTop(bounds.getHeight() / 2);
        
        // Line 1: Header
        g.drawText("CZ-101 EMULATOR", line1Rect, juce::Justification::centred, true);
        
        // Line 2: Carousel with Clip
        juce::Graphics::ScopedSaveState s(g);
        g.reduceClipRegion(textField.toNearestInt());
        
        if (m_transitionProgress >= 1.0f)
        {
            g.drawText(getTextForIndex(m_currentIndex), textField, juce::Justification::centred, true);
        }
        else
        {
            float w = bounds.getWidth();
            // Old slide out to Left
            g.drawText(getTextForIndex(m_currentIndex), 
                       textField.translated(-w * m_transitionProgress, 0), 
                       juce::Justification::centred, true);
                       
            // New slide in from Right
            g.drawText(getTextForIndex(m_nextIndex), 
                       textField.translated(w * (1.0f - m_transitionProgress), 0), 
                       juce::Justification::centred, true);
        }
        
        // Restore context to draw grid over everything? 
        // Or keep grid under? Original code had grid over.
        // Let's draw grid manually here if we want it on top (after ScopedSaveChanges)
    }

    // Deprecated helpers maintained for compatibility but redirected
    void setPresetName(const juce::String& name) { m_presetName = "PRESET: " + name; repaint(); }

    std::function<void()> onClick;

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (onClick) onClick();
    }

private:
    juce::String m_cpuInfo = "CPU: 0%";
    juce::String m_presetName = "PRESET: Init";
    double m_sampleRate = 44100.0;
    int m_lastNote = -1;
    
    // Animation State
    int m_currentIndex = 0;
    int m_nextIndex = 0;
    float m_transitionProgress = 1.0f;
    int m_framesCounts = 0;
};

} // namespace UI
} // namespace CZ101
