#include "GeneralSection.h"
#include "../SkinManager.h"
#include "../../State/ParameterIDs.h"

namespace CZ101 {
namespace UI {

GeneralSection::GeneralSection(CZ101AudioProcessor& p)
    : audioProcessor(p),
      glideKnob("Glide"),
      masterVolumeKnob("Master"),
      macroPanel(p)
{
    auto& params = audioProcessor.getParameters();
    auto& apvts = params.getAPVTS();

    addAndMakeVisible(glideKnob);
    if (params.getGlideTime()) {
        glideAttachment = std::make_unique<SliderAttachment>(*params.getGlideTime(), glideKnob.getSlider());
        glideKnob.getSlider().getProperties().set("paramId", params.getGlideTime()->paramID);
    }

    addAndMakeVisible(masterVolumeKnob);
    if (params.getMasterVolume()) {
        masterVolumeAttachment = std::make_unique<SliderAttachment>(*params.getMasterVolume(), masterVolumeKnob.getSlider());
        masterVolumeKnob.getSlider().getProperties().set("paramId", params.getMasterVolume()->paramID);
    }
    

    
    // Modern Panel visibility controlled by mode 3 (index 2)
    addAndMakeVisible(macroPanel);

    // Initial visibility
    int currentMode = params.getOperationMode() ? params.getOperationMode()->getIndex() : 0;
    macroPanel.setVisible(currentMode == 2);

    apvts.addParameterListener(ParameterIDs::operationMode, this);
}

GeneralSection::~GeneralSection() 
{
    audioProcessor.getParameters().getAPVTS().removeParameterListener(ParameterIDs::operationMode, this);
}

void GeneralSection::paint(juce::Graphics& g) {
    auto& palette = SkinManager::getInstance().getCurrentPalette();
    
    g.fillAll(palette.sectionBackground);

    g.setFont(getScaledFont(14.0f).boldened());
    g.setColour(palette.textPrimary);
    
    auto bounds = getLocalBounds().toFloat();
    g.drawText("GENERAL CONTROLS", bounds.removeFromTop(25), juce::Justification::centred, false);
}

void GeneralSection::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == ParameterIDs::operationMode)
    {
        // 0: 101, 1: 5000, 2: Modern
        // newValue from APVTS is normalized 0.0-1.0 for choices? 
        // No, parameterChanged often gives the denormalized value for index if it's AudioParameterChoice? 
        // Wait, APVTS listener usually gives normalized float.
        // Let's rely on reading the parameter directly safely on message thread to avoid float rounding issues.
        
        juce::MessageManager::callAsync([this]() {
            if (auto* p = audioProcessor.getParameters().getOperationMode())
            {
                int currentMode = p->getIndex();
                bool isModern = (currentMode == 2);
                
                if (macroPanel.isVisible() != isModern)
                {
                    macroPanel.setVisible(isModern);
                    resized();
                }
            }
        });
    }
}

void GeneralSection::resized()
{
    auto area = getLocalBounds();
    float scale = getUiScale();
    
    // Title space - use 40px scaled 
    auto titleArea = area.removeFromTop((int)(40 * scale));
    
    // Remaining area for knobs
    auto knobArea = area.reduced((int)(15 * scale));

    bool isModern = macroPanel.isVisible();

    juce::FlexBox mainFb;
    mainFb.flexDirection = juce::FlexBox::Direction::column;
    mainFb.justifyContent = juce::FlexBox::JustifyContent::center;

    // Row 0: Model Selector (Removed)
    // juce::FlexBox row0; 
    // row0.items.add...


    // Row 1: Main Controls
    juce::FlexBox row1;
    row1.flexDirection = juce::FlexBox::Direction::row;
    row1.justifyContent = juce::FlexBox::JustifyContent::center;
    row1.alignItems = juce::FlexBox::AlignItems::stretch;

    const float kw = 85.0f * scale;
    const float kh = 100.0f * scale;
    const float margin = 5.0f * scale;

    row1.items.add(juce::FlexItem(glideKnob).withFlex(1).withMinWidth(kw).withMinHeight(kh).withMargin(margin));
    
    if (isModern)
    {
        row1.items.add(juce::FlexItem(macroPanel).withFlex(3.0f).withMinWidth(kw * 3).withMinHeight(kh).withMargin(margin));
    }

    row1.items.add(juce::FlexItem(masterVolumeKnob).withFlex(1).withMinWidth(kw).withMinHeight(kh).withMargin(margin));

    mainFb.items.add(juce::FlexItem(row1).withFlex(isModern ? 1.0f : 2.0f).withMinHeight(isModern ? 120 * scale : 200 * scale));

    mainFb.performLayout(knobArea);
}

} // namespace UI
} // namespace CZ101
