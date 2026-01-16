#include "DelayPanel.h"

namespace CZ101 {
namespace UI {

DelayPanel::DelayPanel(CZ101AudioProcessor& p)
    : audioProcessor(p),
      timeKnob("Time"), feedbackKnob("F/B"), mixKnob("Mix")
{
    auto& params = audioProcessor.getParameters();

    addAndMakeVisible(titleLabel);
    titleLabel.setText("Delay", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);

    auto setupKnob = [&](Knob& k, juce::RangedAudioParameter* param) {
        addAndMakeVisible(k);
        if (param) {
            attachments.emplace_back(std::make_unique<SliderAttachment>(*param, k.getSlider()));
            k.getSlider().getProperties().set("paramId", param->paramID);
        }
    };

    setupKnob(timeKnob, params.getDelayTime());
    setupKnob(feedbackKnob, params.getDelayFeedback());
    setupKnob(mixKnob, params.getDelayMix());
}

void DelayPanel::resized()
{
    auto area = getLocalBounds();
    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::column;

    fb.items.add(juce::FlexItem(titleLabel).withFlex(0.5f));
    fb.items.add(juce::FlexItem(timeKnob).withFlex(1.5f));
    fb.items.add(juce::FlexItem(feedbackKnob).withFlex(1.5f));
    fb.items.add(juce::FlexItem(mixKnob).withFlex(1.5f));

    fb.performLayout(area);
}

} // namespace UI
} // namespace CZ101
