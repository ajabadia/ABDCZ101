#include "Knob.h"

namespace CZ101 {
namespace UI {

Knob::Knob(const juce::String& name)
    : juce::Slider(name)
{
    setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    
    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(label);
}

void Knob::paint(juce::Graphics& g)
{
    juce::Slider::paint(g);
}

void Knob::resized()
{
    juce::Slider::resized();
    
    auto bounds = getLocalBounds();
    label.setBounds(bounds.removeFromTop(20));
}

void Knob::setLabel(const juce::String& text)
{
    labelText = text;
    label.setText(text, juce::dontSendNotification);
}

} // namespace UI
} // namespace CZ101
