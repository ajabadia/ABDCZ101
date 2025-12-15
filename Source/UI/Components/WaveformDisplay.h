#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace CZ101 {
namespace UI {

class WaveformDisplay : public juce::Component, public juce::Timer
{
public:
    WaveformDisplay();
    
    void paint(juce::Graphics& g) override;
    void timerCallback() override;
    
    void setWaveform(int waveformType);
    
private:
    std::vector<float> waveformData;
    int currentWaveform = 0;
    
    void generateWaveform();
};

} // namespace UI
} // namespace CZ101
