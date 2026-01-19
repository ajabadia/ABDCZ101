#pragma once

#include <JuceHeader.h>
#include "../Components/Knob.h"
#include "../ScaledComponent.h"
#include "../../PluginProcessor.h"

namespace CZ101 {
namespace UI {

class ArpeggiatorSection : public ScaledComponent,
                           public juce::AudioProcessorValueTreeState::Listener
{
public:
    ArpeggiatorSection(CZ101AudioProcessor& p);
    ~ArpeggiatorSection() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Audit Fix [2.2]: Explicit UI Refresh
    void refreshFromAPVTS();

    // Listener callback
    void parameterChanged(const juce::String& parameterID, float newValue) override;

private:
    void updateVisibility();

    CZ101AudioProcessor& audioProcessor;
    
    Knob arpRateKnob;
    Knob arpBpmKnob;
    Knob arpGateKnob;
    Knob arpSwingKnob;
    Knob arpPatternKnob;
    Knob arpOctaveKnob;
    juce::ComboBox arpSwingModeCombo { "Swing Mode" };
    juce::TextButton arpLatchButton { "LATCH" };
    juce::TextButton arpEnableButton { "ARP ON" };
    
    juce::Label rateLabel { {}, "RATE" };
    juce::Label tempoLabel { {}, "TEMPO" };
    juce::Label patternLabel { {}, "PATTERN" };
    juce::Label octLabel { {}, "OCTAVES" };
    juce::Label gateLabel { {}, "GATE" };
    juce::Label swingLabel { {}, "SWING" };
    juce::Label typeLabel { {}, "MODE" };

    juce::Label classicWarningLabel { {}, "ARPEGGIATOR IS DISABLED IN CLASSIC 101 MODE" };

    using SliderAttachment = juce::SliderParameterAttachment;
    std::unique_ptr<SliderAttachment> arpRateAttachment;
    std::unique_ptr<SliderAttachment> arpBpmAttachment;
    std::unique_ptr<SliderAttachment> arpGateAttachment;
    std::unique_ptr<SliderAttachment> arpSwingAttachment;
    
    using ComboAttachment = juce::ComboBoxParameterAttachment;
    std::unique_ptr<ComboAttachment> arpSwingModeAttachment;
    
    std::unique_ptr<SliderAttachment> arpPatternAttachment;
    std::unique_ptr<SliderAttachment> arpOctaveAttachment;
    
    using ButtonAttachment = juce::ButtonParameterAttachment;
    std::unique_ptr<ButtonAttachment> arpEnableAttachment;
    std::unique_ptr<ButtonAttachment> arpLatchAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArpeggiatorSection)
};

} // namespace UI
} // namespace CZ101
