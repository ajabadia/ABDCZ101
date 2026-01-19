#include "EffectsSection.h"
#include "../SkinManager.h"

namespace CZ101 {
namespace UI {

EffectsSection::EffectsSection(CZ101AudioProcessor& p)
    : audioProcessor(p),
      filterPanel(p),
      chorusPanel(p),
      delayPanel(p),
      reverbPanel(p)
{
    addAndMakeVisible(filterPanel);
    addAndMakeVisible(chorusPanel);
    addAndMakeVisible(delayPanel);
    addAndMakeVisible(reverbPanel);

    updateVisibility();
    audioProcessor.getParameters().getAPVTS().addParameterListener("OPERATION_MODE", this);
}

EffectsSection::~EffectsSection()
{
    audioProcessor.getParameters().getAPVTS().removeParameterListener("OPERATION_MODE", this);
}

void EffectsSection::updateVisibility()
{
    auto& params = audioProcessor.getParameters();
    // Show effects only in Modern Mode (Index 2)
    int opMode = params.getOperationMode() ? params.getOperationMode()->getIndex() : 0;
    bool showEffects = (opMode == 2);
    
    filterPanel.setVisible(showEffects);
    chorusPanel.setVisible(showEffects);
    delayPanel.setVisible(showEffects);
    reverbPanel.setVisible(showEffects);
    
    resized();
    repaint();
}

void EffectsSection::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "OPERATION_MODE")
    {
        juce::MessageManager::callAsync([this]() { updateVisibility(); });
    }
}

void EffectsSection::updateSliderValues()
{
    // Propagate to sub-panels (which don't have explicit refresh, so we assume they are attached)
    // Actually, sub-panels might not have update methods.
    // However, if we forced params via APVTS value tree, Attachments SHOULD have updated.
    // The only reason we do this is because user complained about "not seen".
    // Since Sub-Panels (FilterPanel, etc) are just buckets of knobs with attachments, 
    // simply updating visibility (which forces layout) is usually enough.
    // BUT, if we want to be sure, we can manually check parameters if we had access.
    // Since we don't have access to sub-panel knobs directly from here, we will rely on
    // updateVisibility() to ensure structure is correct.
    // But wait, the user explicitely called effectsSection.updateSliderValues().
    
    updateVisibility();
}

void EffectsSection::paint(juce::Graphics& g) 
{
    auto& palette = SkinManager::getInstance().getCurrentPalette();
    g.setFont(getScaledFont(16.0f));
    g.setColour(palette.textPrimary);
    
    auto bounds = getLocalBounds().toFloat().reduced(0.5f);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    
    auto& params = audioProcessor.getParameters();
    int opMode = audioProcessor.getParameters().getOperationMode() ? audioProcessor.getParameters().getOperationMode()->getIndex() : 0;
    bool isModern = (opMode == 2);
    if (!isModern)
    {
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.fillAll();
        g.setColour(juce::Colours::white);
        g.drawText("Effects Disabled in Classic Mode", getLocalBounds(), juce::Justification::centred);
    }
    else
    {
    int opMode = audioProcessor.getParameters().getOperationMode() ? audioProcessor.getParameters().getOperationMode()->getIndex() : 0;
    bool isModern = (opMode == 2);

    juce::String title = isModern ? "EFFECTS & FILTERS" : "EFFECTS"; // Or "EFFECTS (OFF)"?
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

    auto& params = audioProcessor.getParameters();
    int opMode = params.getOperationMode() ? params.getOperationMode()->getIndex() : 0;
    bool showEffects = (opMode == 2);

    if (showEffects)
    {
        juce::FlexBox fb;
        fb.flexDirection = juce::FlexBox::Direction::row;
        fb.justifyContent = juce::FlexBox::JustifyContent::center;
        fb.alignItems = juce::FlexBox::AlignItems::stretch;

        const float margin = 5.0f * scale;

        fb.items.add(juce::FlexItem(filterPanel).withFlex(1).withMargin(margin));
        fb.items.add(juce::FlexItem(chorusPanel).withFlex(1).withMargin(margin));
        fb.items.add(juce::FlexItem(delayPanel).withFlex(1).withMargin(margin));
        fb.items.add(juce::FlexItem(reverbPanel).withFlex(1).withMargin(margin));

        fb.performLayout(contentArea);
    }
}

} // namespace UI
} // namespace CZ101
