#include "NameEditorOverlay.h"

namespace CZ101 {
namespace UI {

NameEditorOverlay::NameEditorOverlay()
{
    addAndMakeVisible(titleLabel);
    titleLabel.setText("RENAME PRESET", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    
    addAndMakeVisible(nameEditor);
    nameEditor.setJustification(juce::Justification::centred);
    
    addAndMakeVisible(saveButton);
    addAndMakeVisible(cancelButton);
    
    saveButton.addListener(this);
    cancelButton.addListener(this);
    
    // Enter key support
    nameEditor.onReturnKey = [this] { buttonClicked(&saveButton); };
    nameEditor.onEscapeKey = [this] { buttonClicked(&cancelButton); };
}

NameEditorOverlay::~NameEditorOverlay()
{
    saveButton.removeListener(this);
    cancelButton.removeListener(this);
}

void NameEditorOverlay::paint(juce::Graphics& g)
{
    // Semi-transparent background for the whole component
    g.fillAll(juce::Colours::black.withAlpha(0.6f));
    
    // Dialog box
    auto bounds = getLocalBounds().toFloat();
    // Create a 300x150 rectangle centered in bounds
    auto dialogRect = juce::Rectangle<float>(300, 150).withCentre(bounds.getCentre());
    
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRoundedRectangle(dialogRect, 10.0f);
    
    g.setColour(juce::Colours::cyan);
    g.drawRoundedRectangle(dialogRect, 10.0f, 2.0f);
}

void NameEditorOverlay::resized()
{
    auto bounds = getLocalBounds();
    auto center = bounds.getCentre();
    
    int w = 300;
    int h = 150;
    int x = center.x - w/2;
    int y = center.y - h/2;
    
    titleLabel.setBounds(x + 10, y + 10, w - 20, 30);
    nameEditor.setBounds(x + 20, y + 50, w - 40, 30);
    
    cancelButton.setBounds(x + 20, y + 100, 120, 30);
    saveButton.setBounds(x + 160, y + 100, 120, 30);
}

void NameEditorOverlay::startRename(const juce::String& currentName, std::function<void(const juce::String&)> onSaveCallback)
{
    nameEditor.setText(currentName);
    onSave = onSaveCallback;
    setVisible(true);
    toFront(true);
    nameEditor.grabKeyboardFocus();
}

void NameEditorOverlay::buttonClicked(juce::Button* b)
{
    if (b == &saveButton)
    {
        if (onSave) onSave(nameEditor.getText());
        setVisible(false);
    }
    else if (b == &cancelButton)
    {
        setVisible(false);
    }
}

} // namespace UI
} // namespace CZ101
