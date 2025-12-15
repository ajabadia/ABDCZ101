#include "Knob.h"

namespace CZ101 {
namespace UI {

Knob::Knob(const juce::String& name)
    : juce::Slider(name)
{
    setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 14);
    
    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    label.setFont(juce::Font(12.0f));
    addAndMakeVisible(label);
}

void Knob::paint(juce::Graphics& g)
{
    juce::Slider::paint(g);
}

void Knob::resized()
{
    auto bounds = getLocalBounds();
    // Label at Top
    auto labelArea = bounds.removeFromTop(14);
    label.setBounds(labelArea);
    
    // Slider takes the rest (and puts TextBox at bottom)
    juce::Slider::resized();
}

void Knob::setLabel(const juce::String& text)
{
    labelText = text;
    label.setText(text, juce::dontSendNotification);
}

} // namespace UI
} // namespace CZ101
