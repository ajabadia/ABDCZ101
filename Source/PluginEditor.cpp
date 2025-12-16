/*
  ==============================================================================
    PluginEditor.cpp - REDESIGNED for 800x600
    Layout: 2-column dashboard with tabs
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "State/Parameters.h" 

CZ101AudioProcessorEditor::CZ101AudioProcessorEditor(CZ101AudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      // Oscillators
      osc1LevelKnob("OSC1"), osc2LevelKnob("OSC2"), osc2DetuneKnob("DET"),
      hardSyncButton("HSync"), ringModButton("RMod"), glideKnob("GLIDE"),
      // Filter
      filterCutoffKnob("CUTOFF"), filterResonanceKnob("RES"),
      // Envelopes
      pitchEditor(p, CZ101::UI::EnvelopeEditor::EnvelopeType::PITCH),
      dcwEditor(p, CZ101::UI::EnvelopeEditor::EnvelopeType::DCW),
      dcaEditor(p, CZ101::UI::EnvelopeEditor::EnvelopeType::DCA),
      dcwAttackKnob("A"), dcwDecayKnob("D"), dcwSustainKnob("S"), dcwReleaseKnob("R"),
      dcaAttackKnob("A"), dcaDecayKnob("D"), dcaSustainKnob("S"), dcaReleaseKnob("R"),
      // Effects
      delayTimeKnob("TIME"), delayFeedbackKnob("FB"), delayMixKnob("MIX"),
      chorusRateKnob("RATE"), chorusDepthKnob("DEPTH"), chorusMixKnob("MIX"),
      reverbSizeKnob("SIZE"), reverbMixKnob("MIX"),
      lfoRateKnob("RATE"),
      // Keyboard
      keyboardComponent(keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    juce::LookAndFeel::setDefaultLookAndFeel(&customLookAndFeel);
    
    // Enable GPU Architecture (Safe for RPi)
    openGLContext.attachTo(*this);
    
    // ========== HEADER ==========
    addAndMakeVisible(lcdDisplay);
    // Init LCD with correct layout (Name Top, Stats Bottom)
    lcdDisplay.setText("Init", "CPU: 0.0%  SR: 44.1kHz  NOTE: --");
    
    addAndMakeVisible(presetBrowser);
    presetBrowser.setPresetManager(&audioProcessor.getPresetManager());
    presetBrowser.onPresetSelected = [this](int index) {
        juce::ignoreUnused(index);
        auto name = audioProcessor.getPresetManager().getCurrentPreset().name;
        // Keep synced with Timer format (Name Top)
        lcdDisplay.setText(juce::String(name), "LOADING...");
    };

    presetBrowser.onSaveRequested = [this]()
    {
        auto currentName = juce::String(audioProcessor.getPresetManager().getCurrentPreset().name);
        // Strip "(UNSAVED)" if present so the rename box is clean
        currentName = currentName.replace(" (UNSAVED)", "");
        
        nameOverlay.startRename(currentName, [this](const juce::String& newName)
        {
            auto& pm = audioProcessor.getPresetManager();
            // 1. Capture current implementation state (knobs etc) back to preset structure
            pm.copyStateFromProcessor();

            // 2. Save to internal memory (active slot)
            // Ideally we get the index from the Processor or Browser.
            // For now, we update the current one based on processor state, then persist bank.
            // Note: simple save to current slot logic:
            // We assume the user wants to overwrite the currently selected preset.
            // We need to find which index is active. 
            // Let's assume the preset manager or processor knows.
            // The browser triggers loadPreset(index).
            // But we don't store "currentPresetIndex" in manager? 
            // We'll iterate to find name match OR just save to a default User slot if unclear.
            // **Correction**: PresetBrowser has getSelectedItemIndex! But we are in Editor.
            // We can ask the browser:
            // 2. Save to internal memory (active slot)
            // Use the authoritative index from PresetManager
            int idx = pm.getCurrentPresetIndex(); 
            // Note: PresetManager index is 0-based.
            int presetIndex = idx; 
            
            // If checking Browser consistency:
            // int browserIdx = presetBrowser.getSelectedItemIndex() - 1;
            // if (browserIdx != presetIndex) { Logger::writeToLog("Warning: UI Index mismatch"); }

            if (presetIndex >= 0)
            {
                pm.savePreset(presetIndex, newName.toStdString());
                
                // 3. Persist to Disk
                juce::File defaultsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                                            .getChildFile("CZ101Emulator");
                if (!defaultsDir.exists()) defaultsDir.createDirectory();
                pm.saveBank(defaultsDir.getChildFile("user_bank.json"));
                
                // 4. Update UI
                audioProcessor.changeProgramName(presetIndex, newName);
                presetBrowser.updatePresetList();
                presetBrowser.updatePresetList();
                presetBrowser.setSelectedItemIndex(idx); // restore selection
                lcdDisplay.setText(newName, "SAVED OK");
            }
        });
        
        nameOverlay.setVisible(true);
        nameOverlay.toFront(true);
    };
    
    addAndMakeVisible(midiIndicator);
    addAndMakeVisible(midiOutputSelector);
    midiOutputSelector.setTextWhenNoChoicesAvailable("No MIDI");
    midiOutputSelector.onChange = [this] { 
        int idx = midiOutputSelector.getSelectedItemIndex();
        int devIdx = idx - 1;
        auto devices = juce::MidiOutput::getAvailableDevices();
        if (devIdx >= 0 && devIdx < devices.size())
            activeMidiOutput = juce::MidiOutput::openDevice(devices[devIdx].identifier);
        else
            activeMidiOutput.reset();
    };
    refreshMidiOutputs();
    
    addAndMakeVisible(loadSysExButton);
    loadSysExButton.onClick = [this] { loadSysExFile(); };
    
    addAndMakeVisible(saveSysExButton);
    saveSysExButton.onClick = [this] { saveSysExFile(); };
    
    // ========== LEFT PANEL: OSCILLATORS + FILTER + LFO ==========
    addAndMakeVisible(osc1WaveSelector);
    osc1WaveSelector.addItemList({"Sine", "Saw", "Square", "Triangle"}, 1);
    addAndMakeVisible(osc1LevelKnob);
    
    addAndMakeVisible(osc2WaveSelector);
    osc2WaveSelector.addItemList({"Sine", "Saw", "Square", "Triangle"}, 1);
    addAndMakeVisible(osc2LevelKnob);
    addAndMakeVisible(osc2DetuneKnob);
    addAndMakeVisible(hardSyncButton);
    addAndMakeVisible(ringModButton);
    addAndMakeVisible(glideKnob);
    
    addAndMakeVisible(filterCutoffKnob);
    addAndMakeVisible(filterResonanceKnob);
    addAndMakeVisible(lfoRateKnob);
    
    addAndMakeVisible(waveformDisplay);
    
    // ========== RIGHT PANEL: ENVELOPES + EFFECTS ==========
    addAndMakeVisible(envelopeTabs);
    
    // Envelope panels (content for tabs)
    // Envelope panels (content for tabs)
    auto pitchPanel = std::make_unique<juce::Component>();
    pitchPanel->addAndMakeVisible(pitchEditor);
    envelopeTabs.addTab("PITCH", juce::Colours::magenta, pitchPanel.release(), true);
    
    auto dcwPanel = std::make_unique<juce::Component>();
    dcwPanel->addAndMakeVisible(dcwEditor);
    dcwPanel->addAndMakeVisible(dcwAttackKnob);
    dcwPanel->addAndMakeVisible(dcwDecayKnob);
    dcwPanel->addAndMakeVisible(dcwSustainKnob);
    dcwPanel->addAndMakeVisible(dcwReleaseKnob);
    envelopeTabs.addTab("DCW", juce::Colours::orange, dcwPanel.release(), true);
    
    auto dcaPanel = std::make_unique<juce::Component>();
    dcaPanel->addAndMakeVisible(dcaEditor);
    dcaPanel->addAndMakeVisible(dcaAttackKnob);
    dcaPanel->addAndMakeVisible(dcaDecayKnob);
    dcaPanel->addAndMakeVisible(dcaSustainKnob);
    dcaPanel->addAndMakeVisible(dcaReleaseKnob);
    envelopeTabs.addTab("DCA", juce::Colours::cyan, dcaPanel.release(), true);
    
    // Effects (2x3 grid)
    addAndMakeVisible(delayTimeKnob);
    addAndMakeVisible(delayFeedbackKnob);
    addAndMakeVisible(delayMixKnob);
    addAndMakeVisible(chorusRateKnob);
    addAndMakeVisible(chorusDepthKnob);
    addAndMakeVisible(chorusMixKnob);
    addAndMakeVisible(reverbSizeKnob);
    addAndMakeVisible(reverbMixKnob);
    
    delayLabel.setText("DELAY", juce::dontSendNotification);
    chorusLabel.setText("CHORUS", juce::dontSendNotification);
    reverbLabel.setText("REVERB", juce::dontSendNotification);
    delayLabel.setJustificationType(juce::Justification::centred);
    chorusLabel.setJustificationType(juce::Justification::centred);
    reverbLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(delayLabel);
    addAndMakeVisible(chorusLabel);
    addAndMakeVisible(reverbLabel);
    
    // ========== BOTTOM: KEYBOARD ==========
    addAndMakeVisible(keyboardComponent);
    keyboardState.addListener(this);
    
    // ========== ATTACHMENTS ==========
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

    // LCD Click to Rename
    lcdDisplay.onClick = [this]() 
    {
        // Get current name from PresetManager?
        auto currentName = audioProcessor.getPresetManager().getCurrentPreset().name;
        
        nameOverlay.startRename(currentName, [this](const juce::String& newName) {
            audioProcessor.getPresetManager().renamePreset(audioProcessor.getCurrentProgram(), newName.toStdString());
            // Refresh LCD immediately
            audioProcessor.changeProgramName(audioProcessor.getCurrentProgram(), newName);
            // Also need to refresh Browser if visible?
            // Sending a change message might be cleaner but direct update works for now.
        });
    };
    
    addAndMakeVisible(nameOverlay);
    nameOverlay.setVisible(false);
    
    setSize(800, 600);
    setResizable(true, true); 
    setResizeLimits(600, 450, 1920, 1080); 
    startTimerHz(60); 
}

CZ101AudioProcessorEditor::~CZ101AudioProcessorEditor()
{
    stopTimer();
    openGLContext.detach();
    keyboardState.removeListener(this);
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    // Overlay covers entire specific area or full window
    nameOverlay.setBounds(getLocalBounds());
}

void CZ101AudioProcessorEditor::paint(juce::Graphics& g)
{
    // Dark background
    g.fillAll(juce::Colour(0xff0a0e14));
    
    // Subtle grid lines
    g.setColour(juce::Colour(0xff1a2a3a).withAlpha(0.3f));
    for (int x = 0; x < getWidth(); x += 50)
        g.drawVerticalLine((float)x, 0.0f, (float)getHeight());
}

void CZ101AudioProcessorEditor::resized() 
{
    auto area = getLocalBounds();
    
    // ========== HEADER (40px) ==========
    auto headerArea = area.removeFromTop(40);
    
    // LEFT: Preset selector - INCREASED WIDTH from 160 to 230 to fit Combo
    auto leftHeader = headerArea.removeFromLeft(230); 
    presetBrowser.setBounds(leftHeader.removeFromLeft(225).reduced(2));
    
    headerArea.removeFromLeft(6);
    
    // CENTER-LEFT: Buttons
    auto buttonArea = headerArea.removeFromLeft(120); // Reduced slightly from 180 to balance
    loadSysExButton.setBounds(buttonArea.removeFromLeft(55).reduced(2));
    // loadSysExButton.setButtonText("LOAD"); 
    
    saveSysExButton.setBounds(buttonArea.removeFromLeft(55).reduced(2));
    // saveSysExButton.setButtonText("SAVE"); 
    
    headerArea.removeFromLeft(6);
    
    // CENTER: LCD Display
    auto centerHeader = headerArea.removeFromLeft(240); // Slightly reduced from 250
    lcdDisplay.setBounds(centerHeader.reduced(2));
    
    headerArea.removeFromLeft(6);
    
    // RIGHT: MIDI + CPU
    auto rightHeader = headerArea;
    midiOutputSelector.setBounds(rightHeader.removeFromRight(90).reduced(2));
    rightHeader.removeFromRight(4);
    midiIndicator.setBounds(rightHeader.removeFromRight(30).reduced(2));
    
    area.removeFromTop(6);
    
    // ========== MAIN: 3 COLUMNS ==========
    int col1Width = area.getWidth() * 0.22f;
    int col2Width = area.getWidth() * 0.32f;
    // int col3Width = area.getWidth() - col1Width - col2Width;
    
    // --- COLUMN 1: OSCILLATORS ---
    auto col1 = area.removeFromLeft(col1Width).reduced(3);
    
    auto oscArea = col1.removeFromTop(100);
    juce::FlexBox oscFlex;
    oscFlex.flexWrap = juce::FlexBox::Wrap::wrap;
    oscFlex.justifyContent = juce::FlexBox::JustifyContent::center;
    
    oscFlex.items.add(juce::FlexItem(osc1WaveSelector).withWidth(70).withHeight(22).withMargin(2));
    oscFlex.items.add(juce::FlexItem(osc1LevelKnob).withWidth(50).withHeight(50).withMargin(2));
    oscFlex.items.add(juce::FlexItem(osc2WaveSelector).withWidth(70).withHeight(22).withMargin(2));
    oscFlex.items.add(juce::FlexItem(osc2LevelKnob).withWidth(50).withHeight(50).withMargin(2));
    oscFlex.items.add(juce::FlexItem(osc2DetuneKnob).withWidth(50).withHeight(50).withMargin(2));
    oscFlex.items.add(juce::FlexItem(hardSyncButton).withWidth(60).withHeight(22).withMargin(2));
    oscFlex.items.add(juce::FlexItem(ringModButton).withWidth(60).withHeight(22).withMargin(2));
    
    oscFlex.performLayout(oscArea);
    
    col1.removeFromTop(3);
    
    auto filterArea = col1.removeFromTop(60);
    juce::FlexBox filterFlex;
    filterFlex.flexWrap = juce::FlexBox::Wrap::wrap;
    filterFlex.justifyContent = juce::FlexBox::JustifyContent::center;
    
    filterFlex.items.add(juce::FlexItem(filterCutoffKnob).withWidth(50).withHeight(50).withMargin(2));
    filterFlex.items.add(juce::FlexItem(filterResonanceKnob).withWidth(50).withHeight(50).withMargin(2));
    
    filterFlex.performLayout(filterArea);
    
    col1.removeFromTop(3);
    
    auto waveArea = col1;
    waveformDisplay.setBounds(waveArea.reduced(2));
    
    // --- COLUMN 2: ENVELOPES ---
    auto col2 = area.removeFromLeft(col2Width).reduced(3);
    
    envelopeTabs.setBounds(col2.reduced(2));
    
    for (int i = 0; i < envelopeTabs.getNumTabs(); i++) {
        if (auto panel = dynamic_cast<juce::Component*>(envelopeTabs.getTabContentComponent(i))) {
            auto tabBounds = panel->getLocalBounds();
            
            if (i == 0) { // PITCH - No ADSR Knobs, full editor
                pitchEditor.setBounds(tabBounds.reduced(5));
            }
            else if (i == 1) { // DCW
                auto editorArea = tabBounds.removeFromTop(100).reduced(5);
                dcwEditor.setBounds(editorArea);
                tabBounds.removeFromTop(3);
                
                auto adsrArea = tabBounds.reduced(5);
                juce::FlexBox adsr;
                adsr.flexWrap = juce::FlexBox::Wrap::wrap;
                adsr.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;
                
                adsr.items.add(juce::FlexItem(dcwAttackKnob).withWidth(35).withHeight(45).withMargin(2));
                adsr.items.add(juce::FlexItem(dcwDecayKnob).withWidth(35).withHeight(45).withMargin(2));
                adsr.items.add(juce::FlexItem(dcwSustainKnob).withWidth(35).withHeight(45).withMargin(2));
                adsr.items.add(juce::FlexItem(dcwReleaseKnob).withWidth(35).withHeight(45).withMargin(2));
                
                adsr.performLayout(adsrArea);
            }
            else if (i == 2) { // DCA
                auto editorArea = tabBounds.removeFromTop(100).reduced(5);
                dcaEditor.setBounds(editorArea);
                tabBounds.removeFromTop(3);
                
                auto adsrArea = tabBounds.reduced(5);
                juce::FlexBox adsr;
                adsr.flexWrap = juce::FlexBox::Wrap::wrap;
                adsr.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;
                
                adsr.items.add(juce::FlexItem(dcaAttackKnob).withWidth(35).withHeight(45).withMargin(2));
                adsr.items.add(juce::FlexItem(dcaDecayKnob).withWidth(35).withHeight(45).withMargin(2));
                adsr.items.add(juce::FlexItem(dcaSustainKnob).withWidth(35).withHeight(45).withMargin(2));
                adsr.items.add(juce::FlexItem(dcaReleaseKnob).withWidth(35).withHeight(45).withMargin(2));
                
                adsr.performLayout(adsrArea);
            }
        }
    }
    
    // --- COLUMN 3: EFFECTS + LFO ---
    auto col3 = area.reduced(3);
    
    auto effectsArea = col3.removeFromTop(col3.getHeight() * 0.78f);
    col3.removeFromTop(6);
    auto lfoArea = col3;
    
    // EFFECTS GRID (2x3)
    int fxColWidth = effectsArea.getWidth() / 2;
    int fxRowHeight = effectsArea.getHeight() / 3;
    
    auto row1 = effectsArea.removeFromTop(fxRowHeight);
    
    // DELAY
    auto delayCol = row1.removeFromLeft(fxColWidth);
    delayLabel.setBounds(delayCol.removeFromTop(15).reduced(2));
    
    auto delayGrid = delayCol.reduced(5);
    auto delayRow1 = delayGrid.removeFromTop(delayGrid.getHeight() / 2);
    
    delayTimeKnob.setBounds(delayRow1.removeFromLeft(fxColWidth / 2).reduced(2));
    delayFeedbackKnob.setBounds(delayRow1.reduced(2));
    delayMixKnob.setBounds(delayGrid.reduced(2));
    
    // CHORUS
    auto chorusCol = row1;
    chorusLabel.setBounds(chorusCol.removeFromTop(15).reduced(2));
    
    auto chorusGrid = chorusCol.reduced(5);
    auto chorusRow1 = chorusGrid.removeFromTop(chorusGrid.getHeight() / 2);
    
    chorusRateKnob.setBounds(chorusRow1.removeFromLeft(fxColWidth / 2).reduced(2));
    chorusDepthKnob.setBounds(chorusRow1.reduced(2));
    chorusMixKnob.setBounds(chorusGrid.reduced(2));
    
    // REVERB
    auto row2 = effectsArea;
    reverbLabel.setBounds(row2.removeFromTop(15).reduced(2));
    
    auto reverbGrid = row2.reduced(5);
    reverbSizeKnob.setBounds(reverbGrid.removeFromLeft(reverbGrid.getWidth() / 2).reduced(2));
    reverbMixKnob.setBounds(reverbGrid.reduced(2));
    
    // LFO
    auto lfoRateArea = lfoArea.removeFromLeft(lfoArea.getWidth() / 2);
    lfoRateKnob.setBounds(lfoRateArea.reduced(2));
    
    auto glideArea = lfoArea;
    glideKnob.setBounds(glideArea.reduced(2));
    
    // ========== KEYBOARD (80px) ==========
    auto keyboardArea = getLocalBounds().removeFromBottom(80);
    keyboardComponent.setBounds(keyboardArea.reduced(3));
}

void CZ101AudioProcessorEditor::refreshMidiOutputs()
{
    auto devices = juce::MidiOutput::getAvailableDevices();
    midiOutputSelector.clear();
    midiOutputSelector.addItem("No MIDI", 1);
    
    int i = 2;
    for (const auto& dev : devices)
        midiOutputSelector.addItem(dev.name, i++);
    
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
    
    juce::String cpuStr = "CPU: " + juce::String(cpu, 1) + "%";
    while (cpuStr.length() < 12) cpuStr += " "; // Padding
    
    // User requested Name on top, Stats on bottom
    lcdDisplay.setText(juce::String(preset), cpuStr);
    // Pass real sample rate
    lcdDisplay.setSampleRate(audioProcessor.getSampleRate());
    lcdDisplay.setLastNote(audioProcessor.getVoiceManager().getCurrentNote()); // Assuming we expose this in VM or similar
    
    // Transfer Audio Data for Oscilloscope
    auto& fifo = audioProcessor.getVisFifo();
    auto& buffer = audioProcessor.getVisBuffer();
    
    if (fifo.getNumReady() > 0)
    {
        int start1, size1, start2, size2;
        fifo.prepareToRead(buffer.getNumSamples(), start1, size1, start2, size2);
        
        // Construct temp buffer to push to display
        // Actually WaveformDisplay might want a vector or just samples?
        // It accepts AudioBuffer. Let's create a temp wrapper or just copy.
        // For simplicity, let's just push the block we read.
        
        if (size1 > 0)
        {
            // Just wrap the raw pointers in a temporary buffer to avoid copying if possible, 
            // BUT WaveformDisplay inputs const AudioBuffer&.
            // We can't easily wrap ring buffer parts.
            // Let's copy to a temp buffer.
            juce::AudioBuffer<float> temp(1, size1 + size2);
            
            // Part 1
            if (size1 > 0) {
                temp.copyFrom(0, 0, buffer.getReadPointer(0, start1), size1);
            }
            // Part 2
            if (size2 > 0) {
               temp.copyFrom(0, size1, buffer.getReadPointer(0, start2), size2);
            }
            
            waveformDisplay.pushBuffer(temp);
            fifo.finishedRead(size1 + size2);
        }
    }
    
    waveformDisplay.repaint();
    pitchEditor.updateData();
    dcwEditor.updateData();
    dcaEditor.updateData();
}

bool CZ101AudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto file : files)
        if (file.endsWithIgnoreCase(".syx"))
            return true;
    return false;
}

void CZ101AudioProcessorEditor::filesDropped(const juce::StringArray& files, int x, int y)
{
    juce::ignoreUnused(x, y);
    
    for (auto path : files) {
        if (path.endsWithIgnoreCase(".syx")) {
            juce::File file(path);
            juce::MemoryBlock sysexData;
            
            // Security check: Limit SysEx import size to 64KB
            if (file.getSize() > 65536)
            {
                juce::NativeMessageBox::showAsync(juce::MessageBoxOptions()
                    .withIconType(juce::MessageBoxIconType::WarningIcon)
                    .withTitle("File too large")
                    .withMessage("SysEx files must be under 64KB. This file will not be loaded.")
                    .withButton("OK"),
                    nullptr);
                continue;
            }

            if (file.loadFileAsData(sysexData)) {
                audioProcessor.getSysExManager().handleSysEx(sysexData.getData(), (int)sysexData.getSize(), file.getFileNameWithoutExtension() + " (UNSAVED)");
                timerCallback();
            }
        }
    }
}

void CZ101AudioProcessorEditor::loadSysExFile()
{
    fileChooser = std::make_unique<juce::FileChooser>("Load SysEx Patch",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.syx");

    auto browserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(browserFlags, [this](const juce::FileChooser& fc) {
        auto file = fc.getResult();
        if (file.existsAsFile()) 
        {
            // Security check: Limit SysEx import size to 64KB
            if (file.getSize() > 65536)
            {
                 juce::NativeMessageBox::showAsync(juce::MessageBoxOptions()
                    .withIconType(juce::MessageBoxIconType::WarningIcon)
                    .withTitle("File too large")
                    .withMessage("SysEx files must be under 64KB.")
                    .withButton("OK"),
                    nullptr);
                 return;
            }

            juce::MemoryBlock sysexData;
            if (file.loadFileAsData(sysexData)) 
            {
                // Ensure audioProcessor and SysExManager are accessible and handle the call
                audioProcessor.getSysExManager().handleSysEx(sysexData.getData(), (int)sysexData.getSize(), file.getFileNameWithoutExtension() + " (UNSAVED)");
                timerCallback();
            }
        }
    });
}

void CZ101AudioProcessorEditor::saveSysExFile()
{
    // Patch 1: Deep Persistence (User Fix)
    // 1. Capture current state from UI to preset
    audioProcessor.getPresetManager().copyStateFromProcessor();
    
    // 2. Get current preset index
    int idx = presetBrowser.getSelectedItemIndex();
    if (idx < 0) idx = 0;
    
    auto currentName = audioProcessor.getPresetManager().getCurrentPreset().name;
    
    // 3. Save to memory slot
    audioProcessor.getPresetManager().savePreset(idx, currentName);
    
    // 4. CRITICAL: Persist to Disk
    juce::File presetsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                            .getChildFile("CZ101Emulator");
    
    if (!presetsDir.exists()) {
        presetsDir.createDirectory();
    }
    
    juce::File bankFile = presetsDir.getChildFile("userbank.json");
    
    audioProcessor.getPresetManager().saveBank(bankFile);
    
    // Assume success if no exception/crash
    lcdDisplay.setPresetName("SAVED OK");
    // juce::Logger::writeToLog("Preset saved to: " + bankFile.getFullPathName());
    
    presetBrowser.updatePresetList();
}
