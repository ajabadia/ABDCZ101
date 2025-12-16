#include "PresetBrowser.h"

namespace CZ101 {
namespace UI {

PresetBrowser::PresetBrowser()
{
    addAndMakeVisible(presetCombo);
    addAndMakeVisible(prevButton);
    addAndMakeVisible(nextButton);
    addAndMakeVisible(menuButton);
    
    prevButton.setButtonText("<");
    nextButton.setButtonText(">");
    menuButton.setButtonText("MENU");
    
    // updatePresetList() cannot be called here yet as manager is null
    
    presetCombo.onChange = [this]()
    {
        selectPreset(presetCombo.getSelectedItemIndex());
    };
    
    prevButton.onClick = [this]()
    {
        int current = presetCombo.getSelectedItemIndex();
        if (current > 0)
            presetCombo.setSelectedItemIndex(current - 1);
    };
    
    nextButton.onClick = [this]()
    {
        int current = presetCombo.getSelectedItemIndex();
        if (current < presetCombo.getNumItems() - 1)
            presetCombo.setSelectedItemIndex(current + 1);
    };
    
    menuButton.onClick = [this] { showMenu(); };
}

void PresetBrowser::setPresetManager(State::PresetManager* pm)
{
    presetManager = pm;
    if (presetManager)
        updatePresetList();
}

void PresetBrowser::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2a2a2a));
}

void PresetBrowser::resized()
{
    auto bounds = getLocalBounds().reduced(5);
    
    // Left: MENU (60px)
    menuButton.setBounds(bounds.removeFromLeft(60));
    bounds.removeFromLeft(5);
    
    // Nav Buttons
    prevButton.setBounds(bounds.removeFromLeft(30));
    bounds.removeFromLeft(5);
    nextButton.setBounds(bounds.removeFromRight(30));
    bounds.removeFromRight(5);
    
    presetCombo.setBounds(bounds);
}

void PresetBrowser::showMenu()
{
    juce::PopupMenu m;
    m.addSectionHeader("Bank Management");
    m.addItem(1, "Init Bank (Factory Reset)");
    m.addSeparator();
    m.addItem(2, "Load Bank (.json)...");
    m.addItem(3, "Save Bank (.json)...");
    
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&menuButton),
        [this](int result)
        {
            if (result == 1) initBank();
            else if (result == 2) loadBank();
            else if (result == 3) saveBank();
        });
}

void PresetBrowser::initBank()
{
    if (!presetManager) return;

    // Show confirmation dialog
    juce::AlertWindow::showOkCancelBox(
        juce::AlertWindow::WarningIcon,
        "Reset Bank",
        "Are you sure? This will replace all presets with factory defaults.",
        "Reset",
        "Cancel",
        nullptr,
        juce::ModalCallbackFunction::create([this](int result)
        {
            // result == 1 means OK/Reset pressed
            if (result == 1)
            {
                presetManager->resetToFactory();
                updatePresetList();
                selectPreset(0);
            }
        })
    );
}

void PresetBrowser::loadBank()
{
    if (!presetManager) return;
    fileChooser = std::make_unique<juce::FileChooser>("Load Bank", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.json");
    
    auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file.existsAsFile())
        {
            presetManager->loadBank(file);
            updatePresetList();
            selectPreset(0); // Go to first preset
        }
    });
}

void PresetBrowser::saveBank()
{
    if (!presetManager) return;
    fileChooser = std::make_unique<juce::FileChooser>("Save Bank", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.json");
    
    auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting;
    fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file != juce::File())
        {
            presetManager->saveBank(file);
        }
    });
}

void PresetBrowser::updatePresetList()
{
    presetCombo.clear();
    
    if (presetManager == nullptr) return;
    
    const auto& presets = presetManager->getPresets();
    for (size_t i = 0; i < presets.size(); ++i)
    {
        presetCombo.addItem(presets[i].name, static_cast<int>(i) + 1);
    }
    
    presetCombo.setSelectedItemIndex(0);
}

void PresetBrowser::selectPreset(int index)
{
    if (presetManager)
        presetManager->loadPreset(index);

    if (onPresetSelected)
        onPresetSelected(index);
}

int PresetBrowser::getSelectedItemIndex() const
{
    return presetCombo.getSelectedItemIndex();
}

void PresetBrowser::setSelectedItemIndex(int index)
{
    presetCombo.setSelectedItemIndex(index);
}

} // namespace UI
} // namespace CZ101
