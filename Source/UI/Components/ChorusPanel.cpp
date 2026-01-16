#include "ChorusPanel.h"

namespace CZ101 {
namespace UI {

ChorusPanel::ChorusPanel(CZ101AudioProcessor& p)
    : audioProcessor(p),
      rateKnob("Rate"), depthKnob("Depth"), mixKnob("Mix")
{
    auto& params = audioProcessor.getParameters();

    addAndMakeVisible(titleLabel);
    titleLabel.setText("Chorus", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);

    auto setupKnob = [&](Knob& k, juce::RangedAudioParameter* param) {
        addAndMakeVisible(k);
        if (param) {
            attachments.emplace_back(std::make_unique<SliderAttachment>(*param, k.getSlider()));
            k.getSlider().getProperties().set("paramId", param->paramID);
        }
    };

    setupKnob(rateKnob, params.getChorusRate());
    setupKnob(depthKnob, params.getChorusDepth());
    setupKnob(mixKnob, params.getChorusMix());
}

void ChorusPanel::resized()
{
    auto area = getLocalBounds();
    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::column;

    fb.items.add(juce::FlexItem(titleLabel).withFlex(0.5f));
    fb.items.add(juce::FlexItem(rateKnob).withFlex(1.5f));
    fb.items.add(juce::FlexItem(depthKnob).withFlex(1.5f));
    fb.items.add(juce::FlexItem(mixKnob).withFlex(1.5f));

    fb.performLayout(area);
}

} // namespace UI
} // namespace CZ101
