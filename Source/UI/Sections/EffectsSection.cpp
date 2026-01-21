#include "EffectsSection.h"
#include "../SkinManager.h"
#include "../../State/ParameterIDs.h"

namespace CZ101 {
namespace UI {

EffectsSection::EffectsSection(CZ101AudioProcessor& p)
    : audioProcessor(p),
      filterPanel(p),
      drivePanel(p), // [NEW] Phase 12
      chorusPanel(p),
      delayPanel(p),
      reverbPanel(p)
{
    addAndMakeVisible(filterPanel);
    addAndMakeVisible(drivePanel); // [NEW] Phase 12
    addAndMakeVisible(chorusPanel);
    addAndMakeVisible(delayPanel);
    addAndMakeVisible(reverbPanel);

    updateVisibility();
    audioProcessor.getParameters().getAPVTS().addParameterListener(ParameterIDs::operationMode, this);
}

EffectsSection::~EffectsSection()
{
    audioProcessor.getParameters().getAPVTS().removeParameterListener(ParameterIDs::operationMode, this);
}

void EffectsSection::updateVisibility()
{
    auto& params = audioProcessor.getParameters();
    int opMode = params.getOperationMode() ? params.getOperationMode()->getIndex() : 0;
    
    bool isModern = (opMode == 2);
    bool isClassic5000 = (opMode == 1);
    
    // Chorus is available in Modern and Classic 5000
    chorusPanel.setVisible(isModern || isClassic5000);
    
    // Other effects are Modern only
    filterPanel.setVisible(isModern);
    drivePanel.setVisible(isModern);
    delayPanel.setVisible(isModern);
    reverbPanel.setVisible(isModern);
    
    resized();
    repaint();
}

void EffectsSection::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == ParameterIDs::operationMode)
    {
        juce::MessageManager::callAsync([this]() { 
            updateVisibility(); 
            resized(); 
            repaint(); 
        });
    }
}

void EffectsSection::updateSliderValues()
{
    updateVisibility();
}

void EffectsSection::paint(juce::Graphics& g) 
{
    auto& palette = SkinManager::getInstance().getCurrentPalette();
    g.setFont(getScaledFont(16.0f));
    g.setColour(palette.textPrimary);
    
    auto bounds = getLocalBounds().toFloat().reduced(0.5f);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    
    int opMode = audioProcessor.getParameters().getOperationMode() ? audioProcessor.getParameters().getOperationMode()->getIndex() : 0;
    bool isClassic101 = (opMode == 0);
    
    if (isClassic101)
    {
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.fillAll();
        g.setColour(juce::Colours::white);
        g.drawText("Effects Disabled in Classic 101 Mode", getLocalBounds(), juce::Justification::centred);
    }
    else
    {
        bool isModern = (opMode == 2);
        juce::String title = isModern ? "EFFECTS & FILTERS" : "CHORUS (Classic 5000)"; 
        g.drawText(title, bounds.removeFromTop(20), juce::Justification::centred, false);
    }
}

void EffectsSection::resized()
{
    auto area = getLocalBounds();
    float scale = getUiScale();
    
    // Title space
    auto titleArea = area.removeFromTop((int)(40 * scale));
    
    // Reduced area for content
    auto contentArea = area.reduced((int)(10 * scale));

    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::row;
    fb.justifyContent = juce::FlexBox::JustifyContent::center;
    fb.alignItems = juce::FlexBox::AlignItems::stretch;

    const float margin = 5.0f * scale;

    // Ordered Layout: Chorus First, then Modern Effects
    if (chorusPanel.isVisible())
        fb.items.add(juce::FlexItem(chorusPanel).withFlex(1).withMargin(margin));

    if (filterPanel.isVisible())
        fb.items.add(juce::FlexItem(filterPanel).withFlex(1).withMargin(margin));
        
    if (drivePanel.isVisible())
        fb.items.add(juce::FlexItem(drivePanel).withFlex(1).withMargin(margin)); 

    if (delayPanel.isVisible())
        fb.items.add(juce::FlexItem(delayPanel).withFlex(1).withMargin(margin));
        
    if (reverbPanel.isVisible())
        fb.items.add(juce::FlexItem(reverbPanel).withFlex(1).withMargin(margin));

    fb.performLayout(contentArea);
}

} // namespace UI
} // namespace CZ101
