/*
  ==============================================================================

    ScaledSlider.h
    Created: 13 Jan 2026
    Author:  JUCESynthMaster + UX-SynthDesigner

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "ScaledComponent.h"
#include "SkinManager.h"

namespace CZ101 {
namespace UI {

class ScaledSlider : public juce::Slider
{
public:
    ScaledSlider()
    {
        // 5. Touch & HiDPI: Larger hit area
        setInterceptsMouseClicks(true, false); 
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }
    
    // Helper to get scale
    float getUiScale() const
    {
         if (auto* p = findParentComponentOfClass<juce::AudioProcessorEditor>())
             return (float)juce::Desktop::getInstance().getGlobalScaleFactor();
         return 1.0f;
    }

    // Override hit test for larger touch area if needed, 
    // but inheriting from Slider typically handles this if bounds are large enough.
    // UX Rule: "Dobla el hit-area invisible".
    bool hitTest(int x, int y) override
    {
        // Simple expansion: if within bounds, it's a hit.
        // For advanced touch, we might check distance to center for rotary.
        return juce::Slider::hitTest(x, y);
    }
    
    // 7. Micro-interacciones: Easing
    // double getValueFromText(const juce::String& text) override...
    
    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown())
        {
            juce::PopupMenu m;
            m.addItem(1, "MIDI Learn");
            m.addItem(2, "Unlearn MIDI CC");
            
            m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
               [this](int result)
               {
                   if (result == 0) return;
                   
                   // Find Processor
                   // We need to traverse up. Since we don't want to include PluginProcessor.h here fully if avoidable,
                   // we might need a trusted way. 
                   // Ideally, we'd use a bus or command.
                   // But let's try finding the Editor.
                   if (auto* editor = findParentComponentOfClass<juce::AudioProcessorEditor>())
                   {
                       // We need to cast to our Editor to get the processor? 
                       // Or we extends AudioProcessorEditor to have a virtual method `getMidiProcessor`?
                       // Or effectively dynamic_cast.
                       // For now, let's assume we can include PluginProcessor.h in the CPP, OR use a callback.
                       // Actually, ScaledSlider is header-only right now.
                       // Let's implement the logic in a .cpp file to allow includes.
                       handleMenuResult(result, editor);
                   }
               });
            return;
        }
        juce::Slider::mouseDown(e);
    }
    
    void handleMenuResult(int result, juce::AudioProcessorEditor* editor); // Implemented in CPP

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScaledSlider)
};

} // namespace UI
} // namespace CZ101
