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

void LCDDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto& palette = SkinManager::getInstance().getCurrentPalette();
    auto* lf = dynamic_cast<CZ101LookAndFeel*>(&getLookAndFeel());
    
    // 1. Authentic LCD Background
    bool isModern = stateManager && stateManager->isShowingModernParam();
    g.setColour(isModern ? palette.lcdBg.overlaidWith(palette.modernIndicator) : palette.lcdBg);
    g.fillRoundedRectangle(bounds, DesignTokens::Metrics::cornerRadiusSmall);
    
    // 2. Specialized Effects
    if (palette.effect == DesignTokens::Colors::VisualEffect::Scanlines && lf)
    {
        lf->drawScanlines(g, bounds, 0.15f);
    }
    
    // 2.b Amber/Glow Effect
    if (palette.glowColor != juce::Colours::transparentBlack || isModern)
    {
        juce::Colour glowCol = isModern ? palette.modernIndicator.withAlpha(0.12f) : palette.glowColor.withAlpha(0.08f);
        g.setColour(glowCol);
        g.fillRoundedRectangle(bounds, DesignTokens::Metrics::cornerRadiusSmall);
    }
    
    // 3. Inner Bezel Shadow
    g.setColour(juce::Colours::black.withAlpha(0.2f));
    g.drawRoundedRectangle(bounds.reduced(1.0f), DesignTokens::Metrics::cornerRadiusSmall, 2.0f);

    // 4. Outer Border (Hardware Case)
    g.setColour(palette.border);
    g.drawRoundedRectangle(bounds, DesignTokens::Metrics::cornerRadiusSmall, 1.0f);

    // 5. Authentic Mode Indicator
    if (stateManager && stateManager->isAuthentic())
    {
        g.setColour(palette.lcdText.withAlpha(0.6f));
        float scale = getUiScale();
        g.setFont(juce::Font("Courier New", 12.0f * scale, juce::Font::bold));
        g.drawText("AUTH", getLocalBounds().reduced((int)(8 * scale), (int)(4 * scale)), juce::Justification::bottomRight);
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
