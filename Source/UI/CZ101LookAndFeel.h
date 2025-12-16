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
    
    // Custom button drawing with hover effect
    void drawButtonBackground(juce::Graphics& g,
                              juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool isMouseOver,
                              bool isButtonDown) override;
private:
    juce::Colour primaryColour;
    juce::Colour secondaryColour;
    juce::Colour textColour;
};

} // namespace UI
} // namespace CZ101
