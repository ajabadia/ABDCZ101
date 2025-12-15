#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../PluginProcessor.h"

namespace CZ101 {
namespace UI {

class EnvelopeEditor : public juce::Component
{
public:
    enum class EnvelopeType { DCW, DCA, PITCH };
    
    EnvelopeEditor(CZ101AudioProcessor& processor, EnvelopeType type);
    ~EnvelopeEditor() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Updates local data from processor/preset
    void updateData(); 
    
    // Drag handlers
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    CZ101AudioProcessor& audioProcessor;
    EnvelopeType envType;
    
    // Local copy of stage data for drawing/editing
    // 8 stages, rate (0-1), level (0-1)
    struct Point { float x; float y; };
    std::array<float, 8> rates;
    std::array<float, 8> levels;
    int sustainPoint = 2;
    int endPoint = 3;
    
    int selectedStage = -1;
    
    // Helper to map Rate(0-1)/Level(0-1) to screen coordinates
    juce::Point<float> getScreenPoint(int stageIndex, float w, float h);
    
    void sendUpdateToProcessor(int stageIndex);
};

} // namespace UI
} // namespace CZ101
