#pragma once

#include <JuceHeader.h>
#include "../ScaledComponent.h"
#include "../Components/MacroPanel.h"

namespace CZ101 {
namespace UI {

class GeneralSection : public ScaledComponent,
                     public juce::AudioProcessorValueTreeState::Listener
{
public:
    GeneralSection(CZ101AudioProcessor& p);
    ~GeneralSection() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

private:
    CZ101AudioProcessor& audioProcessor;

    Knob glideKnob;
    Knob masterVolumeKnob;
    
    MacroPanel macroPanel;

    using SliderAttachment = juce::SliderParameterAttachment;
    std::unique_ptr<SliderAttachment> glideAttachment;
    std::unique_ptr<SliderAttachment> masterVolumeAttachment;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GeneralSection)
};

} // namespace UI
} // namespace CZ101
