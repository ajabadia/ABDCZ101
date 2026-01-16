#pragma once

#include <JuceHeader.h>
#include "Knob.h"
#include "../ScaledComponent.h"
#include "../../PluginProcessor.h"

namespace CZ101 {
namespace UI {

class DelayPanel : public ScaledComponent
{
public:
    DelayPanel(CZ101AudioProcessor& p);
    void resized() override;

private:
    CZ101AudioProcessor& audioProcessor;
    juce::Label titleLabel;
    Knob timeKnob, feedbackKnob, mixKnob;

    using SliderAttachment = juce::SliderParameterAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> attachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayPanel)
};

} // namespace UI
} // namespace CZ101
