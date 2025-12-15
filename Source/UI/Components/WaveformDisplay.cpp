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

void WaveformDisplay::pushBuffer(const juce::AudioBuffer<float>& buffer)
{
    // Simple circular buffer or just grab the first channel's chunk for visualization
    auto* channelData = buffer.getReadPointer(0);
    int numSamples = buffer.getNumSamples();
    
    for (int i = 0; i < numSamples; ++i)
    {
        // Simple ring buffer push
        waveformData[writePos] = channelData[i];
        writePos = (writePos + 1) % waveformData.size();
    }
}

void WaveformDisplay::generateWaveform() 
{
    // No-op or clear, as we use live data now
}

} // namespace UI
} // namespace CZ101
