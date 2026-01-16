#include "PluginEditor.h"
#include "UI/DesignTokens.h"
#include "UI/SkinManager.h"
#include "../State/PresetRandomizer.h"
#include "MIDI/SysExManager.h"
#include "UI/Sections/GeneralSection.h"

// --- Synthesis Dashboard Component ---
struct SynthesisDashboard : public juce::Component
{
    CZ101::UI::OscillatorSection& oscSection;
    CZ101::UI::FilterLfoSection& filterLfoSection;
    CZ101::UI::WaveformDisplay& waveformDisplay;

    SynthesisDashboard(CZ101::UI::OscillatorSection& osc, 
                       CZ101::UI::FilterLfoSection& fl, 
                       CZ101::UI::WaveformDisplay& wav)
        : oscSection(osc), filterLfoSection(fl), waveformDisplay(wav)
    {
        addAndMakeVisible(waveformDisplay);
        addAndMakeVisible(oscSection);
        addAndMakeVisible(filterLfoSection);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(15);
        
        // Top 35%: Waveform (The "Heartbeat")
        waveformDisplay.setBounds(area.removeFromTop(area.getHeight() * 0.35f));
        
        area.removeFromTop(10); // Gap
        
        // Bottom: Osc & LFO side-by-side
        auto controlArea = area;
        int half = controlArea.getWidth() / 2;
        
        oscSection.setBounds(controlArea.removeFromLeft(half).reduced(5, 0));
        filterLfoSection.setBounds(controlArea.reduced(5, 0));
    }
    
    void paint(juce::Graphics& g) override
    {
        // Draw sub-section backgrounds
        g.setColour(CZ101::UI::DesignTokens::Colors::sectionBackground);
        g.fillRoundedRectangle(waveformDisplay.getBounds().toFloat(), 4.0f);
        g.fillRoundedRectangle(oscSection.getBounds().toFloat(), 4.0f);
        g.fillRoundedRectangle(filterLfoSection.getBounds().toFloat(), 4.0f);
    }
};

CZ101AudioProcessorEditor::CZ101AudioProcessorEditor(CZ101AudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      oscSection(p), effectsSection(p), filterLfoSection(p), modMatrixSection(p), generalSection(p), arpPanel(p),
      pitchEditorL1(p, CZ101::UI::EnvelopeEditor::EnvelopeType::PITCH),
      pitchEditorL2(p, CZ101::UI::EnvelopeEditor::EnvelopeType::PITCH),
      dcwEditorL1(p, CZ101::UI::EnvelopeEditor::EnvelopeType::DCW),
      dcwEditorL2(p, CZ101::UI::EnvelopeEditor::EnvelopeType::DCW),
      dcaEditorL1(p, CZ101::UI::EnvelopeEditor::EnvelopeType::DCA),
      dcaEditorL2(p, CZ101::UI::EnvelopeEditor::EnvelopeType::DCA),
      keyboardComponent(keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel(&customLookAndFeel);
    // DBG("CZ101 Editor: Attaching OpenGL");
    // openGLContext.attachTo(*this);

    // 1. Core Modules
    menuBar = std::make_unique<juce::MenuBarComponent>(this);
    addAndMakeVisible(menuBar.get());
    juce::Logger::writeToLog("CZ101 Editor: Connecting SkinManager");
    CZ101::UI::SkinManager::getInstance().addChangeListener(this);

    // 2. Containers & Tabs
    pitchEditorL1.setLine(1); pitchEditorL2.setLine(2);
    dcwEditorL1.setLine(1);   dcwEditorL2.setLine(2);
    dcaEditorL1.setLine(1);   dcaEditorL2.setLine(2);

    pitchContainer = std::make_unique<DualEnvelopeContainer>(pitchEditorL1, pitchEditorL2);
    dcwContainer   = std::make_unique<DualEnvelopeContainer>(dcwEditorL1, dcwEditorL2);
    dcaContainer   = std::make_unique<DualEnvelopeContainer>(dcaEditorL1, dcaEditorL2);

    // 3. Tab Architecture
    juce::Logger::writeToLog("CZ101 Editor: Setting up Tabs");
    addAndMakeVisible(mainTabs);
    
    mainTabs.addTab("SYNTHESIS", juce::Colours::transparentBlack, new SynthesisDashboard(oscSection, filterLfoSection, waveformDisplay), true);
    mainTabs.addTab("PITCH ENV", juce::Colours::transparentBlack, pitchContainer.get(), false);
    mainTabs.addTab("DCW ENV", juce::Colours::transparentBlack, dcwContainer.get(), false);
    mainTabs.addTab("DCA ENV", juce::Colours::transparentBlack, dcaContainer.get(), false);
    mainTabs.addTab("MOD MATRIX", juce::Colours::transparentBlack, &modMatrixSection, false);
    mainTabs.addTab("ARP", juce::Colours::transparentBlack, &arpPanel, false);
    mainTabs.addTab("EFFECTS", juce::Colours::transparentBlack, &effectsSection, false);
    mainTabs.addTab("GENERAL", juce::Colours::transparentBlack, &generalSection, false);

    // Sync UIManager with Tabs
    mainTabs.getTabbedButtonBar().addChangeListener(this);

    // React to UIManager changes (e.g. for LCD feedback)
    uiManager.addChangeListener(this);

    // 4. Header Controls
    juce::Logger::writeToLog("CZ101 Editor: Adding Header Controls");
    addAndMakeVisible(lcdDisplay);
    addAndMakeVisible(presetBrowser);
    
    juce::Logger::writeToLog("CZ101 Editor: Setting Preset Manager");
    presetBrowser.setPresetManager(&audioProcessor.getPresetManager());
    
    addAndMakeVisible(randomButton);
    randomButton.setColour(juce::TextButton::buttonOnColourId, CZ101::UI::DesignTokens::Colors::czRed);
    randomButton.onClick = [this]() { randomizePatch(); };

    addAndMakeVisible(panicButton);
    panicButton.setColour(juce::TextButton::buttonOnColourId, CZ101::UI::DesignTokens::Colors::czRed);
    panicButton.onClick = [this]() { audioProcessor.getVoiceManager().allNotesOff(); };

    // Header Navigation
    addAndMakeVisible(cursorLeft); addAndMakeVisible(cursorRight);
    addAndMakeVisible(cursorUp); addAndMakeVisible(cursorDown);
    
    // Enable Auto-Repeat for Acceleration
    cursorUp.setRepeatSpeed(400, 60);
    cursorDown.setRepeatSpeed(400, 60);
    cursorLeft.setRepeatSpeed(400, 100);
    cursorRight.setRepeatSpeed(400, 100);
    
    juce::Logger::writeToLog("CZ101 Editor: Getting LCD State Manager");
    auto& lcdManagerLocal = audioProcessor.getLCDStateManager();
    juce::Logger::writeToLog("CZ101 Editor: Setting LCD State Manager");
    lcdDisplay.setStateManager(&lcdManagerLocal);
    
    cursorLeft.onClick   = [this] { 
        if (isProcessingLCDCommand.exchange(true)) return;
        juce::MessageManager::callAsync([this] { 
            audioProcessor.getLCDStateManager().onCursorLeft(); 
            isProcessingLCDCommand = false;
        }); 
    };
    cursorRight.onClick  = [this] { 
        if (isProcessingLCDCommand.exchange(true)) return;
        juce::MessageManager::callAsync([this] { 
            audioProcessor.getLCDStateManager().onCursorRight(); 
            isProcessingLCDCommand = false;
        }); 
    };
    cursorUp.onClick     = [this] { 
        if (isProcessingLCDCommand.exchange(true)) return;
        juce::MessageManager::callAsync([this] { 
            audioProcessor.getLCDStateManager().onValueUp(); 
            isProcessingLCDCommand = false;
        }); 
    };
    cursorDown.onClick   = [this] { 
        if (isProcessingLCDCommand.exchange(true)) return;
        juce::MessageManager::callAsync([this] { 
            audioProcessor.getLCDStateManager().onValueDown(); 
            isProcessingLCDCommand = false;
        }); 
    };
    
    presetBrowser.onPresetSelected = [this](int) {
        auto& lcd = audioProcessor.getLCDStateManager();
        lcd.setParameterFeedbackSuppressed(true);
        
        pitchEditorL1.updateData(); pitchEditorL2.updateData();
        dcwEditorL1.updateData();   dcwEditorL2.updateData();
        dcaEditorL1.updateData();   dcaEditorL2.updateData();
        
        lcd.setParameterFeedbackSuppressed(false);
        lcd.showProgramMode();
    };

    // 5. Finalize
    juce::Logger::writeToLog("CZ101 Editor: Finalizing Components");
    addAndMakeVisible(keyboardComponent);
    keyboardState.addListener(this);
    waveformDisplay.setProcessor(&audioProcessor);

    addChildComponent(nameOverlay);
    addChildComponent(aboutDialog);
    addChildComponent(bankManagerOverlay);
    
    bankManagerOverlay.pm = &audioProcessor.getPresetManager();
    bankManagerOverlay.onUpdate = [this]() {
        presetBrowser.updatePresetList();
    };

    // Modern Mode Only Visibility
    bool isAuthentic = audioProcessor.getParameters().getAuthenticMode() ? audioProcessor.getParameters().getAuthenticMode()->get() : true;
    randomButton.setVisible(!isAuthentic);
    
    audioProcessor.getParameters().getAPVTS().addParameterListener("AUTHENTIC_MODE", this);

    setSize(800, 500); // Strict 8:5 ratio
    setResizeLimits(800, 500, 1600, 1000);
    juce::Logger::writeToLog("CZ101 Editor: Starting Timer");
    startTimerHz(60);
    juce::Logger::writeToLog("CZ101 Editor: Constructor End");
}

CZ101AudioProcessorEditor::~CZ101AudioProcessorEditor()
{
    stopTimer();
    tooltipWindow.setVisible(false); // Audit Fix: Hide tooltip on destruction to prevent race
    audioProcessor.getParameters().getAPVTS().removeParameterListener("AUTHENTIC_MODE", this);
    bankManagerOverlay.listBox.setModel(nullptr); // Audit Fix: Prevent use-after-free
    CZ101::UI::SkinManager::getInstance().removeChangeListener(this);
    keyboardState.removeListener(this);
    openGLContext.detach();
    setLookAndFeel(nullptr);
}

void CZ101AudioProcessorEditor::paint(juce::Graphics& g) 
{ 
    auto& palette = CZ101::UI::SkinManager::getInstance().getCurrentPalette();
    g.fillAll(palette.background); 
    
    float scale = getScaleFactor();
    g.setColour(juce::Colours::black.withAlpha(0.2f));
    g.drawHorizontalLine((int)(60 * scale + 24), 0, getWidth());
}

void CZ101AudioProcessorEditor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "AUTHENTIC_MODE")
    {
        bool isAuthentic = (newValue > 0.5f);
        juce::MessageManager::callAsync([this, isAuthentic]() {
            randomButton.setVisible(true); // Always visible
            panicButton.setVisible(true); // Always visible
            resized();
        });
    }
}

void CZ101AudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    float scale = getWidth() / 800.0f;
    CZ101::UI::DesignTokens::layoutScale = scale;
    
    // 1. Menu Bar (Fixed 24)
    if (menuBar != nullptr)
        menuBar->setBounds(area.removeFromTop(24));

    // 2. Header (Fixed 60 scaled)
    auto headerArea = area.removeFromTop((int)(60 * scale));
    
    // 3. Keyboard (Bottom 80)
    auto keyboardArea = area.removeFromBottom((int)(80 * scale));
    keyboardComponent.setKeyWidth((float)keyboardArea.getWidth() / 50.0f); // Show ~50 keys
    keyboardComponent.setBounds(keyboardArea);
    
    // 4. Tabs (Middle)
    mainTabs.setBounds(area.reduced(10, 5));
    
    // --- Header Layout ---
    auto hLeft  = headerArea.removeFromLeft(getWidth() * 0.30f).reduced(5);
    auto hRight = headerArea.removeFromRight(getWidth() * 0.20f).reduced(5);
    auto hMid   = headerArea.reduced(5);
    
    presetBrowser.setBounds(hLeft);
    
    presetBrowser.setBounds(hLeft);
    
    // RANDOM + PANIC Button Right
    auto rightBox = hRight.reduced(5);
    int buttonW = rightBox.getWidth() / 2 - 5;
    randomButton.setBounds(rightBox.removeFromLeft(buttonW));
    panicButton.setBounds(rightBox.removeFromRight(buttonW));
    
    // LCD + Cursors
    
    float arrowSz = 35.0f * scale; 
    float lW = 220.0f * scale;
    float lH = 50.0f * scale;
    
    juce::FlexBox fb;
    fb.justifyContent = juce::FlexBox::JustifyContent::center;
    fb.alignItems = juce::FlexBox::AlignItems::center;
    
    fb.items.add(juce::FlexItem(cursorLeft).withWidth(arrowSz).withHeight(arrowSz).withMargin(2));
    fb.items.add(juce::FlexItem(lcdDisplay).withWidth(lW).withHeight(lH).withMargin(juce::FlexItem::Margin(0, 10, 0, 10)));
    
    juce::FlexBox stack;
    stack.flexDirection = juce::FlexBox::Direction::column;
    stack.items.add(juce::FlexItem(cursorUp).withWidth(arrowSz).withHeight(arrowSz * 0.5f).withMargin(1));
    stack.items.add(juce::FlexItem(cursorDown).withWidth(arrowSz).withHeight(arrowSz * 0.5f).withMargin(1));
    
    fb.items.add(juce::FlexItem(stack).withWidth(arrowSz).withHeight(arrowSz + 2).withMargin(2));
    
    fb.items.add(juce::FlexItem(cursorRight).withWidth(arrowSz).withHeight(arrowSz).withMargin(2));
    
    fb.performLayout(hMid);
    
    nameOverlay.setBounds(getLocalBounds()); 
    aboutDialog.setBounds(getLocalBounds().getCentreX() - 200, getLocalBounds().getCentreY() - 150, 400, 300);
    bankManagerOverlay.setBounds(getLocalBounds());
}

void CZ101AudioProcessorEditor::timerCallback()
{
    // Audit Fix 1.3: Triple Buffering Consumer
    auto& tb = audioProcessor.getVisTripleBuffer();
    
    if (tb.hasNewData.exchange(false, std::memory_order_acquire))
    {
        int currentFront = tb.frontIndex.load(std::memory_order_relaxed);
        // Atomic Swap: Give our old Front to Mid, take Mid as new Front
        int newFront = tb.midIndex.exchange(currentFront, std::memory_order_acq_rel);
        tb.frontIndex.store(newFront, std::memory_order_relaxed);
        
        // Pass data to WaveformDisplay
        auto& src = tb.buffers[newFront];
        float* channels[] = { src.data() };
        juce::AudioBuffer<float> temp(channels, 1, (int)src.size());
        waveformDisplay.pushBuffer(temp);
    }

    waveformDisplay.repaint();
    if (audioProcessor.getMidiProcessor().hasRecentActivity()) audioProcessor.getMidiProcessor().clearActivityFlag();
}

void CZ101AudioProcessorEditor::handleNoteOn(juce::MidiKeyboardState*, int ch, int n, float v)
{ audioProcessor.getMidiProcessor().processMessage(juce::MidiMessage::noteOn(ch, n, v)); }

void CZ101AudioProcessorEditor::handleNoteOff(juce::MidiKeyboardState*, int ch, int n, float v)
{ audioProcessor.getMidiProcessor().processMessage(juce::MidiMessage::noteOff(ch, n, v)); }

void CZ101AudioProcessorEditor::loadSysExFile()
{
    fileChooser = std::make_unique<juce::FileChooser>("Load CZ-101 SysEx", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.syx");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this](const juce::FileChooser& fc) {
        auto file = fc.getResult();
        if (file.existsAsFile()) {
            juce::MemoryBlock data;
            if (file.loadFileAsData(data)) {
                auto& lcd = audioProcessor.getLCDStateManager();
                lcd.setParameterFeedbackSuppressed(true);

                audioProcessor.getSysExManager().handleSysEx(data.getData(), (int)data.getSize(), file.getFileNameWithoutExtension());
                presetBrowser.updatePresetList();
                pitchEditorL1.updateData(); pitchEditorL2.updateData();
                dcwEditorL1.updateData();   dcwEditorL2.updateData();
                dcaEditorL1.updateData();   dcaEditorL2.updateData();

                lcd.setParameterFeedbackSuppressed(false);
                lcd.showProgramMode();
            }
        }
    });
}

void CZ101AudioProcessorEditor::saveSysExFile()
{
    fileChooser = std::make_unique<juce::FileChooser>("Save CZ-101 SysEx", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.syx");
    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting, [this](const juce::FileChooser& fc) {
        auto file = fc.getResult();
        if (file != juce::File()) {
            audioProcessor.getPresetManager().copyStateFromProcessor();
            auto p = audioProcessor.getPresetManager().getCurrentPreset();
            auto data = audioProcessor.getSysExManager().createPatchDump(p);
            file.replaceWithData(data.getData(), data.getSize());
        }
    });
}

void CZ101AudioProcessorEditor::randomizePatch()
{
    audioProcessor.getUndoManager().beginNewTransaction("Randomize Patch");
    
    // Use Advanced Preset Logic (Phase 5 Bonus)
    auto newPreset = CZ101::State::PresetRandomizer::generateRandomPreset("Randomized");
    
    auto& lcd = audioProcessor.getLCDStateManager();
    lcd.setParameterFeedbackSuppressed(true);

    // Load it via PresetManager which handles distribution to VoiceManager (Envelopes) and APVTS (Params)
    audioProcessor.getPresetManager().loadPresetFromStruct(newPreset);
    
    // 4. Force UI Refresh
    pitchEditorL1.updateData(); pitchEditorL2.updateData();
    dcwEditorL1.updateData();   dcwEditorL2.updateData();
    dcaEditorL1.updateData();   dcaEditorL2.updateData();

    lcd.setParameterFeedbackSuppressed(false);
    lcd.showProgramMode();
    waveformDisplay.repaint();
    lcdDisplay.repaint();
    repaint();
}

bool CZ101AudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& f) { for (auto& s : f) if (s.endsWithIgnoreCase(".syx") || s.endsWithIgnoreCase(".json")) return true; return false; }
void CZ101AudioProcessorEditor::filesDropped(const juce::StringArray& f, int, int) {
    if (f[0].endsWithIgnoreCase(".syx")) {
        juce::MemoryBlock data; juce::File(f[0]).loadFileAsData(data);
        audioProcessor.getSysExManager().handleSysEx(data.getData(), (int)data.getSize(), juce::File(f[0]).getFileNameWithoutExtension());
    } else if (f[0].endsWithIgnoreCase(".json")) {
        audioProcessor.getPresetManager().loadBank(juce::File(f[0]));
    }
    presetBrowser.updatePresetList();
}

juce::StringArray CZ101AudioProcessorEditor::getMenuBarNames() { return { "File", "Edit", "Mode", "View", "Help" }; }
juce::PopupMenu CZ101AudioProcessorEditor::getMenuForIndex(int, const juce::String& n) {
    // Audit Fix: Thread Safety
    juce::ScopedLock lock(audioProcessor.getPresetManager().getLock());
    
    juce::PopupMenu m;
    if (n == "File") {
        m.addItem(100, "Load Bank (.json)...");
        m.addItem(101, "Save Bank (.json)...");
        m.addSeparator();
        m.addItem(104, "Load Patch (.json, .syx)...");
        m.addItem(105, "Save Patch As (.json)...");
        m.addItem(106, "Export Original SysEx (.syx)...");
        m.addSeparator();
        m.addItem(107, "Store Patch (Override Slot)");
        m.addItem(108, "Store to New Slot...");
        m.addItem(109, "Rename Current Patch");
        m.addSeparator();
        m.addItem(102, "Init Bank (Factory Reset)");
        m.addItem(103, "Reset Current Patch");
        
        if (juce::JUCEApplication::getInstance() != nullptr)
        {
            m.addSeparator();
            m.addItem(110, "Exit");
        }
    } else if (n == "Edit") {
        m.addItem(200, "Undo");
        m.addItem(201, "Redo");
        m.addSeparator();
        m.addItem(203, "Bank Manager...");
        m.addItem(204, "Randomize Patch", true, false);
        m.addSeparator();
        bool isSystem = audioProcessor.getLCDStateManager().getMode() == CZ101::UI::LCDStateManager::Mode::SYSTEM;
        m.addItem(205, "Settings (System Mode)", true, isSystem);
        
        if (audioProcessor.requestAudioSettings)
        {
             m.addSeparator();
             m.addItem(206, "Audio Settings...");
        }
    } else if (n == "Mode") {
        bool isAuthentic = audioProcessor.getParameters().getAuthenticMode()->get();
        m.addItem(400, "Authentic (Hardware Strict)", true, isAuthentic);
        m.addItem(401, "Modern (Enhanced Features)", true, !isAuthentic);
    } else if (n == "View") {
        m.addItem(300, "Zoom 100%"); m.addItem(301, "Zoom 125%"); m.addItem(302, "Zoom 150%");
        m.addSeparator();
        m.addItem(310, "Dark Theme"); 
        m.addItem(311, "Light Theme");
        m.addItem(312, "Vintage Theme (ABD Z5001)");
        m.addItem(313, "Retro Beige Theme");
        m.addItem(314, "CyberGlow Theme");
        m.addItem(315, "Neon Retro Theme");
        m.addItem(316, "Steampunk Theme");
        m.addItem(317, "Apple Silicon Theme");
        m.addItem(318, "Retro Terminal Theme");
    } else if (n == "Help") {
        m.addItem(900, "Manual / Wiki");
        m.addItem(901, "About...");
    }
    return m;
}

void CZ101AudioProcessorEditor::menuItemSelected(int id, int) {
    switch (id) {
        case 100: presetBrowser.loadBank(); break;
        case 101: presetBrowser.saveBank(); break;
        case 102: presetBrowser.initBank(); break;
        case 103: audioProcessor.initializeSection(InitSection::ALL); break;
        case 104: loadPatchFile(); break;
        case 105: savePatchFile(); break;
        case 106: exportOriginalSysEx(); break;
        case 107: storeCurrentPatch(); break;
        case 108: storeToNewSlot(); break;
        case 109: renameCurrentPatch(); break;
        case 110: if (auto* app = juce::JUCEApplication::getInstance()) app->systemRequestedQuit(); break;
        case 200: audioProcessor.getUndoManager().undo(); break;
        case 201: audioProcessor.getUndoManager().redo(); break;
        case 203: openBankManager(); break;
        case 204: randomizePatch(); break;
        case 205: {
            auto& lsm = audioProcessor.getLCDStateManager();
            if (lsm.getMode() == CZ101::UI::LCDStateManager::Mode::SYSTEM)
                lsm.setMode(CZ101::UI::LCDStateManager::Mode::EDIT);
            else
                lsm.setMode(CZ101::UI::LCDStateManager::Mode::SYSTEM);
            break;
        }
        case 206: if (audioProcessor.requestAudioSettings) audioProcessor.requestAudioSettings(); break;
        case 300: setSize(800, 500); break;
        case 301: setSize(1000, 625); break;
        case 302: setSize(1200, 750); break;
        case 310: CZ101::UI::SkinManager::getInstance().setTheme(CZ101::UI::SkinManager::Theme::Dark); break;
        case 311: CZ101::UI::SkinManager::getInstance().setTheme(CZ101::UI::SkinManager::Theme::Light); break;
        case 312: CZ101::UI::SkinManager::getInstance().setTheme(CZ101::UI::SkinManager::Theme::Vintage); break;
        case 313: CZ101::UI::SkinManager::getInstance().setTheme(CZ101::UI::SkinManager::Theme::RetroBeige); break;
        case 314: CZ101::UI::SkinManager::getInstance().setTheme(CZ101::UI::SkinManager::Theme::CyberGlow); break;
        case 315: CZ101::UI::SkinManager::getInstance().setTheme(CZ101::UI::SkinManager::Theme::NeonRetro); break;
        case 316: CZ101::UI::SkinManager::getInstance().setTheme(CZ101::UI::SkinManager::Theme::Steampunk); break;
        case 317: CZ101::UI::SkinManager::getInstance().setTheme(CZ101::UI::SkinManager::Theme::AppleSilicon); break;
        case 318: CZ101::UI::SkinManager::getInstance().setTheme(CZ101::UI::SkinManager::Theme::RetroTerminal); break;
        case 400: audioProcessor.getParameters().getAuthenticMode()->setValueNotifyingHost(1.0f); break;
        case 401: audioProcessor.getParameters().getAuthenticMode()->setValueNotifyingHost(0.0f); break;
        case 901: aboutDialog.setVisible(true); aboutDialog.toFront(true); break;
    }
}

void CZ101AudioProcessorEditor::loadPatchFile()
{
    fileChooser = std::make_unique<juce::FileChooser>("Load CZ-101 Patch", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.json;*.syx");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this](const juce::FileChooser& fc) {
        auto file = fc.getResult();
        if (file.existsAsFile()) {
            if (file.getFileExtension().equalsIgnoreCase(".syx")) {
                juce::MemoryBlock data; file.loadFileAsData(data);
                audioProcessor.getSysExManager().handleSysEx(data.getData(), (int)data.getSize(), file.getFileNameWithoutExtension());
            } else {
                audioProcessor.getPresetManager().loadPresetFromFile(file);
            }
            presetBrowser.updatePresetList();
            lcdDisplay.repaint();
            repaint();
        }
    });
}

void CZ101AudioProcessorEditor::savePatchFile()
{
    audioProcessor.getPresetManager().copyStateFromProcessor();
    auto currentName = audioProcessor.getPresetManager().getCurrentPreset().name;
    
    fileChooser = std::make_unique<juce::FileChooser>("Save Patch (.json)", 
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(currentName + ".json"), 
        "*.json");
        
    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting, [this](const juce::FileChooser& fc) {
        auto file = fc.getResult();
        if (file != juce::File()) {
            audioProcessor.getPresetManager().savePresetToFile(audioProcessor.getPresetManager().getCurrentPresetIndex(), file);
        }
    });
}

void CZ101AudioProcessorEditor::storeCurrentPatch()
{
    audioProcessor.getPresetManager().copyStateFromProcessor();
    auto name = audioProcessor.getPresetManager().getCurrentPreset().name;
    audioProcessor.getPresetManager().savePreset(audioProcessor.getPresetManager().getCurrentPresetIndex(), name);
    presetBrowser.updatePresetList();
}

void CZ101AudioProcessorEditor::storeToNewSlot()
{
    nameOverlay.startRename(audioProcessor.getPresetManager().getCurrentPreset().name, [this](const juce::String& name) {
        audioProcessor.getPresetManager().copyStateFromProcessor();
        auto p = audioProcessor.getPresetManager().getCurrentPreset();
        p.name = name.toStdString(); // Use the new name
        
        int newIndex = audioProcessor.getPresetManager().addPreset(p);
        
        audioProcessor.getPresetManager().loadPreset(newIndex);
        presetBrowser.updatePresetList();
        lcdDisplay.repaint();
    });
}

void CZ101AudioProcessorEditor::openBankManager()
{
    bankManagerOverlay.setVisible(true);
    bankManagerOverlay.toFront(true);
    bankManagerOverlay.listBox.updateContent();
}

void CZ101AudioProcessorEditor::renameCurrentPatch()
{
    nameOverlay.startRename(audioProcessor.getPresetManager().getCurrentPreset().name, [this](const juce::String& name) {
        audioProcessor.getPresetManager().renamePreset(audioProcessor.getPresetManager().getCurrentPresetIndex(), name.toStdString());
        lcdDisplay.repaint();
        presetBrowser.updatePresetList();
    });
}

void CZ101AudioProcessorEditor::exportOriginalSysEx()
{
    fileChooser = std::make_unique<juce::FileChooser>("Export Original SysEx", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.syx");
    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting, [this](const juce::FileChooser& fc) {
        auto file = fc.getResult();
        if (file != juce::File()) {
            audioProcessor.getPresetManager().copyStateFromProcessor();
            auto p = audioProcessor.getPresetManager().getCurrentPreset();
            auto data = audioProcessor.getSysExManager().createPatchDump(p);
            file.replaceWithData(data.getData(), data.getSize());
        }
    });
}

void CZ101AudioProcessorEditor::changeListenerCallback(juce::ChangeBroadcaster* s) 
{ 
    if (s == &CZ101::UI::SkinManager::getInstance()) 
    { 
        sendLookAndFeelChange(); 
        repaint(); 
    } 
    else if (s == &uiManager)
    {
        // React to Tab changes
        auto tabName = mainTabs.getTabNames()[uiManager.getCurrentTab()];
        audioProcessor.getLCDStateManager().onPageChanged(tabName);
    }
    else if (s == &mainTabs.getTabbedButtonBar())
    {
        uiManager.setCurrentTab(mainTabs.getCurrentTabIndex());
    }
}

void CZ101AudioProcessorEditor::lookAndFeelChanged()
{
    auto& skin = CZ101::UI::SkinManager::getInstance();
    auto& palette = skin.getCurrentPalette();
    
    // Theme Main Tabs
    auto& mainBar = mainTabs.getTabbedButtonBar();
    mainBar.setColour(juce::TabbedButtonBar::tabTextColourId, palette.textPrimary);
    mainBar.setColour(juce::TabbedButtonBar::frontTextColourId, palette.textPrimary);
    mainBar.setColour(juce::TabbedButtonBar::tabOutlineColourId, palette.border);
    
    // Theme Envelope Tabs
    auto& envBar = envelopeTabs.getTabbedButtonBar();
    envBar.setColour(juce::TabbedButtonBar::tabTextColourId, palette.textPrimary);
    envBar.setColour(juce::TabbedButtonBar::frontTextColourId, palette.textPrimary);
    envBar.setColour(juce::TabbedButtonBar::tabOutlineColourId, palette.border);
}

bool CZ101AudioProcessorEditor::keyPressed(const juce::KeyPress& k) {
    auto& um = audioProcessor.getUndoManager();
    if (k.isKeyCode('z') && k.getModifiers().isCommandDown()) { um.undo(); return true; }
    if (k.isKeyCode('y') && k.getModifiers().isCommandDown()) { um.redo(); return true; }
    return AudioProcessorEditor::keyPressed(k);
}

float CZ101AudioProcessorEditor::getScaleFactor() const { return getWidth() / 800.0f; }

