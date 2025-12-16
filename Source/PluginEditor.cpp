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
    lcdDisplay.setText("CZ-101 EMULATOR", "PRESET: INIT");
    
    addAndMakeVisible(presetBrowser);
    presetBrowser.setPresetManager(&audioProcessor.getPresetManager());
    presetBrowser.onPresetSelected = [this](int index) {
        juce::ignoreUnused(index);
        auto name = audioProcessor.getPresetManager().getCurrentPreset().name;
        lcdDisplay.setText("CZ-101 EMULATOR", "PRESET: " + juce::String(name));
    };

    presetBrowser.onSaveRequested = [this]()
    {
        auto currentName = audioProcessor.getPresetManager().getCurrentPreset().name;
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
            int idx = presetBrowser.getSelectedItemIndex(); // 1-based usually in ComboBox
            int presetIndex = idx - 1; 

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
                presetBrowser.setSelectedItemIndex(idx); // restore selection
                lcdDisplay.setText("CZ-101   SAVED", "PRESET: " + newName);
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
    auto* pitchPanel = new juce::Component();
    pitchPanel->addAndMakeVisible(pitchEditor);
    envelopeTabs.addTab("PITCH", juce::Colours::magenta, pitchPanel, true);
    
    auto* dcwPanel = new juce::Component();
    dcwPanel->addAndMakeVisible(dcwEditor);
    dcwPanel->addAndMakeVisible(dcwAttackKnob);
    dcwPanel->addAndMakeVisible(dcwDecayKnob);
    dcwPanel->addAndMakeVisible(dcwSustainKnob);
    dcwPanel->addAndMakeVisible(dcwReleaseKnob);
    envelopeTabs.addTab("DCW", juce::Colours::orange, dcwPanel, true);
    
    auto* dcaPanel = new juce::Component();
    dcaPanel->addAndMakeVisible(dcaEditor);
    dcaPanel->addAndMakeVisible(dcaAttackKnob);
    dcaPanel->addAndMakeVisible(dcaDecayKnob);
    dcaPanel->addAndMakeVisible(dcaSustainKnob);
    dcaPanel->addAndMakeVisible(dcaReleaseKnob);
    envelopeTabs.addTab("DCA", juce::Colours::cyan, dcaPanel, true);
    
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
        g.drawVerticalLine(x, 0, getHeight());
}

void CZ101AudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    
    // ========== HEADER (45px) ==========
    // ========== HEADER (60px - Bigger) ==========
    auto headerArea = area.removeFromTop(60);
    
    // LCD Centered and Wider
    // Left side: MIDI Controls
    auto leftHeader = headerArea.removeFromLeft(200);
    midiIndicator.setBounds(leftHeader.removeFromLeft(30).reduced(5));
    midiOutputSelector.setBounds(leftHeader.reduced(5));
    
    // Right side: Browser & SysEx Buttons
    auto rightHeader = headerArea.removeFromRight(250);
    saveSysExButton.setBounds(rightHeader.removeFromRight(50).reduced(5));
    loadSysExButton.setBounds(rightHeader.removeFromRight(70).reduced(5));
    presetBrowser.setBounds(rightHeader.reduced(5));
    
    // Center: LCD
    lcdDisplay.setBounds(headerArea.reduced(5)); // Takes remaining center space
    
    area.removeFromTop(3);
    
    // ========== MAIN CONTENT AREA (2 COLUMNS) ==========
    int panelWidth = area.getWidth() / 2;
    
    // LEFT PANEL
    auto leftArea = area.removeFromLeft(panelWidth);
    leftArea.removeFromTop(3);
    
    // --- Left: Oscillators ---
    auto oscArea = leftArea.removeFromTop(80);
    {
        juce::FlexBox oscFlex;
        oscFlex.flexWrap = juce::FlexBox::Wrap::wrap;
        oscFlex.justifyContent = juce::FlexBox::JustifyContent::center;
        oscFlex.alignContent = juce::FlexBox::AlignContent::center;
        
        oscFlex.items.add(juce::FlexItem(osc1WaveSelector).withWidth(70).withHeight(25).withMargin({2, 2, 2, 2}));
        oscFlex.items.add(juce::FlexItem(osc1LevelKnob).withWidth(50).withHeight(50));
        oscFlex.items.add(juce::FlexItem(osc2WaveSelector).withWidth(70).withHeight(25).withMargin({2, 2, 2, 2}));
        oscFlex.items.add(juce::FlexItem(osc2LevelKnob).withWidth(50).withHeight(50));
        oscFlex.items.add(juce::FlexItem(osc2DetuneKnob).withWidth(50).withHeight(50));
        oscFlex.items.add(juce::FlexItem(hardSyncButton).withWidth(55).withHeight(25).withMargin({5, 2, 2, 2}));
        oscFlex.items.add(juce::FlexItem(ringModButton).withWidth(55).withHeight(25).withMargin({5, 2, 2, 2}));
        oscFlex.items.add(juce::FlexItem(glideKnob).withWidth(45).withHeight(50));
        
        oscFlex.performLayout(oscArea);
    }
    
    // --- Left: Waveform Display ---
    auto waveArea = leftArea.removeFromTop(50);
    waveformDisplay.setBounds(waveArea.reduced(3));
    
    leftArea.removeFromTop(3);
    
    // --- Left: Filter ---
    auto filterArea = leftArea.removeFromTop(70);
    {
        juce::FlexBox filterFlex;
        filterFlex.flexWrap = juce::FlexBox::Wrap::wrap;
        filterFlex.justifyContent = juce::FlexBox::JustifyContent::center;
        
        filterFlex.items.add(juce::FlexItem(filterCutoffKnob).withWidth(50).withHeight(60));
        filterFlex.items.add(juce::FlexItem(filterResonanceKnob).withWidth(50).withHeight(60));
        
        filterFlex.performLayout(filterArea);
    }
    
    leftArea.removeFromTop(3);
    
    // --- Left: LFO ---
    auto lfoArea = leftArea.removeFromTop(50);
    lfoRateKnob.setBounds(lfoArea.removeFromLeft(70).reduced(3));
    
    // RIGHT PANEL
    area.removeFromLeft(3);
    auto rightArea = area;
    rightArea.removeFromTop(3);
    
    // --- Right: Envelope Tabs ---
    auto envArea = rightArea.removeFromTop(200);
    envelopeTabs.setBounds(envArea.reduced(3));
    
    // Resize envelope editor content inside tabs
    for (int i = 0; i < envelopeTabs.getNumTabs(); ++i) {
        if (auto* panel = dynamic_cast<juce::Component*>(envelopeTabs.getTabContentComponent(i))) {
            auto tabBounds = panel->getLocalBounds();
            
            if (i == 0) { // PITCH - just editor
                pitchEditor.setBounds(tabBounds.reduced(5));
            }
            else if (i == 1) { // DCW - editor + knobs
                auto editorArea = tabBounds.removeFromTop(100).reduced(5);
                dcwEditor.setBounds(editorArea);
                juce::FlexBox adsr;
                adsr.flexWrap = juce::FlexBox::Wrap::wrap;
                adsr.justifyContent = juce::FlexBox::JustifyContent::center;
                adsr.items.add(juce::FlexItem(dcwAttackKnob).withWidth(35).withHeight(45));
                adsr.items.add(juce::FlexItem(dcwDecayKnob).withWidth(35).withHeight(45));
                adsr.items.add(juce::FlexItem(dcwSustainKnob).withWidth(35).withHeight(45));
                adsr.items.add(juce::FlexItem(dcwReleaseKnob).withWidth(35).withHeight(45));
                adsr.performLayout(tabBounds.reduced(5));
            }
            else if (i == 2) { // DCA - editor + knobs
                auto editorArea = tabBounds.removeFromTop(100).reduced(5);
                dcaEditor.setBounds(editorArea);
                juce::FlexBox adsr;
                adsr.flexWrap = juce::FlexBox::Wrap::wrap;
                adsr.justifyContent = juce::FlexBox::JustifyContent::center;
                adsr.items.add(juce::FlexItem(dcaAttackKnob).withWidth(35).withHeight(45));
                adsr.items.add(juce::FlexItem(dcaDecayKnob).withWidth(35).withHeight(45));
                adsr.items.add(juce::FlexItem(dcaSustainKnob).withWidth(35).withHeight(45));
                adsr.items.add(juce::FlexItem(dcaReleaseKnob).withWidth(35).withHeight(45));
                adsr.performLayout(tabBounds.reduced(5));
            }
        }
    }
    
    rightArea.removeFromTop(3);
    
    // --- Right: Effects Grid (2x3) ---
    auto effectsArea = rightArea;
    int fx_col_width = effectsArea.getWidth() / 3;
    int fx_row_height = effectsArea.getHeight() / 3;
    
    // Row 1: DELAY
    auto delayColArea = effectsArea.removeFromLeft(fx_col_width);
    delayLabel.setBounds(delayColArea.removeFromTop(15).reduced(2));
    auto delayGridArea = delayColArea.removeFromLeft(fx_col_width / 3);
    delayTimeKnob.setBounds(delayGridArea.reduced(2));
    delayGridArea = delayColArea.removeFromLeft(fx_col_width / 3);
    delayFeedbackKnob.setBounds(delayGridArea.reduced(2));
    delayGridArea = delayColArea;
    delayMixKnob.setBounds(delayGridArea.reduced(2));
    
    // Row 2: CHORUS
    auto chorusColArea = effectsArea.removeFromLeft(fx_col_width);
    chorusLabel.setBounds(chorusColArea.removeFromTop(15).reduced(2));
    auto chorusGridArea = chorusColArea.removeFromLeft(fx_col_width / 3);
    chorusRateKnob.setBounds(chorusGridArea.reduced(2));
    chorusGridArea = chorusColArea.removeFromLeft(fx_col_width / 3);
    chorusDepthKnob.setBounds(chorusGridArea.reduced(2));
    chorusGridArea = chorusColArea;
    chorusMixKnob.setBounds(chorusGridArea.reduced(2));
    
    // Row 3: REVERB
    auto reverbColArea = effectsArea;
    reverbLabel.setBounds(reverbColArea.removeFromTop(15).reduced(2));
    auto reverbGridArea = reverbColArea.removeFromLeft(fx_col_width / 3);
    reverbSizeKnob.setBounds(reverbGridArea.reduced(2));
    reverbGridArea = reverbColArea.removeFromLeft(fx_col_width / 3);
    reverbMixKnob.setBounds(reverbGridArea.reduced(2));
    
    // ========== BOTTOM: KEYBOARD (80px) ==========
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
    
    juce::String cpuStr = juce::String(cpu, 1) + "%";
    while (cpuStr.length() < 5) cpuStr = " " + cpuStr;
    
    lcdDisplay.setText("CPU: " + cpuStr, "PRESET: " + juce::String(preset));
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
            
            if (file.loadFileAsData(sysexData)) {
                audioProcessor.getSysExManager().handleSysEx(sysexData.getData(), (int)sysexData.getSize(), file.getFileNameWithoutExtension());
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

    auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc) {
        auto file = fc.getResult();
        if (file.existsAsFile()) 
        {
            juce::MemoryBlock sysexData;
            if (file.loadFileAsData(sysexData)) 
            {
                // Ensure audioProcessor and SysExManager are accessible and handle the call
                audioProcessor.getSysExManager().handleSysEx(sysexData.getData(), (int)sysexData.getSize(), file.getFileNameWithoutExtension());
                timerCallback();
            }
        }
    });
}

void CZ101AudioProcessorEditor::saveSysExFile()
{
    // Save current state to the current preset slot
    auto currentName = audioProcessor.getPresetManager().getCurrentPreset().name;
    
    // For now, just save immediately. 
    // Ideally we'd show a Rename dialog.
    // Let's rely on the current name (which might have come from SysEx load).
    
    audioProcessor.saveCurrentPreset(currentName);
    
    // Feedback
    lcdDisplay.setText("CZ-101   SAVED", "PRESET: " + juce::String(currentName));
    
    // Also trigger a blip?
}
