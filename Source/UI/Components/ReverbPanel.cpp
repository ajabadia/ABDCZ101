#include "ReverbPanel.h"

namespace CZ101 {
namespace UI {

ReverbPanel::ReverbPanel(CZ101AudioProcessor& p)
    : audioProcessor(p),
      sizeKnob("Size"), mixKnob("Mix")
{
    auto& params = audioProcessor.getParameters();

    addAndMakeVisible(titleLabel);
    titleLabel.setText("Reverb", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);

    auto setupKnob = [&](Knob& k, juce::RangedAudioParameter* param) {
        addAndMakeVisible(k);
        if (param) {
            attachments.emplace_back(std::make_unique<SliderAttachment>(*param, k.getSlider()));
            k.getSlider().getProperties().set("paramId", param->paramID);
        }
    };

    setupKnob(sizeKnob, params.getReverbSize());
    setupKnob(mixKnob, params.getReverbMix());
}

void ReverbPanel::resized()
{
    auto area = getLocalBounds();
    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::column;

    fb.items.add(juce::FlexItem(titleLabel).withFlex(0.5f));
    fb.items.add(juce::FlexItem(sizeKnob).withFlex(1.5f));
    fb.items.add(juce::FlexItem(mixKnob).withFlex(1.5f));

    fb.performLayout(area);
}

} // namespace UI
} // namespace CZ101
