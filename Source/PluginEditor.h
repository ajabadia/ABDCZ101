#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/CZ101LookAndFeel.h"
#include "UI/DesignTokens.h"
#include "UI/Sections/OscillatorSection.h"
#include "UI/Sections/EffectsSection.h"
#include "UI/Sections/GeneralSection.h"
#include "UI/Sections/FilterLfoSection.h"
#include "UI/Components/EnvelopeEditor.h"
#include "UI/Components/WaveformDisplay.h"
#include "UI/LCDDisplay.h"
#include "UI/Components/PresetBrowser.h"
#include "UI/Components/MIDIActivityIndicator.h"
#include "UI/Components/PresetBrowser.h"
#include "UI/Components/MIDIActivityIndicator.h"
#include "UI/Overlays/NameEditorOverlay.h"
#include "UI/Sections/ArpeggiatorSection.h"
#include "UI/Sections/ModulationMatrixSection.h"
#include "UI/Components/AboutDialog.h"
#include "UI/Components/DualEnvelopeContainer.h"
#include "UI/Overlays/BankManagerOverlay.h"
#include "UI/UIManager.h"

class CZ101AudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::MidiKeyboardState::Listener,
                                   private juce::Timer,
                                   public juce::FileDragAndDropTarget,
                                   public juce::MenuBarModel,
                                   public juce::ChangeListener,
                                   public juce::AudioProcessorValueTreeState::Listener,
                                   public CZ101::State::PresetManager::Listener
{
public:
    CZ101AudioProcessorEditor(CZ101AudioProcessor&);
    ~CZ101AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void lookAndFeelChanged() override;
    bool keyPressed(const juce::KeyPress& key) override; // Added Keyboard Handling
    float getScaleFactor() const; // For Layout Scaling

    CZ101AudioProcessor& getAudioProcessor() { return audioProcessor; }
    CZ101::UI::UIManager& getUIManager() { return uiManager; }
    
    // MenuBarModel Overrides
    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int menuIndex, const juce::String& menuName) override;
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;
    
    // ChangeListener Override
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    // APVTS Listener Override
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // Preset Listener Override
    void presetLoaded(int index) override;
    void bankUpdated() override;
    void presetRenamed(int index, const std::string& newName) override;

private:
    // --- Métodos de Lógica y Eventos ---
    void timerCallback() override;
    void handleNoteOn(juce::MidiKeyboardState*, int, int, float) override;
    void handleNoteOff(juce::MidiKeyboardState*, int, int, float) override;
    void loadPatchFile();
    void savePatchFile();
    void renameCurrentPatch();
    void storeCurrentPatch();
    void storeToNewSlot();
    void openBankManager();
    void exportOriginalSysEx();
    void handleNameUpdate(const juce::String& newName);

    void loadSysExFile();
    void saveSysExFile();
    void randomizePatch();
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    CZ101AudioProcessor& audioProcessor;
    CZ101::UI::UIManager uiManager;
    CZ101::UI::CZ101LookAndFeel customLookAndFeel;
    juce::OpenGLContext openGLContext;
    
    // Audit Fix 1.1: Atomic guard for LCD
    std::atomic<bool> isProcessingLCDCommand{false};
    
    // --- Componentes Principales y Secciones UI ---
    juce::MidiKeyboardState keyboardState;
    juce::MidiKeyboardComponent keyboardComponent;
    
    // Top Menu
    std::unique_ptr<juce::MenuBarComponent> menuBar;
    
    // Header
    CZ101::UI::LCDDisplay lcdDisplay;
    CZ101::UI::PresetBrowser presetBrowser;
    CZ101::UI::MIDIActivityIndicator midiIndicator;
    juce::TextButton randomButton { "RNDM" };
    juce::TextButton panicButton { "ALL OFF" };
    juce::TextButton compareButton { "COMPR" }; // Audit Fix 10.6
    
    std::unique_ptr<juce::MidiOutput> activeMidiOutput;
    std::unique_ptr<juce::FileChooser> fileChooser;
    
    // Tooltip Implementation
    juce::TooltipWindow tooltipWindow { this, 700 }; // 700ms delay

    // Secciones del Cuerpo
    CZ101::UI::OscillatorSection oscSection;
    CZ101::UI::EffectsSection effectsSection;
    CZ101::UI::FilterLfoSection filterLfoSection;
    CZ101::UI::ModulationMatrixSection modMatrixSection;
    CZ101::UI::GeneralSection generalSection;
    CZ101::UI::ArpeggiatorSection arpPanel;
    CZ101::UI::WaveformDisplay waveformDisplay;
    
    // Line Zoning (Visual only now)
    // juce::TextButton line1Button { "LINE 1" }; // Removed
    // juce::TextButton line2Button { "LINE 2" }; // Removed
    int currentLineView = 1;

    // LCD Navigation
    juce::TextButton cursorLeft  { "<" };
    juce::TextButton cursorRight { ">" };
    juce::TextButton cursorUp    { "^" };
    juce::TextButton cursorDown  { "v" };
    

    // Envolventes (Split View)
    juce::TabbedComponent envelopeTabs { juce::TabbedButtonBar::TabsAtTop };
    
    // Main Tabs (Osc/Filt/Env/Effects)
    juce::TabbedComponent mainTabs { juce::TabbedButtonBar::TabsAtTop }; 
    
    // Editor Instances
    CZ101::UI::EnvelopeEditor pitchEditorL1, pitchEditorL2;
    CZ101::UI::EnvelopeEditor dcwEditorL1, dcwEditorL2;
    CZ101::UI::EnvelopeEditor dcaEditorL1, dcaEditorL2;
    
    // Containers
    std::unique_ptr<CZ101::UI::DualEnvelopeContainer> pitchContainer;
    std::unique_ptr<CZ101::UI::DualEnvelopeContainer> dcwContainer;
    std::unique_ptr<CZ101::UI::DualEnvelopeContainer> dcaContainer;

    // Overlays
    CZ101::UI::NameEditorOverlay nameOverlay;
    CZ101::UI::AboutDialog aboutDialog;
    
    CZ101::UI::BankManagerOverlay bankManagerOverlay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CZ101AudioProcessorEditor)
};
