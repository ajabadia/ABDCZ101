#include "Knob.h"
#include "../SkinManager.h"
#include "../UIManager.h"

namespace CZ101 {
namespace UI {

Knob::Knob(const juce::String& name)
{
    setName(name);
    
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
    addAndMakeVisible(slider);

    slider.onDragStart = [this]() {
        if (auto* manager = UIManager::findInParentHierarchy(this)) {
            manager->setFocusedParameter(slider.getProperties()["paramId"].toString());
        }
    };
    
    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
    
    lookAndFeelChanged();
}

void Knob::lookAndFeelChanged()
{
    auto& skin = SkinManager::getInstance();
    auto textCol = skin.getColour(juce::Label::textColourId);
    
    label.setColour(juce::Label::textColourId, textCol);
    
    // Refresh font with current scale
    label.setFont(getScaledFont(12.0f).boldened());
    
    // Sync slider colors explicitly
    slider.setColour(juce::Slider::textBoxTextColourId, textCol);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, skin.getColour(juce::Slider::textBoxBackgroundColourId));
    slider.setColour(juce::Slider::textBoxOutlineColourId, skin.getColour(juce::Slider::textBoxOutlineColourId));
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, skin.getColour(juce::Slider::rotarySliderOutlineColourId));
    slider.setColour(juce::Slider::thumbColourId, skin.getColour(juce::Slider::thumbColourId));
    
    // Scale Slider TextBox
    float scale = getUiScale();
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, (int)(50 * scale), (int)(16 * scale));
}

void Knob::paint(juce::Graphics& g)
{
    // Background can be added if needed, but usually composite components are transparent
}

void Knob::resized()
{
    auto area = getLocalBounds();
    float scale = getUiScale();
    
    // Refresh font & textbox on resize
    label.setFont(getScaledFont(12.0f).boldened());
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, (int)(45 * scale), (int)(14 * scale));

    if (label.getText().isEmpty())
    {
        label.setVisible(false);
        slider.setBounds(area);
    }
    else
    {
        label.setVisible(true);
        // Use 20% of height for label, or at least 15px scaled
        int labelH = juce::jmax((int)(15 * scale), (int)(area.getHeight() * 0.2f));
        label.setBounds(area.removeFromTop(labelH));
        
        // Small gap
        area.removeFromTop((int)(2 * scale));
        slider.setBounds(area);
    }
}

void Knob::setLabel(const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
}

} // namespace UI
} // namespace CZ101
