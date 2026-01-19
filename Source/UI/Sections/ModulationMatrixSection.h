#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../Components/Knob.h"
#include "../ScaledComponent.h"
#include "../../PluginProcessor.h"

namespace CZ101 {
namespace UI {

class ModulationMatrixSection : public ScaledComponent,
                                public juce::AudioProcessorValueTreeState::Listener
{
public:
    ModulationMatrixSection(CZ101AudioProcessor& p);
    ~ModulationMatrixSection() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

private:
    void updateVisibility();
    CZ101AudioProcessor& audioProcessor;

    // --- Modulation Matrix Knobs ---
    Knob veloToDcwKnob;
    Knob veloToDcaKnob;
    Knob wheelToDcwKnob;
    Knob wheelToLfoRateKnob;
    Knob wheelToVibKnob;
    Knob atToDcwKnob;
    Knob atToVibKnob;

    Knob ktDcwKnob;
    Knob ktPitchKnob;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> attachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationMatrixSection)
};

} // namespace UI
} // namespace CZ101
