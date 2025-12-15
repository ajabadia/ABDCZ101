#include "MIDIActivityIndicator.h"

namespace CZ101 {
namespace UI {

MIDIActivityIndicator::MIDIActivityIndicator()
{
    startTimer(30);
}

void MIDIActivityIndicator::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillEllipse(bounds);
    
    if (brightness > 0.0f)
    {
        auto colour = juce::Colour(0xff4a9eff).withAlpha(brightness);
        g.setColour(colour);
        g.fillEllipse(bounds.reduced(2.0f));
    }
}

void MIDIActivityIndicator::timerCallback()
{
    if (isActive)
    {
        brightness = 1.0f;
        isActive = false;
    }
    else
    {
        brightness -= FADE_SPEED;
        if (brightness < 0.0f)
            brightness = 0.0f;
    }
    
    repaint();
}

void MIDIActivityIndicator::triggerActivity()
{
    isActive = true;
}

} // namespace UI
} // namespace CZ101
