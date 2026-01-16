#include "ArpeggiatorPanel.h"
#include "../SkinManager.h"

namespace CZ101 {
namespace UI {

ArpeggiatorPanel::ArpeggiatorPanel(CZ101AudioProcessor& p)
    : audioProcessor(p),
      arpRateKnob("Arp Rate"),
      arpBpmKnob("Tempo"),
      arpGateKnob("Gate"),
      arpSwingKnob("Swing"),
      arpPatternKnob("Arp Pattern"),
      arpLatchButton("LATCH"),
      arpEnableButton("ARP ON")
{
    auto& params = audioProcessor.getParameters();

    addAndMakeVisible(arpEnableButton);
    arpEnableButton.setClickingTogglesState(true);
    if (params.getArpEnabled()) {
        arpEnableAttachment = std::make_unique<ButtonAttachment>(*params.getArpEnabled(), arpEnableButton);
    }

    addAndMakeVisible(arpLatchButton);
    arpLatchButton.setClickingTogglesState(true);
    if (params.getArpLatch()) {
        arpLatchAttachment = std::make_unique<ButtonAttachment>(*params.getArpLatch(), arpLatchButton);
    }

    addAndMakeVisible(arpRateKnob);
    if (params.getArpRate()) {
        arpRateAttachment = std::make_unique<SliderAttachment>(*params.getArpRate(), arpRateKnob.getSlider());
        arpRateKnob.getSlider().getProperties().set("paramId", params.getArpRate()->paramID);
    }

    addAndMakeVisible(arpBpmKnob);
    if (params.getArpBpm()) {
        arpBpmAttachment = std::make_unique<SliderAttachment>(*params.getArpBpm(), arpBpmKnob.getSlider());
        arpBpmKnob.getSlider().getProperties().set("paramId", params.getArpBpm()->paramID);
    }

    addAndMakeVisible(arpGateKnob);
    if (params.getArpGate()) {
        arpGateAttachment = std::make_unique<SliderAttachment>(*params.getArpGate(), arpGateKnob.getSlider());
        arpGateKnob.getSlider().getProperties().set("paramId", params.getArpGate()->paramID);
    }

    addAndMakeVisible(arpSwingKnob);
    if (params.getArpSwing()) {
        arpSwingAttachment = std::make_unique<SliderAttachment>(*params.getArpSwing(), arpSwingKnob.getSlider());
        arpSwingKnob.getSlider().getProperties().set("paramId", params.getArpSwing()->paramID);
    }

    addAndMakeVisible(arpPatternKnob);
    if (params.getArpPattern()) {
        arpPatternAttachment = std::make_unique<SliderAttachment>(*params.getArpPattern(), arpPatternKnob.getSlider());
        arpPatternKnob.getSlider().getProperties().set("paramId", params.getArpPattern()->paramID);
    }
}

void ArpeggiatorPanel::paint(juce::Graphics& g)
{
    // Optional: Draw sub-group styling
}

void ArpeggiatorPanel::resized()
{
    auto area = getLocalBounds();
    float scale = getUiScale();
    
    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::row;
    fb.justifyContent = juce::FlexBox::JustifyContent::center;
    fb.alignItems = juce::FlexBox::AlignItems::center;

    const float kw = 85.0f * scale;
    const float kh = 100.0f * scale;
    const float bW = 60.0f * scale;
    const float bH = 30.0f * scale;
    const float margin = 5.0f * scale;

    fb.items.add(juce::FlexItem(arpEnableButton).withWidth(bW).withHeight(bH).withMargin(margin));
    fb.items.add(juce::FlexItem(arpLatchButton).withWidth(bW).withHeight(bH).withMargin(margin));
    fb.items.add(juce::FlexItem(arpRateKnob).withFlex(1).withMinWidth(kw * 0.8f).withMinHeight(kh * 0.8f).withMargin(margin));
    fb.items.add(juce::FlexItem(arpBpmKnob).withFlex(1).withMinWidth(kw * 0.8f).withMinHeight(kh * 0.8f).withMargin(margin));
    fb.items.add(juce::FlexItem(arpGateKnob).withFlex(1).withMinWidth(kw * 0.8f).withMinHeight(kh * 0.8f).withMargin(margin));
    fb.items.add(juce::FlexItem(arpSwingKnob).withFlex(1).withMinWidth(kw * 0.8f).withMinHeight(kh * 0.8f).withMargin(margin));
    fb.items.add(juce::FlexItem(arpPatternKnob).withFlex(1).withMinWidth(kw * 0.8f).withMinHeight(kh * 0.8f).withMargin(margin));

    fb.performLayout(area);
}

} // namespace UI
} // namespace CZ101
