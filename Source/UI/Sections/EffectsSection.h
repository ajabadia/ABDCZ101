#pragma once

#include <JuceHeader.h>
#include "../../PluginProcessor.h"
#include "../ScaledComponent.h"
#include "../Components/FilterPanel.h"
#include "../Components/ChorusPanel.h"
#include "../Components/DelayPanel.h"
#include "../Components/ReverbPanel.h"

namespace CZ101 {
namespace UI {

class EffectsSection : public ScaledComponent,
                       public juce::AudioProcessorValueTreeState::Listener
{
public:
    EffectsSection(CZ101AudioProcessor& p);
    ~EffectsSection() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void updateVisibility();
    void updateSliderValues();

private:
    CZ101AudioProcessor& audioProcessor;

    FilterPanel filterPanel;
    ChorusPanel chorusPanel;
    DelayPanel delayPanel;
    ReverbPanel reverbPanel;

    // Attachments
    using SliderAttachment = juce::SliderParameterAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> attachments;
};

} // namespace UI
} // namespace CZ101
