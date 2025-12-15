#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace CZ101 {
namespace UI {

class Knob : public juce::Slider
{
public:
    Knob(const juce::String& name);
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void setLabel(const juce::String& text);
    
private:
    juce::String labelText;
    juce::Label label;
};

} // namespace UI
} // namespace CZ101
