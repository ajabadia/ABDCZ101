/*
  ==============================================================================

    ScaledSlider.cpp
    Created: 13 Jan 2026
    Author:  JUCESynthMaster + UX-SynthDesigner

  ==============================================================================
*/

#include "ScaledSlider.h"
#include "../../PluginEditor.h" 
#include "../../PluginProcessor.h"

namespace CZ101 {
namespace UI {

void ScaledSlider::handleMenuResult(int result, juce::AudioProcessorEditor* editor)
{
    // Safe cast because we know our editor type
    if (auto* czEditor = dynamic_cast<CZ101AudioProcessorEditor*>(editor))
    {
        // Get Parameter ID
        // Slider attachment usually stores it, or we use getComponentID() / getName() if matched.
        // Best bet: check if this slider has an attachment in the processor? No, attachment is in editor.
        // JUCE Sliders don't know their param ID natively unless we parse it or store it.
        // However, we used APVTS attachments.
        
        // Strategy: Iterate APVTS to find which parameter this slider is attached to?
        // Or simpler: We rely on the fact that we passed the ID as the Component ID or Name?
        // In `Knob` constructor: `Knob::Knob(const juce::String& name) : juce::Slider(name) ...`
        // But usually name is "Rate", "Depth" (Human readable). ID is "LFO_RATE".
        
        // Let's assume for now the user sets ComponentID to ParamID for MIDI Learn to work, 
        // OR we traverse the Editor's attachments to find us.
        // Actually, existing OscSection logic:
        // `lfoRateKnob.setName("Rate")` -> This is for display.
        // `sliderAttachments.emplace_back(std::make_unique<SliderAttachment>(*params.lfoRate, lfoRateKnob));`
        
        // We can't easily get the ID from the slider. 
        // We might need to modify `Knob` to store the ParamID.
        // BUT, we can access the `SliderParameterAttachment`.
        
        // Getting attachment from Slider is hard (private implementation).
        
        // WORKAROUND: In `Knob` constructor or where we attach, store the ID as property.
        // For now, let's look at `Knob.h`.
        
        // Assuming we fix `Knob` to store ID, let's implement the logic assuming `getComponentID()` or property holds it.
        // Better: store paramID in separate property "paramId".
        
        auto paramId = getProperties().getWithDefault("paramId", "").toString();
        
        // If empty, maybe fall back to name or debug warning.
        if (paramId.isEmpty()) { 
            // Try to guess or show error
            // For now, let's just log or ignore.
             juce::Logger::writeToLog("ScaledSlider: No paramId property set for MIDI Learn.");
             return;
        }

        auto& proc = czEditor->getAudioProcessor(); // We need public accessor in Editor
        
        if (result == 1) // Learn
        {
            proc.getMidiProcessor().learnNextCC(paramId.toStdString());
            
            // Visual feedback (optional)
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, 
                "MIDI Learn", 
                "Move a MIDI controller knob/slider to map it to " + getName());
        }
        else if (result == 2) // Unlearn
        {
            int cc = proc.getMidiProcessor().getCCForParam(paramId.toStdString());
            if (cc != -1) {
                proc.getMidiProcessor().unmapCC(cc);
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                    "MIDI Learn",
                    "MIDI mapping cleared for " + getName());
            }
        }
    }
}

} // namespace UI
} // namespace CZ101
