#include "WaveformDisplay.h"
#include "../SkinManager.h"
#include "../CZ101LookAndFeel.h"
#include <cmath>
#include <cmath>
#include "../../PluginProcessor.h" // Added include
#include "../../State/ParameterIDs.h"
// Rebuild trigger

namespace CZ101 {
namespace UI {

WaveformDisplay::WaveformDisplay()
{
    waveformData.resize(256);
    generateWaveform();
    startTimer(50);
}

void WaveformDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto& palette = SkinManager::getInstance().getCurrentPalette();
    auto* lf = dynamic_cast<CZ101LookAndFeel*>(&getLookAndFeel());
    
    // 1. Inset Background
    g.setColour(palette.surface.darker(0.3f));
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // 2. Subtle Grid
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    for (float x = 0; x < bounds.getWidth(); x += 15.0f)
        g.drawVerticalLine((int)x, 0.0f, bounds.getHeight());
    for (float y = 0; y < bounds.getHeight(); y += 15.0f)
        g.drawHorizontalLine((int)y, 0.0f, bounds.getWidth());

    // 3. Drawing the Waveform
    juce::Path path;
    auto width = bounds.getWidth();
    auto height = bounds.getHeight();
    auto centerY = bounds.getCentreY();
    
    path.startNewSubPath(bounds.getX(), centerY);
    
    for (size_t i = 0; i < waveformData.size(); ++i)
    {
        float x = bounds.getX() + (i / static_cast<float>(waveformData.size())) * width;
        float y = centerY - waveformData[i] * (height * 0.45f);
        path.lineTo(x, y);
    }
    
    // 4. Glow Stroke
    if (palette.glowColor != juce::Colours::transparentBlack && lf)
    {
        lf->applyGlow(g, path, palette.glowColor, 4.0f);
    }
    else
    {
        g.setColour(palette.accentCyan.withAlpha(0.2f));
        g.strokePath(path, juce::PathStrokeType(4.0f));
    }
    
    g.setColour(palette.accentCyan);
    g.strokePath(path, juce::PathStrokeType(1.5f));

    // 5. Scanlines
    if (palette.effect == DesignTokens::Colors::VisualEffect::Scanlines && lf)
    {
        lf->drawScanlines(g, bounds, 0.08f);
    }
}

void WaveformDisplay::timerCallback()
{
    repaint();
}

void WaveformDisplay::setWaveform(int waveformType)
{
    currentWaveform = waveformType;
    generateWaveform();
}

void WaveformDisplay::pushBuffer(const juce::AudioBuffer<float>& buffer)
{
    // Audit Fix 2.9: Ensure buffer access is safe
    auto* channelData = buffer.getReadPointer(0);
    int numSamples = buffer.getNumSamples();
    int dataSize = static_cast<int>(waveformData.size());
    
    if (dataSize == 0) return;

    // Grab only as much as fits to avoid issues if block size is huge
    int samplesToCopy = std::min(numSamples, dataSize);
    
    for (int i = 0; i < samplesToCopy; ++i)
    {
        waveformData[writePos] = channelData[i];
        writePos = (writePos + 1) % dataSize;
    }
}

void WaveformDisplay::generateWaveform() 
{
    // No-op or clear, as we use live data now
}

// --- Interaction ---
void WaveformDisplay::mouseDown(const juce::MouseEvent& e)
{
    if (processor)
    {
        // Capture start values
        auto* czProcessor = dynamic_cast<CZ101AudioProcessor*>(processor);
        if (czProcessor)
        {
            if (auto* p = czProcessor->getParameters().getAPVTS().getParameter(ParameterIDs::dcwSustain))
                startValX = p->getValue();
                
            if (auto* p = czProcessor->getParameters().getAPVTS().getParameter(ParameterIDs::osc1Level))
                startValY = p->getValue();
        }
    }
}

void WaveformDisplay::mouseDrag(const juce::MouseEvent& e)
{
    if (processor)
    {
        auto* czProcessor = dynamic_cast<CZ101AudioProcessor*>(processor);
        if (!czProcessor) return;

        // X -> DCW Sustain (Timbre)
        float deltaX = (e.getPosition().x - e.getMouseDownX()) / (float)getWidth();
        if (auto* p = czProcessor->getParameters().getAPVTS().getParameter(ParameterIDs::dcwSustain))
        {
            float newVal = juce::jlimit(0.0f, 1.0f, startValX + deltaX);
            p->setValueNotifyingHost(newVal);
        }
        
        // Y -> Osc 1 Level (Volume) - Inverted Y (Up is more level)
        float deltaY = (e.getMouseDownY() - e.getPosition().y) / (float)getHeight();
        if (auto* p = czProcessor->getParameters().getAPVTS().getParameter(ParameterIDs::osc1Level))
        {
            float newVal = juce::jlimit(0.0f, 1.0f, startValY + deltaY);
            p->setValueNotifyingHost(newVal);
        }
    }
}

} // namespace UI
} // namespace CZ101
