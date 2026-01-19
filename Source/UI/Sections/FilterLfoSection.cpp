#include "FilterLfoSection.h"
#include "../SkinManager.h"

namespace CZ101 {
namespace UI {

FilterLfoSection::FilterLfoSection(CZ101AudioProcessor& p)
    : audioProcessor(p),
      lfoRateKnob("Rate"),
      lfoDepthKnob("Depth"),
      lfoDelayKnob("Delay")
{
    auto& params = audioProcessor.getParameters();

    // LFO/Vibrato Controls
    addAndMakeVisible(lfoWaveSelector);
    lfoWaveSelector.addItemList( { "Triangle", "Saw Up", "Saw Down", "Square" }, 1 );
    addAndMakeVisible(lfoRateKnob);
    addAndMakeVisible(lfoDepthKnob);
    addAndMakeVisible(lfoDelayKnob);
    if (params.getLfoWaveform()) comboAttachments.emplace_back(std::make_unique<ComboBoxAttachment>(*params.getLfoWaveform(), lfoWaveSelector));
    if (params.getLfoRate()) {
        sliderAttachments.emplace_back(std::make_unique<SliderAttachment>(*params.getLfoRate(), lfoRateKnob.getSlider()));
        lfoRateKnob.getSlider().getProperties().set("paramId", params.getLfoRate()->paramID);
    }
    if (params.getLfoDepth()) {
        sliderAttachments.emplace_back(std::make_unique<SliderAttachment>(*params.getLfoDepth(), lfoDepthKnob.getSlider()));
        lfoDepthKnob.getSlider().getProperties().set("paramId", params.getLfoDepth()->paramID);
    }
    if (params.getLfoDelay()) {
        sliderAttachments.emplace_back(std::make_unique<SliderAttachment>(*params.getLfoDelay(), lfoDelayKnob.getSlider()));
        lfoDelayKnob.getSlider().getProperties().set("paramId", params.getLfoDelay()->paramID);
    }

    audioProcessor.getParameters().getAPVTS().addParameterListener("AUTHENTIC_MODE", this);
    updateVisibility();
}

FilterLfoSection::~FilterLfoSection() 
{
    audioProcessor.getParameters().getAPVTS().removeParameterListener("AUTHENTIC_MODE", this);
}

void FilterLfoSection::parameterChanged(const juce::String&, float) 
{
    juce::MessageManager::callAsync([this]() { updateVisibility(); });
}


void FilterLfoSection::updateVisibility()
{
    resized();
    repaint();
}

void FilterLfoSection::updateSliderValues()
{
    auto& params = audioProcessor.getParameters();
    
    if (auto* p = params.getLfoRate()) lfoRateKnob.getSlider().setValue(p->get(), juce::dontSendNotification);
    if (auto* p = params.getLfoDepth()) lfoDepthKnob.getSlider().setValue(p->get(), juce::dontSendNotification);
    if (auto* p = params.getLfoDelay()) lfoDelayKnob.getSlider().setValue(p->get(), juce::dontSendNotification);
    
    if (auto* p = params.getLfoWaveform()) lfoWaveSelector.setSelectedItemIndex(p->getIndex(), juce::dontSendNotification);
}

void FilterLfoSection::paint(juce::Graphics& g) {
    auto& palette = SkinManager::getInstance().getCurrentPalette();
    
    // Draw Section Background
    g.fillAll(palette.sectionBackground);

    g.setFont(getScaledFont(14.0f).boldened());
    g.setColour(palette.textPrimary); // Changed to textPrimary for better contrast
    
    auto bounds = getLocalBounds().toFloat();
    g.drawText("VIBRATO (LFO)", bounds.removeFromTop(25), juce::Justification::centred, false);
}

void FilterLfoSection::resized()
{
    auto area = getLocalBounds().reduced((int)(4 * getUiScale()));
    area.removeFromTop((int)(18 * getUiScale())); // Smaller title space
    
    juce::FlexBox lfoBox;
    lfoBox.flexDirection = juce::FlexBox::Direction::column;
    float scale = getUiScale();
    lfoBox.items.add(juce::FlexItem(lfoWaveSelector).withHeight(22 * scale).withMargin(2 * scale));
    
    juce::FlexBox lfoKnobs;
    lfoKnobs.items.add(juce::FlexItem(lfoRateKnob).withFlex(1));
    lfoKnobs.items.add(juce::FlexItem(lfoDepthKnob).withFlex(1));
    lfoKnobs.items.add(juce::FlexItem(lfoDelayKnob).withFlex(1));
    
    lfoBox.items.add(juce::FlexItem(lfoKnobs).withFlex(1.0f));
    lfoBox.performLayout(area);
}

} // namespace UI
} // namespace CZ101
