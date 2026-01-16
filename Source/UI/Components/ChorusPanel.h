#pragma once

#include <JuceHeader.h>
#include "Knob.h"
#include "../ScaledComponent.h"
#include "../../PluginProcessor.h"

namespace CZ101 {
namespace UI {

class ChorusPanel : public ScaledComponent
{
public:
    ChorusPanel(CZ101AudioProcessor& p);
    void resized() override;

private:
    CZ101AudioProcessor& audioProcessor;
    juce::Label titleLabel;
    Knob rateKnob, depthKnob, mixKnob;

    using SliderAttachment = juce::SliderParameterAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> attachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChorusPanel)
};

} // namespace UI
} // namespace CZ101
