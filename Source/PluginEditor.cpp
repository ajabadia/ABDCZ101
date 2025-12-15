/*
  ==============================================================================

    PluginEditor.cpp
    Created: 14 Dec 2025
    Author:  Antigravity

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "State/Parameters.h" 

//==============================================================================
CZ101AudioProcessorEditor::CZ101AudioProcessorEditor(CZ101AudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      // Oscillator 1
      osc1LevelKnob("Level"),
      osc2LevelKnob("Level"), osc2DetuneKnob("Detune"),
      hardSyncButton("Hard Sync"), ringModButton("Ring Mod"),
      glideKnob("Glide"),
      // Filter


      filterCutoffKnob("Cutoff"), filterResonanceKnob("Res."),
      // PITCH
      pitchEditor(p, CZ101::UI::EnvelopeEditor::EnvelopeType::PITCH),
      // DCW
      dcwEditor(p, CZ101::UI::EnvelopeEditor::EnvelopeType::DCW),
      dcwAttackKnob("Att"), dcwDecayKnob("Dec"), dcwSustainKnob("Sus"), dcwReleaseKnob("Rel"),
      // DCA
      dcaEditor(p, CZ101::UI::EnvelopeEditor::EnvelopeType::DCA),
      dcaAttackKnob("Att"), dcaDecayKnob("Dec"), dcaSustainKnob("Sus"), dcaReleaseKnob("Rel"),
      // Effects
      delayTimeKnob("Time"), delayFeedbackKnob("Fdbk"), delayMixKnob("Mix"),
      chorusRateKnob("Rate"), chorusDepthKnob("Dep"), chorusMixKnob("Mix"),
      reverbSizeKnob("Size"), reverbMixKnob("Mix"),
      lfoRateKnob("Rate"),
      
      filterGroup("grpFilter", "FILTER"),
      lfoGroup("grpLfo", "LFO"),
      delayGroup("grpDelay", "DELAY"),
      chorusGroup("grpChorus", "CHORUS"),
      reverbGroup("grpReverb", "REVERB"),
      
      keyboardComponent(keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    juce::LookAndFeel::setDefaultLookAndFeel(&customLookAndFeel);
    
    // --- HEADER ---
    addAndMakeVisible(lcdDisplay);
    lcdDisplay.setText("CZ-101 EMULATOR", "PRESET: INIT");

    addAndMakeVisible(presetBrowser);
    presetBrowser.setPresetManager(&audioProcessor.getPresetManager());
    presetBrowser.onPresetSelected = [this](int index) {
        juce::ignoreUnused(index);
        auto name = audioProcessor.getPresetManager().getCurrentPreset().name;
        lcdDisplay.setText("CZ-101 EMULATOR", "PRESET: " + juce::String(name));
    };
    
    addAndMakeVisible(midiIndicator);
    
    addAndMakeVisible(midiOutputSelector);
    midiOutputSelector.setTextWhenNoChoicesAvailable("No MIDI Out");
    midiOutputSelector.onChange = [this] { 
        int selectedIndex = midiOutputSelector.getSelectedItemIndex();
        int deviceIndex = selectedIndex - 1;
        auto devices = juce::MidiOutput::getAvailableDevices();
        
        if (deviceIndex >= 0 && deviceIndex < devices.size())
            activeMidiOutput = juce::MidiOutput::openDevice(devices[deviceIndex].identifier);
        else
            activeMidiOutput.reset();
    };
    refreshMidiOutputs();
    
    addAndMakeVisible(loadSysExButton);
    loadSysExButton.onClick = [this] { loadSysExFile(); };

    // --- OSCILLATORS ---
    addAndMakeVisible(osc1WaveSelector);
    osc1WaveSelector.addItemList({"Sine", "Sawtooth", "Square", "Triangle"}, 1);
    addAndMakeVisible(osc1LevelKnob);
    
    addAndMakeVisible(osc2WaveSelector);
    osc2WaveSelector.addItemList({"Sine", "Sawtooth", "Square", "Triangle"}, 1);
    addAndMakeVisible(osc2LevelKnob);
    addAndMakeVisible(osc2DetuneKnob);
    addAndMakeVisible(hardSyncButton);
    addAndMakeVisible(ringModButton);
    addAndMakeVisible(glideKnob);

    // --- FILTER ---
    addAndMakeVisible(filterCutoffKnob);
    addAndMakeVisible(filterResonanceKnob);
    
    // --- ENVELOPES ---
    // Editors
    addAndMakeVisible(pitchEditor);
    addAndMakeVisible(dcwEditor);
    addAndMakeVisible(dcaEditor);
    
    // Knobs
    addAndMakeVisible(dcwAttackKnob);
    addAndMakeVisible(dcwDecayKnob);
    addAndMakeVisible(dcwSustainKnob);
    addAndMakeVisible(dcwReleaseKnob);
    
    addAndMakeVisible(dcaAttackKnob);
    addAndMakeVisible(dcaDecayKnob);
    addAndMakeVisible(dcaSustainKnob);
    addAndMakeVisible(dcaReleaseKnob);
    
    addAndMakeVisible(waveformDisplay); 
    
    /* 
       NOTE: Logic that was accidentally pasted into initializer list 
       is effectively restored here by ensuring 'addAndMakeVisible' calls above 
       and 'attachments' below are correct.
    */

    // --- EFFECTS ---
    addAndMakeVisible(filterGroup);
    addAndMakeVisible(lfoGroup);
    addAndMakeVisible(delayGroup);
    addAndMakeVisible(chorusGroup);
    addAndMakeVisible(reverbGroup);

    addAndMakeVisible(delayTimeKnob);
    addAndMakeVisible(delayFeedbackKnob);
    addAndMakeVisible(delayMixKnob);
    
    addAndMakeVisible(chorusRateKnob);
    addAndMakeVisible(chorusDepthKnob);
    addAndMakeVisible(chorusMixKnob);
    
    addAndMakeVisible(reverbSizeKnob);
    addAndMakeVisible(reverbMixKnob);
    
    addAndMakeVisible(lfoRateKnob);

    // --- KEYBOARD ---
    addAndMakeVisible(keyboardComponent);
    keyboardState.addListener(this);

    // --- ATTACHMENTS ---
    auto& params = audioProcessor.getParameters();
    
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
    
    if (params.hardSync)
        hardSyncAttachment = std::make_unique<ButtonAttachment>(*params.hardSync, hardSyncButton);

    if (params.ringMod)
        ringModAttachment = std::make_unique<ButtonAttachment>(*params.ringMod, ringModButton);
    
    if (params.glideTime)
        attachSlider(params.glideTime, glideKnob);
    
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

    attachSlider(params.chorusRate, chorusRateKnob);
    attachSlider(params.chorusDepth, chorusDepthKnob);
    attachSlider(params.chorusMix, chorusMixKnob);
    
    attachSlider(params.reverbSize, reverbSizeKnob);
    attachSlider(params.reverbMix, reverbMixKnob);
    
    attachSlider(params.lfoRate, lfoRateKnob);

    setSize(900, 850); 
    startTimerHz(4); // Update UI 4 times per second
}

CZ101AudioProcessorEditor::~CZ101AudioProcessorEditor()
{
    stopTimer();
    keyboardState.removeListener(this);
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

//==============================================================================
void CZ101AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff181818));
    
    g.setColour(juce::Colours::cyan);
    g.setFont(14.0f);
    
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(60); // Header
    area.removeFromTop(10); // Spacer

    // Osc
    auto oscArea = area.removeFromTop(90);
    g.setColour(juce::Colour(0xff004400));
    g.fillRect(oscArea);
    g.setColour(juce::Colours::white);
    g.drawText("DCO (OSCILLATORS)", oscArea.removeFromTop(20), juce::Justification::centred);
    
    area.removeFromTop(10);
    
    // Waveform
    area.removeFromTop(60);
    area.removeFromTop(10);
    
    // Envelopes
    auto envArea = area.removeFromTop(200);
    g.setColour(juce::Colour(0xff440000));
    g.fillRect(envArea);
    g.setColour(juce::Colours::white);
    
    // Updated Headers for 3 Envelopes
    // PITCH (Left), DCW (Mid), DCA (Right)
    // We can rely on visual separation or draw text in columns
    // Let's just draw centered title and let the sub-labels (if any) handle it.
    // Or split the area here for text.
    
    auto titleArea = envArea.removeFromTop(20);
    auto w3 = titleArea.getWidth() / 3;
    g.drawText("PITCH (DCO)", titleArea.removeFromLeft(w3), juce::Justification::centred);
    g.drawText("TIMBRE (DCW)", titleArea.removeFromLeft(w3), juce::Justification::centred);
    g.drawText("AMP (DCA)", titleArea, juce::Justification::centred);
    
    area.removeFromTop(10);
    
    // Filter/FX
    // Filter/FX
    auto fxArea = area.removeFromTop(120); // Height increased for Groups
    // Text is handled by GroupComponents now
    
    // Background separation
    g.setColour(juce::Colour(0xff000044)); 
    g.fillRect(fxArea);
}

void CZ101AudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    
    // HEADER
    auto headerArea = area.removeFromTop(60);
    lcdDisplay.setBounds(headerArea.removeFromLeft(250).reduced(5));
    midiIndicator.setBounds(headerArea.removeFromRight(40).reduced(5));
    midiOutputSelector.setBounds(headerArea.removeFromRight(150).reduced(5, 15));
    // Load Button next to Preset Browser
    loadSysExButton.setBounds(headerArea.removeFromRight(80).reduced(5, 15));
    
    presetBrowser.setBounds(headerArea.reduced(5));

    area.removeFromTop(10);

    // OSCILLATORS
    auto oscArea = area.removeFromTop(90);
    oscArea.removeFromTop(20);
    
    juce::FlexBox oscBox;
    oscBox.justifyContent = juce::FlexBox::JustifyContent::center;
    
    oscBox.items.add(juce::FlexItem(osc1WaveSelector).withWidth(100).withHeight(30).withMargin({20, 10, 0, 10}));
    oscBox.items.add(juce::FlexItem(osc1LevelKnob).withWidth(70).withHeight(70));
    
    oscBox.items.add(juce::FlexItem().withWidth(20));
    
    oscBox.items.add(juce::FlexItem(osc2WaveSelector).withWidth(100).withHeight(30).withMargin({20, 10, 0, 10}));
    oscBox.items.add(juce::FlexItem(osc2LevelKnob).withWidth(70).withHeight(70));
    oscBox.items.add(juce::FlexItem(osc2DetuneKnob).withWidth(70).withHeight(70));
    
    // Buttons container? Or just add them
    oscBox.items.add(juce::FlexItem(hardSyncButton).withWidth(80).withHeight(30).withMargin({20, 0, 0, 5}));
    oscBox.items.add(juce::FlexItem(ringModButton).withWidth(80).withHeight(30).withMargin({20, 0, 0, 5}));
    
    // Glide Knob
    oscBox.items.add(juce::FlexItem(glideKnob).withWidth(60).withHeight(60).withMargin({0, 0, 0, 10}));

    oscBox.performLayout(oscArea);
    
    area.removeFromTop(10);

    // WAVEFORM
    auto vizArea = area.removeFromTop(60);
    waveformDisplay.setBounds(vizArea.reduced(40, 5));
    
    area.removeFromTop(10);

    // ENVELOPES
    auto envArea = area.removeFromTop(200);
    envArea.removeFromTop(20); 
    
    // Split into 3
    int w3 = envArea.getWidth() / 3;
    auto pitchArea = envArea.removeFromLeft(w3).reduced(5);
    auto dcwArea = envArea.removeFromLeft(w3).reduced(5);
    auto dcaArea = envArea.reduced(5);
    
    // PITCH (Only Editor, no ADSR knobs for Pitch yet? Or reuse? We don't have ADSR knobs for Pitch exposed in Processor params)
    // So Pitch is just the graph.
    pitchEditor.setBounds(pitchArea.removeFromTop(100)); 
    // Spacer below pitch?
    
    // DCW
    dcwEditor.setBounds(dcwArea.removeFromTop(100));
    juce::FlexBox dcwKnobs;
    dcwKnobs.justifyContent = juce::FlexBox::JustifyContent::center;
    dcwKnobs.items.add(juce::FlexItem(dcwAttackKnob).withWidth(40).withHeight(75)); 
    dcwKnobs.items.add(juce::FlexItem(dcwDecayKnob).withWidth(40).withHeight(75));
    dcwKnobs.items.add(juce::FlexItem(dcwSustainKnob).withWidth(40).withHeight(75));
    dcwKnobs.items.add(juce::FlexItem(dcwReleaseKnob).withWidth(40).withHeight(75));
    dcwKnobs.performLayout(dcwArea);
    
    // DCA
    dcaEditor.setBounds(dcaArea.removeFromTop(100));
    juce::FlexBox dcaKnobs;
    dcaKnobs.justifyContent = juce::FlexBox::JustifyContent::center;
    dcaKnobs.items.add(juce::FlexItem(dcaAttackKnob).withWidth(40).withHeight(75));
    dcaKnobs.items.add(juce::FlexItem(dcaDecayKnob).withWidth(40).withHeight(75));
    dcaKnobs.items.add(juce::FlexItem(dcaSustainKnob).withWidth(40).withHeight(75));
    dcaKnobs.items.add(juce::FlexItem(dcaReleaseKnob).withWidth(40).withHeight(75));
    dcaKnobs.performLayout(dcaArea);

    area.removeFromTop(10);

    // FILTER/FX
    auto fxArea = area.removeFromTop(120);
    
    // 5 Columns now ? (Filter, LFO, Delay, Chorus, Reverb)
    int colW = fxArea.getWidth() / 5;
    
    auto filterArea = fxArea.removeFromLeft(colW).reduced(5);
    auto lfoArea = fxArea.removeFromLeft(colW).reduced(5);
    auto delayArea = fxArea.removeFromLeft(colW).reduced(5);
    auto chorusArea = fxArea.removeFromLeft(colW).reduced(5);
    auto reverbArea = fxArea.reduced(5);
    
    // ... (Filter/LFO same) ...
    // Filter
    filterGroup.setBounds(filterArea);
    juce::FlexBox fltBox; fltBox.justifyContent = juce::FlexBox::JustifyContent::center;
    fltBox.items.add(juce::FlexItem(filterCutoffKnob).withWidth(70).withHeight(75).withMargin({15, 0, 0, 0}));
    fltBox.items.add(juce::FlexItem(filterResonanceKnob).withWidth(70).withHeight(75).withMargin({15, 0, 0, 0}));
    fltBox.performLayout(filterArea);
    
    // LFO
    lfoGroup.setBounds(lfoArea);
    juce::FlexBox lfoBox; lfoBox.justifyContent = juce::FlexBox::JustifyContent::center;
    lfoBox.items.add(juce::FlexItem(lfoRateKnob).withWidth(70).withHeight(75).withMargin({15, 0, 0, 0}));
    lfoBox.performLayout(lfoArea);

    // Delay
    delayGroup.setBounds(delayArea);
    juce::FlexBox dlyBox; dlyBox.justifyContent = juce::FlexBox::JustifyContent::center;
    // Reduce width to fit 5 cols
    dlyBox.items.add(juce::FlexItem(delayTimeKnob).withWidth(50).withHeight(75).withMargin({15, 0, 0, 0}));
    dlyBox.items.add(juce::FlexItem(delayFeedbackKnob).withWidth(50).withHeight(75).withMargin({15, 0, 0, 0}));
    dlyBox.items.add(juce::FlexItem(delayMixKnob).withWidth(50).withHeight(75).withMargin({15, 0, 0, 0}));
    dlyBox.performLayout(delayArea);
    
    // Chorus
    chorusGroup.setBounds(chorusArea);
    juce::FlexBox choBox; choBox.justifyContent = juce::FlexBox::JustifyContent::center;
    choBox.items.add(juce::FlexItem(chorusRateKnob).withWidth(50).withHeight(75).withMargin({15, 0, 0, 0}));
    choBox.items.add(juce::FlexItem(chorusDepthKnob).withWidth(50).withHeight(75).withMargin({15, 0, 0, 0}));
    choBox.items.add(juce::FlexItem(chorusMixKnob).withWidth(50).withHeight(75).withMargin({15, 0, 0, 0}));
    choBox.performLayout(chorusArea);

    // Reverb
    reverbGroup.setBounds(reverbArea);
    juce::FlexBox revBox; revBox.justifyContent = juce::FlexBox::JustifyContent::center;
    revBox.items.add(juce::FlexItem(reverbSizeKnob).withWidth(50).withHeight(75).withMargin({15, 0, 0, 0}));
    revBox.items.add(juce::FlexItem(reverbMixKnob).withWidth(50).withHeight(75).withMargin({15, 0, 0, 0}));
    revBox.performLayout(reverbArea);
    
    // KEYBOARD
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
    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(midiChannel, midiNoteNumber, velocity);
    audioProcessor.getMidiProcessor().processMessage(noteOn);
    midiIndicator.triggerActivity();
    if (activeMidiOutput) activeMidiOutput->sendMessageNow(noteOn);
}

void CZ101AudioProcessorEditor::handleNoteOff(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
    juce::MidiMessage noteOff = juce::MidiMessage::noteOff(midiChannel, midiNoteNumber, velocity);
    audioProcessor.getMidiProcessor().processMessage(noteOff);
    if (activeMidiOutput) activeMidiOutput->sendMessageNow(noteOff);
}

void CZ101AudioProcessorEditor::timerCallback()
{
    auto cpu = audioProcessor.getPerformanceMonitor().getAverageCpuUsage() * 100.0;
    auto preset = audioProcessor.getPresetManager().getCurrentPreset().name;
    
    // Format: "CZ-101  CPU: 12.5%"
    juce::String cpuStr = juce::String(cpu, 1) + "%";
    
    // Pad CPU string
    while (cpuStr.length() < 5) cpuStr = " " + cpuStr;
    
    lcdDisplay.setText("CZ-101   CPU:" + cpuStr, "PRESET: " + juce::String(preset));
    
    // Update waveform (if needed frequently, though it usually pulls from buffer)
    waveformDisplay.repaint();
    
    // Refresh Envelope UI from Engine State (in case preset changed)
    // We do this periodically to catch preset changes without a dedicated listener system
    pitchEditor.updateData();
    dcwEditor.updateData();
    dcaEditor.updateData();
}

bool CZ101AudioProcessorEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto file : files)
    {
        if (file.endsWithIgnoreCase(".syx"))
            return true;
    }
    return false;
}

void CZ101AudioProcessorEditor::filesDropped (const juce::StringArray& files, int x, int y)
{
    juce::ignoreUnused(x, y);
    
    for (auto path : files)
    {
        if (path.endsWithIgnoreCase(".syx"))
        {
            juce::File file(path);
            juce::MemoryBlock sysexData;
            
            if (file.loadFileAsData(sysexData))
            {
                // Pass to SysExManager via Processor
                // SysExManager takes raw void* and size
                audioProcessor.getSysExManager().handleSysEx(sysexData.getData(), (int)sysexData.getSize());
                
                // Explicitly refresh UI immediately (though timer fits eventually)
                timerCallback(); 
                
                // Flash indicator or show message?
                // For now, rely on LCD updating "PRESET: [Name]" which happens in timerCallback
            }
        }
    }
}

void CZ101AudioProcessorEditor::loadSysExFile()
{
    fileChooser = std::make_unique<juce::FileChooser>("Load SysEx Patch",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.syx");

    auto folderChooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(folderChooserFlags, [this](const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file.existsAsFile())
        {
            juce::MemoryBlock sysexData;
            if (file.loadFileAsData(sysexData))
            {
                audioProcessor.getSysExManager().handleSysEx(sysexData.getData(), (int)sysexData.getSize());
                timerCallback();
            }
        }
    });
}
