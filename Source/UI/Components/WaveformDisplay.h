#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <JuceHeader.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ScaledComponent.h"
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
    
    // Interaction
    void setProcessor(juce::AudioProcessor* p) { processor = p; }
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    
private:
    std::vector<float> waveformData;
    int currentWaveform = 0;
    int writePos = 0;
    
    juce::AudioProcessor* processor = nullptr;
    float startValX = 0.0f;
    float startValY = 0.0f; // For drag deltas
    
    void generateWaveform();
};

} // namespace UI
} // namespace CZ101
