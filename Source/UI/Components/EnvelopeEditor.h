#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ScaledComponent.h"
#include "../../PluginProcessor.h"

namespace CZ101 {
namespace UI {

class EnvelopeEditor : public ScaledComponent
{
public:
    enum class EnvelopeType { DCW, DCA, PITCH };
    
    EnvelopeEditor(CZ101AudioProcessor& processor, EnvelopeType type);
    ~EnvelopeEditor() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Updates local data from processor/preset
    void updateData(); 
    void setLine(int line); 
    
    // Drag handlers
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    
    // Clipboard Struct
    struct ClipboardData {
        std::array<float, 8> rates;
        std::array<float, 8> levels;
        int sustain;
        int end;
        bool active = false;
    };
    static ClipboardData clipboard;

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
    
    int currentLine = 1;
    int selectedStage = -1;
    float startDragRate = 0.0f;
    float startDragLevel = 0.0f;
    
    // Helper to map Rate(0-1)/Level(0-1) to screen coordinates
    juce::Point<float> getScreenPoint(int stageIndex, float w, float h);
    
    void sendUpdateToProcessor(int stageIndex);
    
    // Custom Vertical Button Class
    class VerticalButton : public juce::Button
    {
    public:
        VerticalButton(const juce::String& name, const juce::String& text);
        void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override;
    private:
        juce::String buttonText;
    };

    VerticalButton copyButton;
    VerticalButton pasteButton;
    
    void performCopy();
    void performPaste();
};

} // namespace UI
} // namespace CZ101
