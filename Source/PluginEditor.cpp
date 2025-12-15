/*
  ==============================================================================

    PluginEditor.cpp
    Created: 14 Dec 2025
    Author:  Antigravity

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "State/Parameters.h" // Full definition needed

//==============================================================================
CZ101AudioProcessorEditor::CZ101AudioProcessorEditor(CZ101AudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      // Oscillator 1
      osc1LevelKnob("Level"),
      osc2LevelKnob("Level"), osc2DetuneKnob("Detune"),
      // Filter
      filterCutoffKnob("Cutoff"), filterResonanceKnob("Res."),
      // DCW (Timbre)
      dcwAttackKnob("Attack"), dcwDecayKnob("Decay"), dcwSustainKnob("Sus"), dcwReleaseKnob("Rel"),
      // DCA (Amp)
      dcaAttackKnob("Attack"), dcaDecayKnob("Decay"), dcaSustainKnob("Sus"), dcaReleaseKnob("Rel"),
      // Effects / LFO
      delayTimeKnob("Time"), delayFeedbackKnob("Fdbk"), delayMixKnob("Mix"),
      lfoRateKnob("Rate"),
      // MIDI Keyboard
      keyboardComponent(keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    // --- LOOK AND FEEL ---
    juce::LookAndFeel::setDefaultLookAndFeel(&customLookAndFeel);
    
    // --- HEADER START ---
    addAndMakeVisible(lcdDisplay);
    lcdDisplay.setText("CZ-101 EMULATOR", "PRESET: INIT");

    addAndMakeVisible(presetBrowser);
    presetBrowser.setPresetManager(&audioProcessor.getPresetManager());
    presetBrowser.onPresetSelected = [this](int index) {
        auto name = audioProcessor.getPresetManager().getCurrentPreset().name;
        lcdDisplay.setText("CZ-101 EMULATOR", "PRESET: " + juce::String(name));
    };
    
    addAndMakeVisible(midiIndicator);
    
    addAndMakeVisible(midiOutputSelector);
    midiOutputSelector.setTextWhenNoChoicesAvailable("No MIDI Out");
    midiOutputSelector.onChange = [this] { 
        int selectedIndex = midiOutputSelector.getSelectedItemIndex();
        
        // Items: 0="No MIDI Out", 1..N = Devices
        // So device index is selectedIndex - 1
        int deviceIndex = selectedIndex - 1;
        
        auto devices = juce::MidiOutput::getAvailableDevices();
        
        if (deviceIndex >= 0 && deviceIndex < devices.size())
        {
            activeMidiOutput = juce::MidiOutput::openDevice(devices[deviceIndex].identifier);
        }
        else
        {
            activeMidiOutput.reset();
        }
    };
    refreshMidiOutputs();
    // --- HEADER END ---

    // --- OSCILLATORS ---
    addAndMakeVisible(osc1WaveSelector);
    osc1WaveSelector.addItemList({"Sine", "Sawtooth", "Square", "Triangle"}, 1);
    addAndMakeVisible(osc1LevelKnob);
    
    addAndMakeVisible(osc2WaveSelector);
    osc2WaveSelector.addItemList({"Sine", "Sawtooth", "Square", "Triangle"}, 1);
    addAndMakeVisible(osc2LevelKnob);
    addAndMakeVisible(osc2DetuneKnob);

    // --- FILTER ---
    addAndMakeVisible(filterCutoffKnob);
    addAndMakeVisible(filterResonanceKnob);
    
    // --- ENVELOPES ---
    // DCW
    addAndMakeVisible(dcwAttackKnob);
    addAndMakeVisible(dcwDecayKnob);
    addAndMakeVisible(dcwSustainKnob);
    addAndMakeVisible(dcwReleaseKnob);
    
    // DCA
    addAndMakeVisible(dcaAttackKnob);
    addAndMakeVisible(dcaDecayKnob);
    addAndMakeVisible(dcaSustainKnob);
    addAndMakeVisible(dcaReleaseKnob);
    
    addAndMakeVisible(waveformDisplay); // Visualizer

    // --- EFFECTS & LFO ---
    addAndMakeVisible(delayTimeKnob);
    addAndMakeVisible(delayFeedbackKnob);
    addAndMakeVisible(delayMixKnob);
    addAndMakeVisible(lfoRateKnob);

    // --- KEYBOARD ---
    addAndMakeVisible(keyboardComponent);
    keyboardState.addListener(this);

    // --- ATTACHMENTS ---
    // Connect UI to Audio Parameters
    auto& params = audioProcessor.getParameters();
    
    // Helper lambda for safer attachment
    auto attachSlider = [&](juce::AudioParameterFloat* param, juce::Slider& slider) {
        if (param) sliderAttachments.emplace_back(std::make_unique<SliderAttachment>(*param, slider));
    };
    
    auto attachCombo = [&](juce::AudioParameterChoice* param, juce::ComboBox& box) {
        if (param) comboBoxAttachments.emplace_back(std::make_unique<ComboBoxAttachment>(*param, box));
    };

    attachSlider(params.osc1Level, osc1LevelKnob);
    attachCombo(params.osc1Waveform, osc1WaveSelector);
    
    attachSlider(params.osc2Level, osc2LevelKnob);
    attachSlider(params.osc2Detune, osc2DetuneKnob);
    attachCombo(params.osc2Waveform, osc2WaveSelector);
    
    attachSlider(params.filterCutoff, filterCutoffKnob);
    attachSlider(params.filterResonance, filterResonanceKnob);
    
    attachSlider(params.dcwAttack, dcwAttackKnob);
    attachSlider(params.dcwDecay, dcwDecayKnob);
    attachSlider(params.dcwSustain, dcwSustainKnob);
    attachSlider(params.dcwRelease, dcwReleaseKnob);
    
    attachSlider(params.dcaAttack, dcaAttackKnob);
    attachSlider(params.dcaDecay, dcaDecayKnob);
    attachSlider(params.dcaSustain, dcaSustainKnob);
    attachSlider(params.dcaRelease, dcaReleaseKnob);
    
    attachSlider(params.delayTime, delayTimeKnob);
    attachSlider(params.delayFeedback, delayFeedbackKnob);
    attachSlider(params.delayMix, delayMixKnob);
    
    attachSlider(params.lfoRate, lfoRateKnob);

    // Window setup
    setSize(900, 700);
}

CZ101AudioProcessorEditor::~CZ101AudioProcessorEditor()
{
    keyboardState.removeListener(this);
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

//==============================================================================
void CZ101AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF2A2A2A)); // Dark Charcoal

    // Section Titles
}

void CZ101AudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    
    // --- HEADER (60px) ---
    auto headerArea = area.removeFromTop(60);
    lcdDisplay.setBounds(headerArea.removeFromLeft(250).reduced(5));
    midiIndicator.setBounds(headerArea.removeFromRight(40).reduced(5));
    midiOutputSelector.setBounds(headerArea.removeFromRight(150).reduced(5, 15));
    presetBrowser.setBounds(headerArea.reduced(5)); // Takes remaining center

    area.removeFromTop(10); // Spacer

    // --- OSCILLATORS (80px) ---
    auto oscArea = area.removeFromTop(90);
    // Label for section? Drawn in paint? For now just layout components
    
    juce::FlexBox oscBox;
    oscBox.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    
    // DCO 1
    oscBox.items.add(juce::FlexItem(osc1WaveSelector).withWidth(100).withHeight(30).withMargin({10, 10, 10, 10}));
    oscBox.items.add(juce::FlexItem(osc1LevelKnob).withWidth(60).withHeight(80));
    
    // DCO 2
    oscBox.items.add(juce::FlexItem(osc2WaveSelector).withWidth(100).withHeight(30).withMargin({10, 10, 10, 10}));
    oscBox.items.add(juce::FlexItem(osc2LevelKnob).withWidth(60).withHeight(80));
    oscBox.items.add(juce::FlexItem(osc2DetuneKnob).withWidth(60).withHeight(80));

    oscBox.performLayout(oscArea);

    area.removeFromTop(10);

    // --- WAVEFORM VISUALIZATION (80px) ---
    auto vizArea = area.removeFromTop(80);
    waveformDisplay.setBounds(vizArea.reduced(40, 5));
    
    area.removeFromTop(10);

    // --- ENVELOPES (100px) ---
    auto envArea = area.removeFromTop(100);
    juce::FlexBox envBox;
    
    // Labels should be added, but for now knobs have internal labels
    envBox.items.add(juce::FlexItem(dcwAttackKnob).withWidth(60).withHeight(80));
    envBox.items.add(juce::FlexItem(dcwDecayKnob).withWidth(60).withHeight(80));
    envBox.items.add(juce::FlexItem(dcwSustainKnob).withWidth(60).withHeight(80));
    envBox.items.add(juce::FlexItem(dcwReleaseKnob).withWidth(60).withHeight(80));
    
    // Spacer between DCW and DCA
    envBox.items.add(juce::FlexItem().withWidth(40));
    
    envBox.items.add(juce::FlexItem(dcaAttackKnob).withWidth(60).withHeight(80));
    envBox.items.add(juce::FlexItem(dcaDecayKnob).withWidth(60).withHeight(80));
    envBox.items.add(juce::FlexItem(dcaSustainKnob).withWidth(60).withHeight(80));
    envBox.items.add(juce::FlexItem(dcaReleaseKnob).withWidth(60).withHeight(80));
    
    envBox.performLayout(envArea);

    area.removeFromTop(10);

    // --- FILTER & EFFECTS & LFO (90px) ---
    auto fxArea = area.removeFromTop(90);
    juce::FlexBox fxBox;
    
    fxBox.items.add(juce::FlexItem(filterCutoffKnob).withWidth(60).withHeight(80));
    fxBox.items.add(juce::FlexItem(filterResonanceKnob).withWidth(60).withHeight(80));
    
    fxBox.items.add(juce::FlexItem().withWidth(40)); // Spacer
    
    fxBox.items.add(juce::FlexItem(lfoRateKnob).withWidth(60).withHeight(80));
    
    fxBox.items.add(juce::FlexItem().withWidth(40)); // Spacer
    
    fxBox.items.add(juce::FlexItem(delayTimeKnob).withWidth(60).withHeight(80));
    fxBox.items.add(juce::FlexItem(delayFeedbackKnob).withWidth(60).withHeight(80));
    fxBox.items.add(juce::FlexItem(delayMixKnob).withWidth(60).withHeight(80));
    
    fxBox.performLayout(fxArea);
    
    // --- KEYBOARD (Bottom) ---
    auto keyboardArea = area.removeFromBottom(80);
    keyboardComponent.setBounds(keyboardArea);
}

void CZ101AudioProcessorEditor::refreshMidiOutputs()
{
    auto devices = juce::MidiOutput::getAvailableDevices();
    midiOutputSelector.clear();
    midiOutputSelector.addItem("No MIDI Out", 1);
    
    int i = 2;
    for (const auto& dev : devices)
    {
        midiOutputSelector.addItem(dev.name, i++);
    }
    midiOutputSelector.setSelectedId(1);
}

void CZ101AudioProcessorEditor::handleNoteOn(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
    // Send to internal engine
    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(midiChannel, midiNoteNumber, velocity);
    audioProcessor.getMidiProcessor().processMessage(noteOn);
    
    // Indicate
    midiIndicator.triggerActivity();
    
    // Send to hardware output if active
    if (activeMidiOutput)
    {
        activeMidiOutput->sendMessageNow(noteOn);
    }
}

void CZ101AudioProcessorEditor::handleNoteOff(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
    // Send to internal engine
    juce::MidiMessage noteOff = juce::MidiMessage::noteOff(midiChannel, midiNoteNumber, velocity);
    audioProcessor.getMidiProcessor().processMessage(noteOff);
    
    // Send to hardware output if active
    if (activeMidiOutput)
    {
        activeMidiOutput->sendMessageNow(noteOff);
    }
}
