#include "CZ101LookAndFeel.h"

namespace CZ101 {
namespace UI {

CZ101LookAndFeel::CZ101LookAndFeel()
{
    primaryColour = juce::Colour(0xff2a2a2a);
    secondaryColour = juce::Colour(0xff4a9eff);
    textColour = juce::Colours::white;
    
    setColour(juce::Slider::thumbColourId, secondaryColour);
    setColour(juce::Slider::rotarySliderFillColourId, secondaryColour);
    setColour(juce::Slider::rotarySliderOutlineColourId, primaryColour);
}

void CZ101LookAndFeel::drawRotarySlider(juce::Graphics& g,
                                       int x, int y, int width, int height,
                                       float sliderPos,
                                       float rotaryStartAngle,
                                       float rotaryEndAngle,
                                       juce::Slider& slider)
{
    juce::ignoreUnused(slider);
    auto radius = juce::jmin(width / 2, height / 2) - 4.0f;
    auto centreX = x + width * 0.5f;
    auto centreY = y + height * 0.5f;
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    
    // Fill
    g.setColour(primaryColour);
    g.fillEllipse(rx, ry, rw, rw);
    
    // Outline
    g.setColour(secondaryColour);
    g.drawEllipse(rx, ry, rw, rw, 2.0f);
    
    // Pointer
    juce::Path p;
    auto pointerLength = radius * 0.6f;
    auto pointerThickness = 3.0f;
    p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
    p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
    
    g.setColour(secondaryColour);
    g.fillPath(p);
}
void CZ101LookAndFeel::drawButtonBackground(juce::Graphics& g,
                                        juce::Button& button,
                                        const juce::Colour& backgroundColour,
                                        bool isMouseOver,
                                        bool isButtonDown)
{
    // Base background
    g.setColour(backgroundColour);
    auto bounds = button.getLocalBounds().toFloat();

    // Hover scaling effect
    if (isMouseOver && !isButtonDown)
    {
        // Slightly enlarge the button background (2% increase)
        auto scale = 1.02f;
        auto centre = bounds.getCentre();
        bounds = bounds.withSizeKeepingCentre(bounds.getWidth() * scale,
                                             bounds.getHeight() * scale);
    }
    else if (isButtonDown)
    {
        // Slightly shrink when pressed (2% decrease)
        auto scale = 0.98f;
        auto centre = bounds.getCentre();
        bounds = bounds.withSizeKeepingCentre(bounds.getWidth() * scale,
                                             bounds.getHeight() * scale);
    }

    // Draw rounded rectangle for button background
    g.fillRoundedRectangle(bounds, 4.0f);
}

} // namespace UI
} // namespace CZ101
