#pragma once

#include <JuceHeader.h>
#include "../Components/Knob.h"
#include "../../PluginProcessor.h"
#include "../ScaledComponent.h" // Add include

namespace CZ101 {
namespace UI {

class OscillatorSection : public ScaledComponent // Inherit ScaledComponent instead of GroupComponent
{
public:
    OscillatorSection(CZ101AudioProcessor& p);
    ~OscillatorSection() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void updateToggleStates();

private:
    CZ101AudioProcessor& audioProcessor;
    
    // Controles de DCO1
    juce::ComboBox osc1WaveSelector;
    juce::Label osc1WaveLabel;
    Knob osc1LevelKnob;
    juce::Label osc1LevelLabel;

    // Controles de DCO2
    juce::ComboBox osc2WaveSelector;
    juce::Label osc2WaveLabel;
    Knob osc2LevelKnob;
    juce::Label osc2LevelLabel;
    Knob osc2DetuneKnob;
    juce::Label osc2DetuneLabel;

    // Audit Fix [2.4]: Tone Mix
    Knob lineMixKnob;

    // Controles compartidos
    juce::TextButton hardSyncButton;
    juce::TextButton ringModButton;

    // Attachments
    using ComboBoxAttachment = juce::ComboBoxParameterAttachment;
    using SliderAttachment = juce::SliderParameterAttachment;
    using ButtonAttachment = juce::ButtonParameterAttachment;

    std::unique_ptr<ComboBoxAttachment> osc1WaveAttachment;
    std::unique_ptr<SliderAttachment> osc1LevelAttachment;
    std::unique_ptr<ComboBoxAttachment> osc2WaveAttachment;
    std::unique_ptr<SliderAttachment> osc2LevelAttachment;
    std::unique_ptr<SliderAttachment> osc2DetuneAttachment;
    std::unique_ptr<SliderAttachment> lineMixAttachment; // [NEW]
    std::unique_ptr<ButtonAttachment> hardSyncAttachment;
    std::unique_ptr<ButtonAttachment> ringModAttachment;
};

} // namespace UI
} // namespace CZ101
