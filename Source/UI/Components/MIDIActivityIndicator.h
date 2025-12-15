#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace CZ101 {
namespace UI {

class MIDIActivityIndicator : public juce::Component, public juce::Timer
{
public:
    MIDIActivityIndicator();
    
    void paint(juce::Graphics& g) override;
    void timerCallback() override;
    
    void triggerActivity();
    
private:
    bool isActive = false;
    float brightness = 0.0f;
    
    static constexpr float FADE_SPEED = 0.1f;
};

} // namespace UI
} // namespace CZ101
