#pragma once

#include <JuceHeader.h>
#include "../Components/Knob.h"
#include "../../PluginProcessor.h"
#include "../ScaledComponent.h" 

namespace CZ101 {
namespace UI {

class FilterLfoSection : public ScaledComponent,
                         public juce::AudioProcessorValueTreeState::Listener
{
public:
    FilterLfoSection(CZ101AudioProcessor& p);
    ~FilterLfoSection() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void updateVisibility();
    void updateSliderValues();

private:
    CZ101AudioProcessor& audioProcessor;


    // LFO Controls
    juce::ComboBox lfoWaveSelector;
    Knob lfoRateKnob, lfoDepthKnob, lfoDelayKnob;
    juce::Label lfoLabel;

    // Attachments
    using SliderAttachment = juce::SliderParameterAttachment;
    using ComboBoxAttachment = juce::ComboBoxParameterAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ComboBoxAttachment>> comboAttachments;
};

} // namespace UI
} // namespace CZ101
