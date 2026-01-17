#include "LCDDisplay.h"
#include "SkinManager.h"
#include "LCDStateManager.h"
#include "CZ101LookAndFeel.h"

namespace CZ101 {
namespace UI {

LCDDisplay::LCDDisplay()
{
    SkinManager::getInstance().addChangeListener(this);
    
    addAndMakeVisible(topLineLabel);
    topLineLabel.setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(bottomLineLabel);
    bottomLineLabel.setJustificationType(juce::Justification::centredLeft);

    updateSkin();
}

LCDDisplay::~LCDDisplay()
{
    if (stateManager)
        stateManager->removeChangeListener(this);
    SkinManager::getInstance().removeChangeListener(this);
}

void LCDDisplay::setStateManager(LCDStateManager* mgr)
{
    if (stateManager)
        stateManager->removeChangeListener(this);
        
    stateManager = mgr;
    
    if (stateManager)
    {
        stateManager->addChangeListener(this);
        changeListenerCallback(stateManager);
    }
}

void LCDDisplay::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &SkinManager::getInstance())
    {
        updateSkin();
        repaint();
    }
    else if (stateManager && source == stateManager)
    {
        topLineLabel.setText(stateManager->getTopLineText(), juce::dontSendNotification);
        bottomLineLabel.setText(stateManager->getBottomLineText(), juce::dontSendNotification);
    }
}

void LCDDisplay::updateSkin()
{
    auto& palette = SkinManager::getInstance().getCurrentPalette();
    topLineLabel.setColour(juce::Label::textColourId, palette.lcdText);
    bottomLineLabel.setColour(juce::Label::textColourId, palette.lcdText);
}

void LCDDisplay::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "OPERATION_MODE")
    {
        // 0: 101, 1: 5000, 2: Modern
        // We need to retrieve the actual index carefully, but since listener gives normalized float usually for Choice,
        // wait, AudioParameterChoice::getValue() returns normalized 0..1.
        // But parameterChanged (APVTS Listener) returns the value that the parameter has.
        // For Choice parameter, it's safer to just re-check the parameter safely on message thread
        
        juce::MessageManager::callAsync([this, newValue]() {
            // Find 101/5000/Modern
            setModernMode(newValue > 0.6f);
        });
    }
}

void LCDDisplay::setModernMode(bool isModern)
{
    if (modernMode != isModern)
    {
        modernMode = isModern;
        repaint();
    }
}

void LCDDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto& palette = SkinManager::getInstance().getCurrentPalette();
    auto* lf = dynamic_cast<CZ101LookAndFeel*>(&getLookAndFeel());
    
    // 1. LCD Background
    // Authentic uses standard LCD color. Modern uses a distinct color (e.g. Blue/Teal tint).
    juce::Colour bgCol = palette.lcdBg;
    
    if (modernMode)
    {
        // Modern Mode Color: Teal/Blue tint to signify "Enhanced"
        bgCol = bgCol.interpolatedWith(juce::Colours::cyan.darker(0.5f), 0.3f);
    }
    
    // Overlay 'show parameter' temporary logic
    bool tempParamShow = stateManager && stateManager->isShowingModernParam();
    if (tempParamShow) {
        bgCol = bgCol.overlaidWith(palette.modernIndicator.withAlpha(0.2f));
    }

    g.setColour(bgCol);
    g.fillRoundedRectangle(bounds, DesignTokens::Metrics::cornerRadiusSmall);
    
    // 2. Specialized Effects
    if (palette.effect == DesignTokens::Colors::VisualEffect::Scanlines && lf)
    {
        lf->drawScanlines(g, bounds, 0.15f);
    }
    
    // 2.b Glow Effect
    if (modernMode || tempParamShow)
    {
        juce::Colour glowCol = modernMode ? juce::Colours::cyan.withAlpha(0.15f) : palette.modernIndicator.withAlpha(0.12f);
        g.setColour(glowCol);
        g.fillRoundedRectangle(bounds, DesignTokens::Metrics::cornerRadiusSmall);
    }
    
    // 3. Inner Bezel Shadow
    g.setColour(juce::Colours::black.withAlpha(0.2f));
    g.drawRoundedRectangle(bounds.reduced(1.0f), DesignTokens::Metrics::cornerRadiusSmall, 2.0f);

    // 4. Outer Border (Hardware Case)
    g.setColour(palette.border);
    g.drawRoundedRectangle(bounds, DesignTokens::Metrics::cornerRadiusSmall, 1.0f);

    // 5. Authentic Mode Indicator (Inverted logic: Show "MODERN" if Modern)
    if (modernMode)
    {
        g.setColour(palette.lcdText.withAlpha(0.6f));
        float scale = getUiScale();
        g.setFont(juce::Font("Courier New", 12.0f * scale, juce::Font::bold));
        g.drawText("MODERN", getLocalBounds().reduced((int)(8 * scale), (int)(4 * scale)), juce::Justification::bottomRight);
    }
}

void LCDDisplay::resized()
{
    float scale = getUiScale();
    auto bounds = getLocalBounds().reduced((int)(10 * scale), (int)(5 * scale));
    int rowHeight = bounds.getHeight() / 2;
    
    // Refresh LCD fonts on resize
    topLineLabel.setFont(juce::Font("Courier New", 18.0f * scale, juce::Font::bold));
    bottomLineLabel.setFont(juce::Font("Courier New", 18.0f * scale, juce::Font::bold));

    topLineLabel.setBounds(bounds.removeFromTop(rowHeight));
    bottomLineLabel.setBounds(bounds);
}

} // namespace UI
} // namespace CZ101
