#include "EnvelopeEditor.h"

namespace CZ101 {
namespace UI {

EnvelopeEditor::EnvelopeEditor(CZ101AudioProcessor& processor, EnvelopeType type)
    : audioProcessor(processor), envType(type)
{
    // Initialize with dummy default data
    for (int i = 0; i < 8; ++i)
    {
        rates[i] = 0.5f;
        levels[i] = (i % 2 == 0) ? 1.0f : 0.0f;
    }
    
    updateData();
}

EnvelopeEditor::~EnvelopeEditor()
{
}

void EnvelopeEditor::updateData()
{
    auto& vm = audioProcessor.getVoiceManager();
    
    for (int i = 0; i < 8; ++i)
    {
        float r = 0.5f;
        float l = 0.0f;
        
        if (envType == EnvelopeType::DCA)
            vm.getDCAStage(i, r, l);
        else if (envType == EnvelopeType::DCW)
            vm.getDCWStage(i, r, l);
        else if (envType == EnvelopeType::PITCH)
            vm.getPitchStage(i, r, l);
            
        rates[i] = r;
        levels[i] = l;
    }
    
    if (envType == EnvelopeType::DCA)
    {
        sustainPoint = vm.getDCASustainPoint();
        endPoint = vm.getDCAEndPoint();
    }
    else if (envType == EnvelopeType::DCW)
    {
        sustainPoint = vm.getDCWSustainPoint();
        endPoint = vm.getDCWEndPoint();
    }
    else if (envType == EnvelopeType::PITCH)
    {
        sustainPoint = vm.getPitchSustainPoint();
        endPoint = vm.getPitchEndPoint();
    }
    
    repaint();
}

void EnvelopeEditor::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colours::black.withAlpha(0.8f));
    g.setColour(juce::Colours::darkgrey);
    g.drawRect(getLocalBounds(), 1);
    
    auto bounds = getLocalBounds().toFloat();
    float w = bounds.getWidth();
    float h = bounds.getHeight();
    
    // Draw grid/guides
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    for (int i = 0; i < 8; ++i)
    {
        float x = w * (static_cast<float>(i) / 8.0f);
        g.drawVerticalLine(static_cast<int>(x), 0.0f, h);
    }
    
    // Draw Path
    juce::Colour pathColour;
    if (envType == EnvelopeType::DCA) pathColour = juce::Colours::cyan;
    else if (envType == EnvelopeType::DCW) pathColour = juce::Colours::orange;
    else pathColour = juce::Colours::magenta; // PITCH

    g.setColour(pathColour);
    juce::Path p;
    
    // Start at 0,0 (bottom left-ish logic, but CZ starts at previous level. Assume 0 for start)
    // Actually CZ starts at 'Level' of current step? No, it moves TO Level.
    // Stage I: Start Level -> Target Level at Rate.
    // We need to simulate the path for visualization.
    
    float currentX = 0.0f;
    float currentY = h; // 0 level is bottom
    
    p.startNewSubPath(currentX, currentY);
    
    // We visualize cumulative time on X axis?
    // Or constant width per stage? 
    // Constant width per stage is easier to edit. Time-based is hard if rate is 0 (long time).
    // Let's use constant width for editing (CZ-101 LCD style).
    
    float stepWidth = w / 8.0f;
    
    for (int i = 0; i < 8; ++i)
    {
        float nextX = (i + 1) * stepWidth;
        // Level 0..1 -> h..0
        float val = levels[i];
        float nextY = h - (val * h);
        
        // Rate affects slope, but here X is fixed stages. 
        // We can visualize Rate as the steepness? No, X is time.
        // Let's draw the target point.
        
        p.lineTo(nextX, nextY);
        
        // Dotted line for sustain point
        if (i == sustainPoint)
        {
            g.setColour(juce::Colours::yellow);
            g.drawVerticalLine(static_cast<int>(nextX), 0.0f, h);
            g.drawText("SUS", static_cast<int>(nextX) - 20, 0, 40, 15, juce::Justification::centred);
        }
        if (i == endPoint)
        {
            g.setColour(juce::Colours::red);
            g.drawVerticalLine(static_cast<int>(nextX), 0.0f, h);
            g.drawText("END", static_cast<int>(nextX) - 20, 15, 40, 15, juce::Justification::centred);
        }
        
        // Draw Handle
        g.setColour(i == selectedStage ? juce::Colours::white : juce::Colours::lightgrey);
        g.fillEllipse(nextX - 4, nextY - 4, 8, 8);
        
        // Revert color for path
        // Revert color for path
        g.setColour(pathColour);
    }
    
    g.strokePath(p, juce::PathStrokeType(2.0f));
    
    // Add help text at bottom
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.setFont(10.0f);
    g.drawText("Drag: Level | Shift+Drag: Rate", 
               getLocalBounds().reduced(5).withHeight(15),
               juce::Justification::centredBottom, false);
}

void EnvelopeEditor::resized()
{
}

void EnvelopeEditor::mouseDown(const juce::MouseEvent& e)
{
    float w = static_cast<float>(getWidth());
    float stepWidth = w / 8.0f;
    
    // Find clicked stage
    int stage = static_cast<int>(e.position.x / stepWidth);
    if (stage >= 0 && stage < 8)
    {
        selectedStage = stage;
        repaint();
    }
}

void EnvelopeEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (selectedStage >= 0 && selectedStage < 8)
    {
        if (e.mods.isShiftDown())
        {
            // SHIFT+DRAG = Rate (horizontal)
            // Range 0..1 based on position within the step width
            float w = static_cast<float>(getWidth());
            float stepWidth = w / 8.0f;
            float stageStartX = selectedStage * stepWidth;
            
            // Normalize X within stage to 0..1
            float xWithinStage = e.position.x - stageStartX;
            float newRate = std::clamp(xWithinStage / stepWidth, 0.01f, 0.99f);
            
            rates[selectedStage] = newRate;
        }
        else
        {
            // NORMAL DRAG = Level (vertical)
            float y = std::clamp(e.position.y / static_cast<float>(getHeight()), 0.0f, 1.0f);
            levels[selectedStage] = 1.0f - y; // Invert because screen Y is top-down
        }
        
        sendUpdateToProcessor(selectedStage);
        repaint();
    }
}

void EnvelopeEditor::mouseUp(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
}

void EnvelopeEditor::sendUpdateToProcessor(int stageIndex)
{
    // Update Voice Manager directly
    // Note: In a real plugin we should use parameters to support automation
    // But we are in "Prototype / Phase 2" to verify engine.
    
    float lvl = levels[stageIndex];
    float rate = rates[stageIndex]; // Fixed for now
    
    auto& vm = audioProcessor.getVoiceManager(); // We need to expose this getter in Processor!
    
    if (envType == EnvelopeType::DCA)
    {
        vm.setDCAStage(stageIndex, rate, lvl);
    }
    else if (envType == EnvelopeType::DCW)
    {
        vm.setDCWStage(stageIndex, rate, lvl);
    }
    else if (envType == EnvelopeType::PITCH)
    {
        vm.setPitchStage(stageIndex, rate, lvl);
    }
}

} // namespace UI
} // namespace CZ101
