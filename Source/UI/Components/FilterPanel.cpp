#include "FilterPanel.h"

namespace CZ101 {
namespace UI {

FilterPanel::FilterPanel(CZ101AudioProcessor& p)
    : audioProcessor(p),
      lpfCutoffKnob("Freq"), lpfResoKnob("Reso"), hpfCutoffKnob("HPF")
{
    auto& params = audioProcessor.getParameters();

    addAndMakeVisible(titleLabel);
    titleLabel.setText("Filters", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);

    auto setupKnob = [&](Knob& k, juce::RangedAudioParameter* param) {
        addAndMakeVisible(k);
        if (param) {
            attachments.emplace_back(std::make_unique<SliderAttachment>(*param, k.getSlider()));
            k.getSlider().getProperties().set("paramId", param->paramID);
        }
    };

    setupKnob(lpfCutoffKnob, params.getModernLpfCutoff());
    setupKnob(lpfResoKnob, params.getModernLpfReso());
    setupKnob(hpfCutoffKnob, params.getModernHpfCutoff());
}

void FilterPanel::resized()
{
    auto area = getLocalBounds();
    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::column;

    fb.items.add(juce::FlexItem(titleLabel).withFlex(0.5f));
    fb.items.add(juce::FlexItem(lpfCutoffKnob).withFlex(1.5f));
    fb.items.add(juce::FlexItem(lpfResoKnob).withFlex(1.5f));
    fb.items.add(juce::FlexItem(hpfCutoffKnob).withFlex(1.5f));

    fb.performLayout(area);
}

} // namespace UI
} // namespace CZ101
