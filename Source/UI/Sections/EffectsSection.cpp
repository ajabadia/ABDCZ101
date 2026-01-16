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
    audioProcessor.getParameters().getAPVTS().addParameterListener("AUTHENTIC_MODE", this);
}

EffectsSection::~EffectsSection()
{
    audioProcessor.getParameters().getAPVTS().removeParameterListener("AUTHENTIC_MODE", this);
}

void EffectsSection::updateVisibility()
{
    auto& params = audioProcessor.getParameters();
    bool isAuth = params.getAuthenticMode() ? params.getAuthenticMode()->get() : true;
    
    filterPanel.setVisible(!isAuth);
    chorusPanel.setVisible(!isAuth);
    delayPanel.setVisible(!isAuth);
    reverbPanel.setVisible(!isAuth);
    
    resized();
    repaint();
}

void EffectsSection::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "AUTHENTIC_MODE")
    {
        juce::MessageManager::callAsync([this]() { updateVisibility(); });
    }
}

void EffectsSection::paint(juce::Graphics& g) 
{
    auto& palette = SkinManager::getInstance().getCurrentPalette();
    g.setFont(getScaledFont(16.0f));
    g.setColour(palette.textPrimary);
    
    auto bounds = getLocalBounds().toFloat().reduced(0.5f);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    
    auto& params = audioProcessor.getParameters();
    bool isAuth = params.getAuthenticMode() ? params.getAuthenticMode()->get() : true;
    juce::String title = isAuth ? "EFFECTS" : "EFFECTS & FILTERS";
    g.drawText(title, bounds.removeFromTop(20), juce::Justification::centred, false);
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
    bool isAuth = params.getAuthenticMode() ? params.getAuthenticMode()->get() : true;

    if (!isAuth)
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
