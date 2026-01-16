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
#include "UI/Components/ArpeggiatorPanel.h"
#include "UI/Components/ModulationMatrixComponent.h"
#include "UI/Components/AboutDialog.h"
#include "UI/UIManager.h"

class CZ101AudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::MidiKeyboardState::Listener,
                                   private juce::Timer,
                                   public juce::FileDragAndDropTarget,
                                   public juce::MenuBarModel,
                                   public juce::ChangeListener,
                                   public juce::AudioProcessorValueTreeState::Listener
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
    juce::TextButton randomButton { "RANDOM" };
    juce::TextButton panicButton { "ALL OFF" };
    
    std::unique_ptr<juce::MidiOutput> activeMidiOutput;
    std::unique_ptr<juce::FileChooser> fileChooser;
    
    // Tooltip Implementation
    juce::TooltipWindow tooltipWindow { this, 700 }; // 700ms delay

    // Secciones del Cuerpo
    CZ101::UI::OscillatorSection oscSection;
    CZ101::UI::EffectsSection effectsSection;
    CZ101::UI::FilterLfoSection filterLfoSection;
    CZ101::UI::ModulationMatrixComponent modMatrixSection;
    CZ101::UI::GeneralSection generalSection;
    CZ101::UI::ArpeggiatorPanel arpPanel;
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
    
    // Helper Class for Dual View
    struct DualEnvelopeContainer : public juce::Component {
        CZ101::UI::EnvelopeEditor& l1;
        CZ101::UI::EnvelopeEditor& l2;
        juce::Label label1 { {}, "LINE 1" };
        juce::Label label2 { {}, "LINE 2" };
        
        DualEnvelopeContainer(CZ101::UI::EnvelopeEditor& e1, CZ101::UI::EnvelopeEditor& e2) : l1(e1), l2(e2) {
            addAndMakeVisible(l1); addAndMakeVisible(l2);
            addAndMakeVisible(label1); addAndMakeVisible(label2);
            label1.setJustificationType(juce::Justification::centred);
            label2.setJustificationType(juce::Justification::centred);
            label1.setColour(juce::Label::textColourId, juce::Colours::cyan);
            // label2 is default
        }
        void resized() override {
            auto area = getLocalBounds();
            auto h = area.getHeight() / 2;
            auto area1 = area.removeFromTop(h);
            label1.setBounds(area1.removeFromTop(20));
            l1.setBounds(area1);
            
            label2.setBounds(area.removeFromTop(20));
            l2.setBounds(area);
        }
    };

    // Envolventes (Split View)
    juce::TabbedComponent envelopeTabs { juce::TabbedButtonBar::TabsAtTop };
    
    // Main Tabs (Osc/Filt/Env/Effects)
    juce::TabbedComponent mainTabs { juce::TabbedButtonBar::TabsAtTop }; 
    
    // Editor Instances
    CZ101::UI::EnvelopeEditor pitchEditorL1, pitchEditorL2;
    CZ101::UI::EnvelopeEditor dcwEditorL1, dcwEditorL2;
    CZ101::UI::EnvelopeEditor dcaEditorL1, dcaEditorL2;
    
    // Containers
    std::unique_ptr<DualEnvelopeContainer> pitchContainer;
    std::unique_ptr<DualEnvelopeContainer> dcwContainer;
    std::unique_ptr<DualEnvelopeContainer> dcaContainer;

    // Overlays
    CZ101::UI::NameEditorOverlay nameOverlay;
    CZ101::UI::AboutDialog aboutDialog;
    
    struct BankManagerOverlay : public juce::Component, public juce::ListBoxModel {
        CZ101::State::PresetManager* pm = nullptr;
        juce::ListBox listBox { "BankList", this };
        juce::TextButton closeButton { "CLOSE" };
        std::function<void()> onUpdate;
        
        BankManagerOverlay() {
            addAndMakeVisible(listBox);
            addAndMakeVisible(closeButton);
            closeButton.onClick = [this]() { setVisible(false); };
            listBox.setRowHeight(30);
        }
        
        void paint(juce::Graphics& g) override {
            g.fillAll(juce::Colours::black.withAlpha(0.85f));
            g.setColour(juce::Colours::white);
            g.setFont(20.0f);
            g.drawText("BANK MANAGER", getLocalBounds().removeFromTop(40), juce::Justification::centred);
        }
        
        void resized() override {
            auto area = getLocalBounds().reduced(40);
            area.removeFromTop(40); // Title space
            closeButton.setBounds(area.removeFromBottom(40).reduced(100, 5));
            listBox.setBounds(area);
        }
        
        int getNumRows() override { return pm ? (int)pm->getPresets().size() : 0; }
        
        void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override {
            if (pm && row < (int)pm->getPresets().size()) {
                if (selected) g.fillAll(juce::Colours::cyan.withAlpha(0.3f));
                g.setColour(juce::Colours::white);
                g.drawText(juce::String(row + 1) + ": " + pm->getPresets()[row].name, 5, 0, w - 80, h, juce::Justification::centredLeft);
                
                // Draw Up/Down/Delete icons/buttons?
                // For now, simplicity: keyboard or right click? 
                // Let's add simple buttons in row? No, ListBox with custom components is better for that.
            }
        }
        
        void listBoxItemClicked(int row, const juce::MouseEvent& e) override {
            if (e.mods.isRightButtonDown()) {
                juce::PopupMenu m;
                m.addItem(1, "Move Up");
                m.addItem(2, "Move Down");
                m.addItem(4, "Move to Position...");
                m.addSeparator();
                m.addItem(5, "Rename...");
                m.addSeparator();
                m.addItem(3, "Delete");
                
                m.showMenuAsync(juce::PopupMenu::Options(), [this, row](int result) {
                    if (result == 1) pm->movePreset(row, row - 1);
                    else if (result == 2) pm->movePreset(row, row + 1);
                    else if (result == 4) {
                        auto* dw = new juce::AlertWindow("Move Preset", "Enter new position (1-" + juce::String(pm->getPresets().size()) + "):", juce::AlertWindow::QuestionIcon);
                        dw->addTextEditor("pos", juce::String(row + 1));
                        dw->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
                        dw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
                        dw->enterModalState(true, juce::ModalCallbackFunction::create([this, dw, row](int r) {
                            if (r == 1) {
                                int newPos = dw->getTextEditorContents("pos").getIntValue() - 1;
                                int maxPos = (int)pm->getPresets().size() - 1;
                                if (newPos >= 0 && newPos <= maxPos) {
                                    pm->movePreset(row, newPos);
                                    listBox.updateContent();
                                    if (onUpdate) onUpdate();
                                }
                            }
                        }));
                    }
                    else if (result == 5) {
                        auto* dw = new juce::AlertWindow("Rename Preset", "Enter new name:", juce::AlertWindow::QuestionIcon);
                        dw->addTextEditor("name", pm->getPresets()[row].name);
                        dw->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
                        dw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
                        dw->enterModalState(true, juce::ModalCallbackFunction::create([this, dw, row](int r) {
                            if (r == 1) {
                                pm->renamePreset(row, dw->getTextEditorContents("name").toStdString());
                                listBox.updateContent();
                                if (onUpdate) onUpdate();
                            }
                        }));
                    }
                    else if (result == 3) pm->deletePreset(row);
                    
                    if (result > 0 && result != 4 && result != 5) {
                        listBox.updateContent();
                        if (onUpdate) onUpdate();
                    }
                });
            }
        }
    } bankManagerOverlay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CZ101AudioProcessorEditor)
};
