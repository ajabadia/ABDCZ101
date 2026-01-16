#include <JuceHeader.h>
#include "EnvelopeEditor.h"
#include <cmath>
#include "../SkinManager.h"
#include "../DesignTokens.h"
#include "../CZ101LookAndFeel.h"

namespace CZ101 {
namespace UI {

EnvelopeEditor::ClipboardData EnvelopeEditor::clipboard;

// --- VerticalButton Implementation ---
EnvelopeEditor::VerticalButton::VerticalButton(const juce::String& name, const juce::String& text)
    : juce::Button(name), buttonText(text)
{}

void EnvelopeEditor::VerticalButton::paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown)
{
    auto& palette = SkinManager::getInstance().getCurrentPalette();
    
    // Background - Use surfaceLight for hover
    juce::Colour bg = isButtonDown ? palette.accentCyan : (isMouseOver ? palette.surfaceLight : palette.surface);
    if (!isEnabled()) bg = palette.surface.darker(0.1f);
    
    g.setColour(bg);
    g.fillAll();
    
    g.setColour(palette.border);
    g.drawRect(getLocalBounds());

    // Text (Rotated -90 degrees)
    g.setColour(isButtonDown ? juce::Colours::white : (isEnabled() ? palette.textPrimary : palette.textSecondary));
    
    // Find parent EnvelopeEditor to get scale
    float scale = 1.0f;
    if (auto* ee = findParentComponentOfClass<EnvelopeEditor>()) scale = ee->getUiScale();
    g.setFont(12.0f * scale);
    
    auto bounds = getLocalBounds().toFloat();
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    
    g.saveState();
    g.addTransform(juce::AffineTransform::rotation(-juce::MathConstants<float>::halfPi, cx, cy));
    g.drawText(buttonText, bounds.withWidth(bounds.getHeight()).withHeight(bounds.getWidth()).withCentre({cx, cy}), juce::Justification::centred, false);
    g.restoreState();
}

// --- Undo Action ---
class EnvelopeChangeAction : public juce::UndoableAction
{
public:
    EnvelopeChangeAction(CZ101AudioProcessor& proc, int type, int line, int stage, 
                         float oldR, float oldL, float newR, float newL)
        : processor(proc), envType(type), lineIndex(line), stageIndex(stage),
          oldRate(oldR), oldLevel(oldL), newRate(newR), newLevel(newL)
    {}

    bool perform() override { sendUpdate(newRate, newLevel); return true; }
    bool undo() override    { sendUpdate(oldRate, oldLevel); return true; }
    int getSizeInUnits() override { return sizeof(*this); }

private:
    void sendUpdate(float r, float l)
    {
        EnvelopeUpdateCommand cmd;
        if (envType == 1) cmd.type = EnvelopeUpdateCommand::DCA_STAGE;
        else if (envType == 0) cmd.type = EnvelopeUpdateCommand::DCW_STAGE;
        else cmd.type = EnvelopeUpdateCommand::PITCH_STAGE;

        cmd.line = lineIndex;
        cmd.index = stageIndex;
        cmd.rate = r;
        cmd.level = l;
        processor.scheduleEnvelopeUpdate(cmd);
    }

    CZ101AudioProcessor& processor;
    int envType, lineIndex, stageIndex;
    float oldRate, oldLevel, newRate, newLevel;
};

// --- Main Editor ---
EnvelopeEditor::EnvelopeEditor(CZ101AudioProcessor& processor, EnvelopeType type)
    : audioProcessor(processor), envType(type),
      copyButton("Copy", "COPY"), pasteButton("Paste", "PASTE")
{
    addAndMakeVisible(copyButton);
    addAndMakeVisible(pasteButton);
    
    copyButton.setTooltip("Copy Envelope Data");
    pasteButton.setTooltip("Paste Envelope Data");
    
    copyButton.onClick = [this]() { performCopy(); };
    pasteButton.onClick = [this]() { performPaste(); };
    
    pasteButton.setEnabled(clipboard.active);
    updateData();
}

EnvelopeEditor::~EnvelopeEditor() {}

void EnvelopeEditor::updateData()
{
    auto& vm = audioProcessor.getVoiceManager();
    for (int i = 0; i < 8; ++i) {
        float r = 0.5f, l = 0.0f;
        if (envType == EnvelopeType::DCA)      vm.getDCAStage(currentLine, i, r, l);
        else if (envType == EnvelopeType::DCW) vm.getDCWStage(currentLine, i, r, l);
        else if (envType == EnvelopeType::PITCH) vm.getPitchStage(currentLine, i, r, l);
        rates[i] = r; levels[i] = l;
    }
    
    if (envType == EnvelopeType::DCA) {
        sustainPoint = vm.getDCASustainPoint(currentLine);
        endPoint = vm.getDCAEndPoint(currentLine);
    } else if (envType == EnvelopeType::DCW) {
        sustainPoint = vm.getDCWSustainPoint(currentLine);
        endPoint = vm.getDCWEndPoint(currentLine);
    } else if (envType == EnvelopeType::PITCH) {
        sustainPoint = vm.getPitchSustainPoint(currentLine);
        endPoint = vm.getPitchEndPoint(currentLine);
    }
    repaint();
}

void EnvelopeEditor::setLine(int line) { currentLine = line; updateData(); }

void EnvelopeEditor::paint(juce::Graphics& g)
{
    auto& palette = SkinManager::getInstance().getCurrentPalette();
    auto* lf = dynamic_cast<CZ101LookAndFeel*>(&getLookAndFeel());
    
    g.fillAll(palette.surface);
    
    auto bounds = getLocalBounds().toFloat();
    float w = bounds.getWidth() - 26; // Account for vertical buttons
    float h = bounds.getHeight();
    float stepWidth = w / 8.0f;

    // Grid
    g.setColour(palette.textPrimary.withAlpha(0.15f));
    for (int i = 1; i < 8; ++i) g.drawVerticalLine((int)(i * stepWidth), 0, h);
    g.setColour(palette.textPrimary.withAlpha(0.05f));
    for (int i = 1; i < 4; ++i) g.drawHorizontalLine((int)(i * h / 4.0f), 0, w);

    // Path
    juce::Colour accent = (envType == EnvelopeType::DCA) ? palette.accentCyan : 
                         (envType == EnvelopeType::DCW) ? palette.accentOrange : 
                          palette.accentTertiary;

    juce::Path p;
    p.startNewSubPath(0.0f, h);
    for (int i = 0; i < 8; ++i) p.lineTo((i + 1) * stepWidth, h - (levels[i] * h));

    // Glow Effect
    if (palette.glowColor != juce::Colours::transparentBlack && lf)
    {
        lf->applyGlow(g, p, palette.glowColor, 3.0f);
    }
    else
    {
        g.setColour(accent.withAlpha(0.2f));
        g.strokePath(p, juce::PathStrokeType(4.0f));
    }
    
    g.setColour(accent);
    float strokeThickness = 1.5f * getUiScale();
    g.strokePath(p, juce::PathStrokeType(strokeThickness));

    // Scanlines Overlay
    if (palette.effect == DesignTokens::Colors::VisualEffect::Scanlines && lf)
    {
        lf->drawScanlines(g, bounds, 0.05f);
    }

    // Handles
    for (int i = 0; i < 8; ++i) {
        float x = (i + 1) * stepWidth;
        float y = h - (levels[i] * h);
        
        if (i == sustainPoint) {
            g.setColour(juce::Colours::yellow.withAlpha(0.3f));
            g.drawVerticalLine((int)x, 0, h);
        }
        if (i == endPoint) {
            g.setColour(juce::Colours::red.withAlpha(0.3f));
            g.drawVerticalLine((int)x, 0, h);
        }

        g.setColour( (i == selectedStage) ? juce::Colours::white : accent.brighter(0.2f));
        float handleSize = 7.0f * getUiScale();
        g.fillEllipse(x - handleSize * 0.5f, y - handleSize * 0.5f, handleSize, handleSize);
        
        if (palette.glowColor != juce::Colours::transparentBlack)
        {
            g.setColour(palette.glowColor.withAlpha(0.4f));
            float outerSize = 9.0f * getUiScale();
            g.drawEllipse(x - outerSize * 0.5f, y - outerSize * 0.5f, outerSize, outerSize, 1.0f);
        }
    }
}

void EnvelopeEditor::resized()
{
    auto area = getLocalBounds();
    float scale = getUiScale();
    auto buttonsArea = area.removeFromRight((int)(26 * scale));
    int bh = buttonsArea.getHeight() / 2;
    copyButton.setBounds(buttonsArea.removeFromTop(bh).reduced(1));
    pasteButton.setBounds(buttonsArea.reduced(1));
    
    // Refresh for VerticalButton scaling if needed (they use EE's scale)
    copyButton.repaint();
    pasteButton.repaint();
}

void EnvelopeEditor::mouseDown(const juce::MouseEvent& e)
{
    float w = getWidth() - 26.0f;
    float stepWidth = w / 8.0f;
    int stage = (int)((e.position.x + stepWidth * 0.5f) / stepWidth) - 1;
    if (stage >= 0 && stage < 8) {
        selectedStage = stage;
        startDragRate = rates[stage];
        startDragLevel = levels[stage];
        audioProcessor.getUndoManager().beginNewTransaction();
        repaint();
    }
}

void EnvelopeEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (selectedStage >= 0 && selectedStage < 8) {
        if (e.mods.isShiftDown()) {
            float w = getWidth() - 26.0f;
            float stepWidth = w / 8.0f;
            rates[selectedStage] = std::clamp((e.position.x - selectedStage * stepWidth) / stepWidth, 0.01f, 0.99f);
        } else {
            levels[selectedStage] = 1.0f - std::clamp(e.position.y / getHeight(), 0.0f, 1.0f);
        }
        sendUpdateToProcessor(selectedStage);
        repaint();
    }
}

void EnvelopeEditor::mouseUp(const juce::MouseEvent& e)
{
    if (selectedStage >= 0 && selectedStage < 8) {
        if (rates[selectedStage] != startDragRate || levels[selectedStage] != startDragLevel) {
            audioProcessor.getUndoManager().perform(new EnvelopeChangeAction(
                audioProcessor, (int)envType, currentLine, selectedStage, 
                startDragRate, startDragLevel, rates[selectedStage], levels[selectedStage]));
        }
    }
    selectedStage = -1;
    repaint();
}

void EnvelopeEditor::sendUpdateToProcessor(int stageIndex)
{
    EnvelopeUpdateCommand cmd;
    cmd.type = (envType == EnvelopeType::DCA) ? EnvelopeUpdateCommand::DCA_STAGE :
               (envType == EnvelopeType::DCW) ? EnvelopeUpdateCommand::DCW_STAGE :
               EnvelopeUpdateCommand::PITCH_STAGE;
    cmd.line = currentLine;
    cmd.index = stageIndex;
    cmd.rate = rates[stageIndex];
    cmd.level = levels[stageIndex];
    audioProcessor.scheduleEnvelopeUpdate(cmd);
}

void EnvelopeEditor::performCopy()
{
    clipboard.rates = rates;
    clipboard.levels = levels;
    clipboard.sustain = sustainPoint;
    clipboard.end = endPoint;
    clipboard.active = true;
    pasteButton.setEnabled(true);
}

void EnvelopeEditor::performPaste()
{
    if (!clipboard.active) return;
    rates = clipboard.rates;
    levels = clipboard.levels;
    sustainPoint = clipboard.sustain;
    endPoint = clipboard.end;
    
    for (int i = 0; i < 8; ++i) sendUpdateToProcessor(i);
    
    auto& vm = audioProcessor.getVoiceManager();
    if (envType == EnvelopeType::DCA) {
        vm.setDCASustainPoint(currentLine, sustainPoint);
        vm.setDCAEndPoint(currentLine, endPoint);
    } else if (envType == EnvelopeType::DCW) {
        vm.setDCWSustainPoint(currentLine, sustainPoint);
        vm.setDCWEndPoint(currentLine, endPoint);
    } else {
        vm.setPitchSustainPoint(currentLine, sustainPoint);
        vm.setPitchEndPoint(currentLine, endPoint);
    }
    repaint();
}

} // namespace UI
} // namespace CZ101
