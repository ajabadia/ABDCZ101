#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace CZ101 {
namespace UI {

class CZ101LookAndFeel : public juce::LookAndFeel_V4
{
public:
    CZ101LookAndFeel();
    
    void drawRotarySlider(juce::Graphics& g,
                         int x, int y, int width, int height,
                         float sliderPos,
                         float rotaryStartAngle,
                         float rotaryEndAngle,
                         juce::Slider& slider) override;
    
    // Simplified LookAndFeel (components use SkinManager directly for theming)


    // Custom button drawing with hover effect
    void drawButtonBackground(juce::Graphics& g,
                              juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool isMouseOver,
                              bool isButtonDown) override;

    void drawGroupComponentOutline(juce::Graphics& g, int width, int height,
                                   const juce::String& text, const juce::Justification& position,
                                   juce::GroupComponent& group) override;

    // --- Specialized Effects ---
    void drawScanlines(juce::Graphics& g, const juce::Rectangle<float>& area, float opacity = 0.1f);
    void applyGlow(juce::Graphics& g, const juce::Path& path, const juce::Colour& colour, float thickness = 2.0f);
    void drawGlassEffect(juce::Graphics& g, const juce::Rectangle<float>& area);
private:
    juce::Colour primaryColour;
    juce::Colour secondaryColour;
    juce::Colour textColour;
};

} // namespace UI
} // namespace CZ101
