#pragma once

#include <JuceHeader.h>
#include "Knob.h"
#include "../ScaledComponent.h"
#include "../../PluginProcessor.h"

namespace CZ101 {
namespace UI {

class ArpeggiatorPanel : public ScaledComponent
{
public:
    ArpeggiatorPanel(CZ101AudioProcessor& p);
    
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    CZ101AudioProcessor& audioProcessor;
    
    Knob arpRateKnob;
    Knob arpBpmKnob;
    Knob arpGateKnob;
    Knob arpSwingKnob;
    Knob arpPatternKnob;
    juce::TextButton arpLatchButton { "LATCH" };
    juce::TextButton arpEnableButton { "ARP ON" };

    using SliderAttachment = juce::SliderParameterAttachment;
    std::unique_ptr<SliderAttachment> arpRateAttachment;
    std::unique_ptr<SliderAttachment> arpBpmAttachment;
    std::unique_ptr<SliderAttachment> arpGateAttachment;
    std::unique_ptr<SliderAttachment> arpSwingAttachment;
    std::unique_ptr<SliderAttachment> arpPatternAttachment;
    
    using ButtonAttachment = juce::ButtonParameterAttachment;
    std::unique_ptr<ButtonAttachment> arpEnableAttachment;
    std::unique_ptr<ButtonAttachment> arpLatchAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArpeggiatorPanel)
};

} // namespace UI
} // namespace CZ101
