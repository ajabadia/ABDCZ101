#pragma once

#include <JuceHeader.h>
#include "Knob.h"
#include "../ScaledComponent.h"
#include "../../PluginProcessor.h"

namespace CZ101 {
namespace UI {

class MacroPanel : public ScaledComponent
{
public:
    MacroPanel(CZ101AudioProcessor& p);
    
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    CZ101AudioProcessor& audioProcessor;
    
    Knob brillianceKnob;
    Knob toneKnob;
    Knob spaceKnob;

    using SliderAttachment = juce::SliderParameterAttachment;
    std::unique_ptr<SliderAttachment> brillianceAttachment;
    std::unique_ptr<SliderAttachment> toneAttachment;
    std::unique_ptr<SliderAttachment> spaceAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroPanel)
};

} // namespace UI
} // namespace CZ101
