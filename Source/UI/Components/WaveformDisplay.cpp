#include "WaveformDisplay.h"
#include <cmath>

namespace CZ101 {
namespace UI {

WaveformDisplay::WaveformDisplay()
{
    waveformData.resize(256);
    generateWaveform();
    startTimer(50);
}

void WaveformDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    g.fillAll(juce::Colour(0xff1a1a1a));
    
    g.setColour(juce::Colour(0xff2a2a2a));
    g.drawRect(bounds, 2.0f);
    
    juce::Path path;
    auto width = bounds.getWidth();
    auto height = bounds.getHeight();
    auto centerY = bounds.getCentreY();
    
    path.startNewSubPath(bounds.getX(), centerY);
    
    for (size_t i = 0; i < waveformData.size(); ++i)
    {
        float x = bounds.getX() + (i / static_cast<float>(waveformData.size())) * width;
        float y = centerY - waveformData[i] * (height * 0.4f);
        path.lineTo(x, y);
    }
    
    g.setColour(juce::Colour(0xff4a9eff));
    g.strokePath(path, juce::PathStrokeType(2.0f));
}

void WaveformDisplay::timerCallback()
{
    repaint();
}

void WaveformDisplay::setWaveform(int waveformType)
{
    currentWaveform = waveformType;
    generateWaveform();
}

void WaveformDisplay::generateWaveform()
{
    constexpr float TWO_PI = 6.28318530718f;
    
    for (size_t i = 0; i < waveformData.size(); ++i)
    {
        float phase = i / static_cast<float>(waveformData.size());
        
        switch (currentWaveform)
        {
            case 0: // Sine
                waveformData[i] = std::sin(TWO_PI * phase);
                break;
            case 1: // Sawtooth
                waveformData[i] = 2.0f * phase - 1.0f;
                break;
            case 2: // Square
                waveformData[i] = (phase < 0.5f) ? 1.0f : -1.0f;
                break;
            case 3: // Triangle
                if (phase < 0.25f)
                    waveformData[i] = 4.0f * phase;
                else if (phase < 0.75f)
                    waveformData[i] = 2.0f - 4.0f * phase;
                else
                    waveformData[i] = 4.0f * phase - 4.0f;
                break;
            default:
                waveformData[i] = 0.0f;
        }
    }
}

} // namespace UI
} // namespace CZ101
