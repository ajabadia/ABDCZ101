#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h> // For AudioBuffer
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
    void pushBuffer(const juce::AudioBuffer<float>& buffer);
    
private:
    std::vector<float> waveformData;
    int currentWaveform = 0;
    int writePos = 0;
    
    void generateWaveform();
};

} // namespace UI
} // namespace CZ101
