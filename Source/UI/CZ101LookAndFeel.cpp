#include "CZ101LookAndFeel.h"
#include "DesignTokens.h"
#include "SkinManager.h"

namespace CZ101 {
namespace UI {

CZ101LookAndFeel::CZ101LookAndFeel()
{
    // Constructor largely redundant if SkinManager::getInstance().getColour is overridden for everything,
    // but useful for fallback behavior if we call base class.
}




void CZ101LookAndFeel::drawRotarySlider(juce::Graphics& g,
                                       int x, int y, int width, int height,
                                       float sliderPos,
                                       float rotaryStartAngle,
                                       float rotaryEndAngle,
                                       juce::Slider& slider)
{
    auto& palette = SkinManager::getInstance().getCurrentPalette();
    auto outline = palette.accentCyan;
    auto fill    = palette.surface;

    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(6);
    auto diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
    bounds = bounds.withSizeKeepingCentre(diameter, diameter); // Enforce Square
    auto radius = diameter / 2.0f;
    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY();
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // 0. Interaction State
    const bool isMouseOver = slider.isMouseOverOrDragging();
    const float hoverAlpha = isMouseOver ? 1.0f : 0.0f;

    // 1. Shadow
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.fillEllipse(bounds.translated(0, 2));
    
    // 1.b Hover Glow (Outer Ring)
    if (isMouseOver) {
        g.setColour(outline.withAlpha(0.15f * hoverAlpha));
        g.drawEllipse(bounds.expanded(2.0f), 4.0f);
    }

    // 2. Base Plate (Gradient)
    juce::ColourGradient cg(fill.brighter(0.1f), centreX, centreY - radius,
                           fill.darker(0.2f), centreX, centreY + radius, false);
    g.setGradientFill(cg);
    g.fillEllipse(bounds);

    // 3. Ring Outline
    g.setColour(outline.withAlpha(0.3f + (0.2f * hoverAlpha))); // Brighter on hover
    g.drawEllipse(bounds, 1.5f);

    // 4. Indicator Path
    juce::Path p;
    auto pointerLength = radius * 0.8f;
    auto pointerThickness = 3.0f;
    p.addRoundedRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength, 1.0f);
    p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
    
    // Pointer Glow
    if (palette.glowColor != juce::Colours::transparentBlack)
    {
        applyGlow(g, p, palette.glowColor, 4.0f);
    }
    else
    {
        g.setColour(outline.withAlpha(0.2f * sliderPos + (0.3f * hoverAlpha))); // Fallback glow
        g.fillPath(p.createPathWithRoundedCorners(2.0f));
    }
    
    g.setColour(outline.brighter(isMouseOver ? 0.2f : 0.0f));
    g.fillPath(p);
    
    // Tick mark for center
    if (slider.getProperties().contains("showCenter")) {
        g.setColour(palette.textPrimary.withAlpha(0.5f));
        g.fillEllipse(centreX-1, bounds.getY()-4, 2, 2);
    }
}

void CZ101LookAndFeel::drawButtonBackground(juce::Graphics& g,
                                        juce::Button& button,
                                        const juce::Colour& backgroundColour,
                                        bool isMouseOver,
                                        bool isButtonDown)
{
    auto& palette = SkinManager::getInstance().getCurrentPalette();
    auto cornerSize = DesignTokens::Metrics::cornerRadiusSmall;
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    
    auto baseColour = backgroundColour;
    float alpha = 0.85f;
    
    if (isButtonDown) {
        baseColour = baseColour.darker(0.1f);
        alpha = 0.95f;
    }
    else if (isMouseOver) {
        baseColour = baseColour.brighter(0.1f);
        alpha = 0.95f;
    }

    // 0. Glass Effect
    if (palette.effect == DesignTokens::Colors::VisualEffect::Glass)
    {
        drawGlassEffect(g, bounds);
    }
    else
    {
        // 1. Fill
        g.setColour(baseColour.withAlpha(alpha));
        g.fillRoundedRectangle(bounds, cornerSize);
    }
    
    // 1.b Scanlines (if applicable)
    if (palette.effect == DesignTokens::Colors::VisualEffect::Scanlines)
    {
        drawScanlines(g, bounds, 0.05f);
    }

    // 2. Subtle Top Highlight
    g.setColour(juce::Colours::white.withAlpha(isMouseOver ? 0.2f : 0.1f));
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
    
    // 3. Accent Bottom Line (active/down)
    if (button.getToggleState() || isButtonDown) {
        auto accent = palette.accentCyan;
        g.setColour(accent.brighter(0.2f));
        auto bottomArea = bounds.removeFromBottom(2.0f).reduced(4, 0);
        g.fillRoundedRectangle(bottomArea, 1.0f);
        
        // Active Glow
        if (palette.glowColor != juce::Colours::transparentBlack)
        {
            g.setColour(palette.glowColor.withAlpha(0.2f));
            g.fillRoundedRectangle(bounds, cornerSize);
        }
        else
        {
            g.setColour(accent.withAlpha(0.2f));
            g.fillRoundedRectangle(bounds, cornerSize);
        }
    }
}

void CZ101LookAndFeel::drawGroupComponentOutline(juce::Graphics& g, int width, int height,
                                               const juce::String& text, const juce::Justification& position,
                                               juce::GroupComponent& group)
{
    auto& palette = SkinManager::getInstance().getCurrentPalette();
    auto bounds = group.getLocalBounds().toFloat();
    auto textWidth = g.getCurrentFont().getStringWidth(text);
    
    g.setColour(palette.border);
    juce::Path p;
    
    float textX = 15.0f;
    float headH = 20.0f;
    
    p.startNewSubPath(textX + textWidth + 5.0f, 10);
    p.lineTo(width - 10.0f, 10);
    p.lineTo(width - 10.0f, height - 10.0f);
    p.lineTo(10.0f, height - 10.0f);
    p.lineTo(10.0f, 10);
    p.lineTo(textX - 5.0f, 10);
    
    g.strokePath(p, juce::PathStrokeType(1.0f));
    
    // Scanlines on group header area?
    if (palette.effect == DesignTokens::Colors::VisualEffect::Scanlines)
    {
        drawScanlines(g, juce::Rectangle<float>(10, 10, width - 20, 2), 0.1f);
    }

    g.setColour(palette.textPrimary);
    g.setFont(DesignTokens::Typography::getSubHeaderSize());
    g.drawText(text, 15, 0, textWidth, headH, juce::Justification::centredLeft);
}

// --- Effects Implementation ---
void CZ101LookAndFeel::drawScanlines(juce::Graphics& g, const juce::Rectangle<float>& area, float opacity)
{
    g.setColour(juce::Colours::black.withAlpha(opacity));
    for (float y = area.getY(); y < area.getBottom(); y += 3.0f)
        g.drawHorizontalLine((int)y, area.getX(), area.getRight());
}

void CZ101LookAndFeel::applyGlow(juce::Graphics& g, const juce::Path& path, const juce::Colour& colour, float thickness)
{
    g.saveState();
    for (int i = 1; i <= 3; ++i)
    {
        g.setColour(colour.withAlpha(0.15f / (float)i));
        g.strokePath(path, juce::PathStrokeType(thickness * (float)i));
    }
    g.restoreState();
}

void CZ101LookAndFeel::drawGlassEffect(juce::Graphics& g, const juce::Rectangle<float>& area)
{
    juce::ColourGradient cg(juce::Colours::white.withAlpha(0.15f), 0, area.getY(),
                           juce::Colours::white.withAlpha(0.05f), 0, area.getBottom(), false);
    g.setGradientFill(cg);
    g.fillRoundedRectangle(area, 4.0f);
    
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.drawRoundedRectangle(area, 4.0f, 1.0f);
}

} // namespace UI
} // namespace CZ101
