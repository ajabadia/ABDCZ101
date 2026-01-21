#include "ChorusPanel.h"
#include "../../State/ParameterIDs.h"

namespace CZ101 {
namespace UI {

ChorusPanel::ChorusPanel(CZ101AudioProcessor& p)
    : audioProcessor(p),
      titleLabel({}, "CHORUS"),
      rateKnob("Rate"),
      depthKnob("Depth"),
      mixKnob("Mix")
{
    auto& params = audioProcessor.getParameters();

    addAndMakeVisible(titleLabel);
    titleLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(rateKnob);
    addAndMakeVisible(depthKnob);
    addAndMakeVisible(mixKnob);

    // Create attachments using named members
    if (auto* pRate = params.getChorusRate()) {
        rateAttachment = std::make_unique<SliderAttachment>(*pRate, rateKnob.getSlider());
    }
    if (auto* pDepth = params.getChorusDepth()) {
        depthAttachment = std::make_unique<SliderAttachment>(*pDepth, depthKnob.getSlider());
    }
    if (auto* pMix = params.getChorusMix()) {
        mixAttachment = std::make_unique<SliderAttachment>(*pMix, mixKnob.getSlider());
    }
}

void ChorusPanel::resized()
{
    auto area = getLocalBounds();
    titleLabel.setBounds(area.removeFromTop(20));

    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::row;
    fb.justifyContent = juce::FlexBox::JustifyContent::spaceAround;
    fb.alignItems = juce::FlexBox::AlignItems::center;

    fb.items.add(juce::FlexItem(rateKnob).withFlex(1));
    fb.items.add(juce::FlexItem(depthKnob).withFlex(1));
    fb.items.add(juce::FlexItem(mixKnob).withFlex(1));

    fb.performLayout(area);
}

} // namespace UI
} // namespace CZ101
