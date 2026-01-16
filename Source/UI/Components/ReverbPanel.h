#pragma once

#include <JuceHeader.h>
#include "Knob.h"
#include "../ScaledComponent.h"
#include "../../PluginProcessor.h"

namespace CZ101 {
namespace UI {

class ReverbPanel : public ScaledComponent
{
public:
    ReverbPanel(CZ101AudioProcessor& p);
    void resized() override;

private:
    CZ101AudioProcessor& audioProcessor;
    juce::Label titleLabel;
    Knob sizeKnob, mixKnob;

    using SliderAttachment = juce::SliderParameterAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> attachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbPanel)
};

} // namespace UI
} // namespace CZ101
