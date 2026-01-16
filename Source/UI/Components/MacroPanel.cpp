#include "MacroPanel.h"
#include "../SkinManager.h"

namespace CZ101 {
namespace UI {

MacroPanel::MacroPanel(CZ101AudioProcessor& p)
    : audioProcessor(p),
      brillianceKnob("Brilliance"),
      toneKnob("Tone"),
      spaceKnob("Space")
{
    auto& params = audioProcessor.getParameters();

    addAndMakeVisible(brillianceKnob);
    if (params.getMacroBrilliance()) {
        brillianceAttachment = std::make_unique<SliderAttachment>(*params.getMacroBrilliance(), brillianceKnob.getSlider());
        brillianceKnob.getSlider().getProperties().set("paramId", params.getMacroBrilliance()->paramID);
    }

    addAndMakeVisible(toneKnob);
    if (params.getMacroTone()) {
        toneAttachment = std::make_unique<SliderAttachment>(*params.getMacroTone(), toneKnob.getSlider());
        toneKnob.getSlider().getProperties().set("paramId", params.getMacroTone()->paramID);
    }

    addAndMakeVisible(spaceKnob);
    if (params.getMacroSpace()) {
        spaceAttachment = std::make_unique<SliderAttachment>(*params.getMacroSpace(), spaceKnob.getSlider());
        spaceKnob.getSlider().getProperties().set("paramId", params.getMacroSpace()->paramID);
    }
}

void MacroPanel::paint(juce::Graphics& g)
{
    // Optional: Draw a subtle frame or label
}

void MacroPanel::resized()
{
    auto area = getLocalBounds();
    float scale = getUiScale();
    
    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::row;
    fb.justifyContent = juce::FlexBox::JustifyContent::center;
    fb.alignItems = juce::FlexBox::AlignItems::stretch;

    const float kw = 85.0f * scale;
    const float kh = 100.0f * scale;
    const float margin = 5.0f * scale;

    fb.items.add(juce::FlexItem(brillianceKnob).withFlex(1).withMinWidth(kw).withMinHeight(kh).withMargin(margin));
    fb.items.add(juce::FlexItem(toneKnob).withFlex(1).withMinWidth(kw).withMinHeight(kh).withMargin(margin));
    fb.items.add(juce::FlexItem(spaceKnob).withFlex(1).withMinWidth(kw).withMinHeight(kh).withMargin(margin));

    fb.performLayout(area);
}

} // namespace UI
} // namespace CZ101
