#include "ArpeggiatorPanel.h"
#include "../SkinManager.h"

namespace CZ101 {
namespace UI {

ArpeggiatorPanel::ArpeggiatorPanel(CZ101AudioProcessor& p)
    : audioProcessor(p),
      arpRateKnob("Arp Rate"),
      arpBpmKnob("Tempo"),
      arpGateKnob("Gate"),
      arpSwingKnob("Amount"),
      arpPatternKnob("Pattern"),
      arpOctaveKnob("Octaves"),
      arpLatchButton("LATCH"),
      arpEnableButton("ARP ON")
{
    auto& params = audioProcessor.getParameters();

    auto setupLabel = [this](juce::Label& l, const juce::String& text) {
        addAndMakeVisible(l);
        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setFont(getScaledFont(11.0f).boldened());
    };

    setupLabel(rateLabel, "RATE");
    setupLabel(tempoLabel, "TEMPO");
    setupLabel(patternLabel, "PATTERN");
    setupLabel(octLabel, "OCTAVES");
    setupLabel(gateLabel, "GATE");
    setupLabel(swingLabel, "SWING");
    setupLabel(typeLabel, "MODE");

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

    if (params.getArpSwing()) {
        arpSwingAttachment = std::make_unique<SliderAttachment>(*params.getArpSwing(), arpSwingKnob.getSlider());
        arpSwingKnob.getSlider().getProperties().set("paramId", params.getArpSwing()->paramID);
    }

    addAndMakeVisible(arpSwingModeCombo);
    arpSwingModeCombo.addItemList(params.getArpSwingMode()->choices, 1);
    if (params.getArpSwingMode()) {
        arpSwingModeAttachment = std::make_unique<ComboAttachment>(*params.getArpSwingMode(), arpSwingModeCombo);
    }

    addAndMakeVisible(arpPatternKnob);
    if (params.getArpPattern()) {
        arpPatternAttachment = std::make_unique<SliderAttachment>(*params.getArpPattern(), arpPatternKnob.getSlider());
        arpPatternKnob.getSlider().getProperties().set("paramId", params.getArpPattern()->paramID);
    }

    addAndMakeVisible(arpOctaveKnob);
    if (params.getArpOctave()) {
        arpOctaveAttachment = std::make_unique<SliderAttachment>(*params.getArpOctave(), arpOctaveKnob.getSlider());
        arpOctaveKnob.getSlider().getProperties().set("paramId", params.getArpOctave()->paramID);
    }
}

void ArpeggiatorPanel::paint(juce::Graphics& g)
{
    // Optional: Draw sub-group styling
}

void ArpeggiatorPanel::resized()
{
    auto area = getLocalBounds().reduced(10);
    float scale = getUiScale();
    
    const float kw = 80.0f * scale;
    const float kh = 90.0f * scale;
    const float bW = 60.0f * scale;
    const float bH = 26.0f * scale;
    const float margin = 4.0f * scale;

    auto createKnobGroup = [&](Knob& k, juce::Label& l) {
        juce::FlexBox group;
        group.flexDirection = juce::FlexBox::Direction::column;
        group.justifyContent = juce::FlexBox::JustifyContent::center;
        group.alignItems = juce::FlexBox::AlignItems::center;
        
        group.items.add(juce::FlexItem(l).withHeight(15 * scale).withWidth(kw));
        group.items.add(juce::FlexItem(k).withWidth(kw).withHeight(kh - 15 * scale));
        return group;
    };

    // Row 1: Timing & Sequence
    juce::FlexBox row1;
    row1.flexDirection = juce::FlexBox::Direction::row;
    row1.justifyContent = juce::FlexBox::JustifyContent::spaceAround;
    row1.alignItems = juce::FlexBox::AlignItems::center;

    // Sub-group for buttons
    juce::FlexBox btnFb;
    btnFb.flexDirection = juce::FlexBox::Direction::column;
    btnFb.justifyContent = juce::FlexBox::JustifyContent::center;
    btnFb.items.add(juce::FlexItem(arpEnableButton).withWidth(bW).withHeight(bH).withMargin(margin));
    btnFb.items.add(juce::FlexItem(arpLatchButton).withWidth(bW).withHeight(bH).withMargin(margin));
    
    row1.items.add(juce::FlexItem(btnFb).withWidth(bW + margin*2));
    row1.items.add(juce::FlexItem(createKnobGroup(arpRateKnob, rateLabel)).withFlex(1));
    row1.items.add(juce::FlexItem(createKnobGroup(arpBpmKnob, tempoLabel)).withFlex(1));
    row1.items.add(juce::FlexItem(createKnobGroup(arpPatternKnob, patternLabel)).withFlex(1));

    // Row 2: Range & Feel
    juce::FlexBox row2;
    row2.flexDirection = juce::FlexBox::Direction::row;
    row2.justifyContent = juce::FlexBox::JustifyContent::spaceAround;
    row2.alignItems = juce::FlexBox::AlignItems::center;

    row2.items.add(juce::FlexItem(createKnobGroup(arpOctaveKnob, octLabel)).withFlex(1));
    row2.items.add(juce::FlexItem(createKnobGroup(arpGateKnob, gateLabel)).withFlex(1));
    
    // Swing Combo Special Group
    juce::FlexBox swingFb;
    swingFb.flexDirection = juce::FlexBox::Direction::column;
    swingFb.justifyContent = juce::FlexBox::JustifyContent::center;
    swingFb.alignItems = juce::FlexBox::AlignItems::center;
    swingFb.items.add(juce::FlexItem(swingLabel).withHeight(14 * scale).withWidth(kw));
    swingFb.items.add(juce::FlexItem(arpSwingKnob).withWidth(kw).withHeight(kh - 36 * scale));
    swingFb.items.add(juce::FlexItem(typeLabel).withHeight(10 * scale).withWidth(kw));
    swingFb.items.add(juce::FlexItem(arpSwingModeCombo).withWidth(kw - 10*scale).withHeight(18 * scale));
    
    row2.items.add(juce::FlexItem(swingFb).withFlex(1.2f));

    // Main Layout
    juce::FlexBox mainFb;
    mainFb.flexDirection = juce::FlexBox::Direction::column;
    mainFb.items.add(juce::FlexItem(row1).withFlex(1));
    mainFb.items.add(juce::FlexItem(row2).withFlex(1));

    mainFb.performLayout(area);
}

} // namespace UI
} // namespace CZ101
