#include "BankManagerOverlay.h"

namespace CZ101 {
namespace UI {

BankManagerOverlay::BankManagerOverlay() 
{
    addAndMakeVisible(listBox);
    addAndMakeVisible(closeButton);
    closeButton.onClick = [this]() { setVisible(false); };
    listBox.setRowHeight(30);
}

void BankManagerOverlay::paint(juce::Graphics& g) 
{
    g.fillAll(juce::Colours::black.withAlpha(0.85f));
    g.setColour(juce::Colours::white);
    g.setFont(20.0f);
    g.drawText("BANK MANAGER", getLocalBounds().removeFromTop(40), juce::Justification::centred);
}

void BankManagerOverlay::resized() 
{
    auto area = getLocalBounds().reduced(40);
    area.removeFromTop(40); // Title space
    closeButton.setBounds(area.removeFromBottom(40).reduced(100, 5));
    listBox.setBounds(area);
}

int BankManagerOverlay::getNumRows() 
{ 
    if (!pm) return 0;
    const juce::ScopedReadLock sl(pm->getLock());
    return (int)pm->getPresets().size(); 
}

void BankManagerOverlay::paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) 
{
    if (!pm) return;
    const juce::ScopedReadLock sl(pm->getLock());
    
    if (row < (int)pm->getPresets().size()) {
        if (selected) g.fillAll(juce::Colours::cyan.withAlpha(0.3f));
        g.setColour(juce::Colours::white);
        g.drawText(juce::String(row + 1) + ": " + pm->getPresets()[row].name, 5, 0, w - 80, h, juce::Justification::centredLeft);
    }
}

void BankManagerOverlay::listBoxItemClicked(int row, const juce::MouseEvent& e) 
{
    if (e.mods.isRightButtonDown()) {
        juce::PopupMenu m;
        m.addItem(1, "Move Up");
        m.addItem(2, "Move Down");
        m.addItem(4, "Move to Position...");
        m.addSeparator();
        m.addItem(5, "Rename...");
        m.addSeparator();
        m.addItem(3, "Delete");
        
        juce::Component::SafePointer<BankManagerOverlay> safeThis(this);
        
        m.showMenuAsync(juce::PopupMenu::Options(), [safeThis, row](int result) {
            if (safeThis == nullptr || safeThis->pm == nullptr) return;
            
            auto* overlay = safeThis.getComponent();
            auto* pm = overlay->pm;
            
            if (result == 1) pm->movePreset(row, row - 1);
            else if (result == 2) pm->movePreset(row, row + 1);
            else if (result == 4) {
                auto* dw = new juce::AlertWindow("Move Preset", "Enter new position (1-" + juce::String(pm->getPresets().size()) + "):", juce::AlertWindow::QuestionIcon);
                dw->addTextEditor("pos", juce::String(row + 1));
                dw->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
                dw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
                
                dw->enterModalState(true, juce::ModalCallbackFunction::create([safeThis, dw, row](int r) {
                    if (safeThis == nullptr || safeThis->pm == nullptr) return;
                    
                    if (r == 1) {
                        int newPos = dw->getTextEditorContents("pos").getIntValue() - 1;
                        int maxPos = (int)safeThis->pm->getPresets().size() - 1;
                        if (newPos >= 0 && newPos <= maxPos) {
                            safeThis->pm->movePreset(row, newPos);
                            safeThis->listBox.updateContent();
                            if (safeThis->onUpdate) safeThis->onUpdate();
                        }
                    }
                }));
            }
            else if (result == 5) {
                auto* dw = new juce::AlertWindow("Rename Preset", "Enter new name:", juce::AlertWindow::QuestionIcon);
                const juce::ScopedReadLock sl(pm->getLock());
                dw->addTextEditor("name", pm->getPresets()[row].name);
                
                dw->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
                dw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
                dw->enterModalState(true, juce::ModalCallbackFunction::create([safeThis, dw, row](int r) {
                    if (safeThis == nullptr || safeThis->pm == nullptr) return;
                    
                    if (r == 1) {
                        safeThis->pm->renamePreset(row, dw->getTextEditorContents("name").toStdString());
                        safeThis->listBox.updateContent();
                        if (safeThis->onUpdate) safeThis->onUpdate();
                    }
                }));
            }
            else if (result == 3) {
                 pm->deletePreset(row);
                 safeThis->listBox.updateContent();
                 if (safeThis->onUpdate) safeThis->onUpdate();
            }
            
            if (result > 0 && result != 4 && result != 5 && result != 3) {
                safeThis->listBox.updateContent();
                if (safeThis->onUpdate) safeThis->onUpdate();
            }
        });
    }
}

} // namespace UI
} // namespace CZ101
