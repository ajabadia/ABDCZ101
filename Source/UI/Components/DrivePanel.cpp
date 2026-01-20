#include "DrivePanel.h"

namespace CZ101 {
namespace UI {

DrivePanel::DrivePanel(CZ101AudioProcessor& p)
    : audioProcessor(p),
      amountKnob("Drive"), colorKnob("Color"), mixKnob("Mix")
{
    auto& params = audioProcessor.getParameters();

    addAndMakeVisible(titleLabel);
    titleLabel.setText("Drive", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);

    auto setupKnob = [&](Knob& k, juce::RangedAudioParameter* param) {
        addAndMakeVisible(k);
        if (param) {
            attachments.emplace_back(std::make_unique<SliderAttachment>(*param, k.getSlider()));
            k.getSlider().getProperties().set("paramId", param->paramID);
        }
    };

    setupKnob(amountKnob, params.getDriveAmount());
    setupKnob(colorKnob, params.getDriveColor());
    setupKnob(mixKnob, params.getDriveMix());
}

void DrivePanel::resized()
{
    auto area = getLocalBounds();
    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::column;

    fb.items.add(juce::FlexItem(titleLabel).withFlex(0.5f));
    fb.items.add(juce::FlexItem(amountKnob).withFlex(1.5f));
    fb.items.add(juce::FlexItem(colorKnob).withFlex(1.5f));
    fb.items.add(juce::FlexItem(mixKnob).withFlex(1.5f));

    fb.performLayout(area);
}

} // namespace UI
} // namespace CZ101
