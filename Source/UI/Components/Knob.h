#pragma once

#include "ScaledSlider.h"

namespace CZ101 {
namespace UI {

class Knob : public ScaledComponent
{
public:
    Knob(const juce::String& name);
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void lookAndFeelChanged() override;
    
    void setLabel(const juce::String& text);
    
    juce::Slider& getSlider() { return slider; }
    const juce::Slider& getSlider() const { return slider; }
    
private:
    juce::Label label;
    ScaledSlider slider;
};

} // namespace UI
} // namespace CZ101
