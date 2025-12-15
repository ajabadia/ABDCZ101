#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "UI/CZ101LookAndFeel.h"
#include "UI/Components/Knob.h"
#include "UI/Components/WaveformDisplay.h"
#include "UI/Components/PresetBrowser.h"
#include "UI/Components/MIDIActivityIndicator.h"
#include "UI/LCDDisplay.h"

/**
 * CZ-101 Emulator Editor
 * 
 * Interfaz gráfica completa del plugin con todos los componentes UI
 */
class CZ101AudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::MidiKeyboardState::Listener
{
public:
    CZ101AudioProcessorEditor(CZ101AudioProcessor&);
    ~CZ101AudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;
    
    juce::MidiKeyboardState& getKeyboardState() { return keyboardState; }

    // Helper to refresh MIDI devices
    void refreshMidiOutputs();

private:
    // MidiKeyboardState::Listener
    void handleNoteOn(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    CZ101AudioProcessor& audioProcessor;
    
    // Custom Look and Feel
    CZ101::UI::CZ101LookAndFeel customLookAndFeel;
    
    // MIDI Infrastructure
    juce::MidiKeyboardState keyboardState;
    juce::MidiKeyboardComponent keyboardComponent;
    juce::ComboBox midiOutputSelector;
    std::unique_ptr<juce::MidiOutput> activeMidiOutput;
    
    // Header Components
    CZ101::UI::LCDDisplay lcdDisplay;
    CZ101::UI::PresetBrowser presetBrowser;
    CZ101::UI::MIDIActivityIndicator midiIndicator;
    CZ101::UI::WaveformDisplay waveformDisplay;
    
    // --- OSCILLATOR SECTION ---
    juce::ComboBox osc1WaveSelector;
    CZ101::UI::Knob osc1LevelKnob;
    
    juce::ComboBox osc2WaveSelector;
    CZ101::UI::Knob osc2LevelKnob;
    CZ101::UI::Knob osc2DetuneKnob;
    
    // --- FILTER SECTION ---
    CZ101::UI::Knob filterCutoffKnob;
    CZ101::UI::Knob filterResonanceKnob;
    
    // --- ENVELOPE SECTIONS ---
    // DCW (Timbre)
    CZ101::UI::Knob dcwAttackKnob;
    CZ101::UI::Knob dcwDecayKnob;
    CZ101::UI::Knob dcwSustainKnob;
    CZ101::UI::Knob dcwReleaseKnob;
    
    // DCA (Amplitude) -> Reusing generic names for clarity
    CZ101::UI::Knob dcaAttackKnob;
    CZ101::UI::Knob dcaDecayKnob;
    CZ101::UI::Knob dcaSustainKnob;
    CZ101::UI::Knob dcaReleaseKnob;
    
    // --- EFFECTS & LFO ---
    CZ101::UI::Knob delayTimeKnob;
    CZ101::UI::Knob delayFeedbackKnob;
    CZ101::UI::Knob delayMixKnob;
    CZ101::UI::Knob lfoRateKnob;
    
    // --- ATTACHMENTS ---
    // Using ParameterAttachment because we manage parameters manually (no APVTS)
    using SliderAttachment = juce::SliderParameterAttachment;
    using ComboBoxAttachment = juce::ComboBoxParameterAttachment;
    
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ComboBoxAttachment>> comboBoxAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CZ101AudioProcessorEditor)
};
