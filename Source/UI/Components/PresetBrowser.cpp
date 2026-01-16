#include "PresetBrowser.h"
#include "../../State/PresetManager.h"
#include "../SkinManager.h"

namespace CZ101 {
namespace UI {

PresetBrowser::PresetBrowser()
{
    addAndMakeVisible(presetCombo);
    addAndMakeVisible(prevButton);
    addAndMakeVisible(nextButton);
    prevButton.setButtonText("<");
    nextButton.setButtonText(">");
    // Wire changes
    presetCombo.onChange = [this]() { selectPreset(presetCombo.getSelectedItemIndex()); };
    
    prevButton.onClick = [this]() {
        int current = presetCombo.getSelectedItemIndex();
        if (current > 0) presetCombo.setSelectedItemIndex(current - 1);
    };
    
    nextButton.onClick = [this]() {
        int current = presetCombo.getSelectedItemIndex();
        if (current < presetCombo.getNumItems() - 1) presetCombo.setSelectedItemIndex(current + 1);
    };
}

void PresetBrowser::setPresetManager(State::PresetManager* pm)
{
    presetManager = pm;
    if (presetManager) updatePresetList();
}

void PresetBrowser::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto& palette = SkinManager::getInstance().getCurrentPalette();
    
    // 1. Hardware-style Recessed Background
    g.setColour(palette.surface);
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // 2. Subtle Border
    g.setColour(palette.border);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    
    g.setColour(juce::Colours::white.withAlpha(0.05f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
}

void PresetBrowser::resized()
{
    auto bounds = getLocalBounds().reduced(4);
    int h = bounds.getHeight();
    prevButton.setBounds(bounds.removeFromLeft(h));
    nextButton.setBounds(bounds.removeFromRight(h));
    
    bounds.reduce(4, 0);
    presetCombo.setBounds(bounds);
}

void PresetBrowser::initBank()
{
    if (!presetManager) return;
    juce::AlertWindow::showOkCancelBox(juce::AlertWindow::WarningIcon, "Reset Bank", "Are you sure? This will replace all presets with factory defaults.", "Reset", "Cancel", nullptr,
        juce::ModalCallbackFunction::create([this](int result) {
            if (result == 1) {
                presetManager->resetToFactory();
                updatePresetList();
                selectPreset(0);
            }
        }));
}

void PresetBrowser::loadBank()
{
    if (!presetManager) return;
    fileChooser = std::make_unique<juce::FileChooser>("Load Bank", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.json");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this](const juce::FileChooser& fc) {
        auto file = fc.getResult();
        if (file.existsAsFile()) {
            presetManager->loadBank(file);
            updatePresetList();
            selectPreset(0);
        }
    });
}

void PresetBrowser::saveBank()
{
    if (!presetManager) return;
    fileChooser = std::make_unique<juce::FileChooser>("Save Bank", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.json");
    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting, [this](const juce::FileChooser& fc) {
        auto file = fc.getResult();
        if (file != juce::File()) presetManager->saveBank(file);
    });
}

void PresetBrowser::updatePresetList()
{
    presetCombo.clear();
    if (!presetManager) return;
    const auto& presets = presetManager->getPresets();
    for (size_t i = 0; i < presets.size(); ++i)
        presetCombo.addItem(presets[i].name, static_cast<int>(i) + 1);
    
    int currentIdx = presetManager->getCurrentPresetIndex();
    presetCombo.setSelectedItemIndex(currentIdx >= 0 ? currentIdx : 0, juce::dontSendNotification);
}

void PresetBrowser::selectPreset(int index)
{
    if (presetManager) presetManager->loadPreset(index);
    if (onPresetSelected) onPresetSelected(index);
}

} // namespace UI
} // namespace CZ101
