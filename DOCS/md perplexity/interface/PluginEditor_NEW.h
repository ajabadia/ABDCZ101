#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "UI/CZ101LookAndFeel.h"
#include "UI/Components/Knob.h"
#include "UI/Components/WaveformDisplay.h"
#include "UI/Components/PresetBrowser.h"
#include "UI/Components/MIDIActivityIndicator.h"
#include "UI/Components/EnvelopeEditor.h"
#include "UI/LCDDisplay.h"

/**
 * CZ-101 Emulator Editor - REDESIGNED
 * 
 * Nuevo layout 800x600 (responsive):
 * - Header compacto (45px)
 * - Left Panel: Osciladores, Filter, LFO
 * - Right Panel: Envelopes (tabs), Effects (2x3 grid)
 * - Bottom: Keyboard MIDI
 * 
 * Optimizado para Raspberry Pi y pantallas pequeñas
 */
class CZ101AudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::MidiKeyboardState::Listener,
                                   private juce::Timer,
                                   public juce::FileDragAndDropTarget
{
public:
    CZ101AudioProcessorEditor(CZ101AudioProcessor&);
    ~CZ101AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    
    juce::MidiKeyboardState& getKeyboardState() { return keyboardState; }
    void refreshMidiOutputs();
    void timerCallback() override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    void handleNoteOn(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    
    CZ101AudioProcessor& audioProcessor;
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
    juce::TextButton loadSysExButton { "LOAD SYX" };
    std::unique_ptr<juce::FileChooser> fileChooser;
    void loadSysExFile();
    
    // --- LEFT PANEL: OSCILLATORS + FILTER + LFO ---
    
    // Oscillators
    juce::ComboBox osc1WaveSelector;
    CZ101::UI::Knob osc1LevelKnob;
    juce::ComboBox osc2WaveSelector;
    CZ101::UI::Knob osc2LevelKnob;
    CZ101::UI::Knob osc2DetuneKnob;
    juce::ToggleButton hardSyncButton;
    juce::ToggleButton ringModButton;
    CZ101::UI::Knob glideKnob;
    CZ101::UI::WaveformDisplay waveformDisplay;
    
    // Filter
    CZ101::UI::Knob filterCutoffKnob;
    CZ101::UI::Knob filterResonanceKnob;
    
    // LFO
    CZ101::UI::Knob lfoRateKnob;
    
    // --- RIGHT PANEL: ENVELOPES (TABS) + EFFECTS GRID ---
    
    // Envelope Tabs
    juce::TabbedComponent envelopeTabs { juce::TabbedButtonBar::TabsAtTop };
    
    // Envelopes
    CZ101::UI::EnvelopeEditor pitchEditor;
    CZ101::UI::EnvelopeEditor dcwEditor;
    CZ101::UI::EnvelopeEditor dcaEditor;
    
    // Envelope ADSR Knobs (DCW)
    CZ101::UI::Knob dcwAttackKnob;
    CZ101::UI::Knob dcwDecayKnob;
    CZ101::UI::Knob dcwSustainKnob;
    CZ101::UI::Knob dcwReleaseKnob;
    
    // Envelope ADSR Knobs (DCA)
    CZ101::UI::Knob dcaAttackKnob;
    CZ101::UI::Knob dcaDecayKnob;
    CZ101::UI::Knob dcaSustainKnob;
    CZ101::UI::Knob dcaReleaseKnob;
    
    // Effects Grid (2x3)
    CZ101::UI::Knob delayTimeKnob;
    CZ101::UI::Knob delayFeedbackKnob;
    CZ101::UI::Knob delayMixKnob;
    CZ101::UI::Knob chorusRateKnob;
    CZ101::UI::Knob chorusDepthKnob;
    CZ101::UI::Knob chorusMixKnob;
    CZ101::UI::Knob reverbSizeKnob;
    CZ101::UI::Knob reverbMixKnob;
    
    // Effect Labels (no GroupComponents, solo etiquetas)
    juce::Label delayLabel;
    juce::Label chorusLabel;
    juce::Label reverbLabel;
    
    // --- ATTACHMENTS ---
    using SliderAttachment = juce::SliderParameterAttachment;
    using ComboBoxAttachment = juce::ComboBoxParameterAttachment;
    using ButtonAttachment = juce::ButtonParameterAttachment;
    
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ComboBoxAttachment>> comboBoxAttachments;
    std::unique_ptr<ButtonAttachment> hardSyncAttachment;
    std::unique_ptr<ButtonAttachment> ringModAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CZ101AudioProcessorEditor)
};
