#include "PresetBrowser.h"
#include "../../State/PresetManager.h"

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
    
    // High Visibility Colors
    presetCombo.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff202020));
    presetCombo.setColour(juce::ComboBox::textColourId, juce::Colours::cyan);
    presetCombo.setColour(juce::ComboBox::arrowColourId, juce::Colours::cyan);
    presetCombo.setColour(juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha(0.2f));
    
    prevButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    nextButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    menuButton.setColour(juce::TextButton::textColourOffId, juce::Colours::gold);
    
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
    auto bounds = getLocalBounds().toFloat();
    
    // Glass-morphism Effect
    g.setColour(juce::Colour(0xff1a1a1a).withAlpha(0.8f));
    g.fillRoundedRectangle(bounds, 6.0f);
    
    // Gradient Sheen
    juce::ColourGradient sheen(juce::Colours::white.withAlpha(0.1f), 0.0f, 0.0f,
                               juce::Colours::transparentWhite, 0.0f, bounds.getHeight() * 0.5f, false);
    g.setGradientFill(sheen);
    g.fillRoundedRectangle(bounds, 6.0f);
    
    // Border
    g.setColour(juce::Colours::white.withAlpha(0.15f));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
}

void PresetBrowser::resized()
{
    auto bounds = getLocalBounds().reduced(5);
    
    // Left: MENU (Reduced 60 -> 40)
    menuButton.setBounds(bounds.removeFromLeft(40));
    bounds.removeFromLeft(2);
    
    // Nav Buttons (Reduced 30 -> 25)
    prevButton.setBounds(bounds.removeFromLeft(25));
    bounds.removeFromLeft(2);
    nextButton.setBounds(bounds.removeFromRight(25));
    bounds.removeFromRight(2);
    
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
    
    auto browserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    fileChooser->launchAsync(browserFlags, [this](const juce::FileChooser& fc)
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
    
    auto browserFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting;
    fileChooser->launchAsync(browserFlags, [this](const juce::FileChooser& fc)
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
    
    // Sync with current PresetManager index
    int currentIdx = presetManager->getCurrentPresetIndex();
    if (currentIdx >= 0 && currentIdx < presetCombo.getNumItems())
    {
        presetCombo.setSelectedItemIndex(currentIdx, juce::dontSendNotification);
    }
    else
    {
        presetCombo.setSelectedItemIndex(0, juce::dontSendNotification);
    }
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
