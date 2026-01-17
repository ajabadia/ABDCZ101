#include "ModulationMatrixComponent.h"
#include "../SkinManager.h"
#include "../DesignTokens.h"

namespace CZ101 {
namespace UI {

ModulationMatrixComponent::ModulationMatrixComponent(CZ101AudioProcessor& p)
    : audioProcessor(p),
      veloToDcwKnob(""),
      veloToDcaKnob(""),
      wheelToDcwKnob(""),
      wheelToLfoRateKnob(""),
      wheelToVibKnob(""),
      atToDcwKnob(""),
      atToVibKnob(""),
      ktDcwKnob(""),
      ktPitchKnob("")
{
    auto& apvts = audioProcessor.getParameters().getAPVTS();

    auto setupKnob = [&](Knob& knob, const juce::String& paramId) {
        addAndMakeVisible(knob);
        attachments.push_back(std::make_unique<SliderAttachment>(apvts, paramId, knob.getSlider()));
        knob.getSlider().getProperties().set("paramId", paramId);
    };

    setupKnob(veloToDcwKnob, "MOD_VELO_DCW");
    setupKnob(veloToDcaKnob, "MOD_VELO_DCA");
    setupKnob(wheelToDcwKnob, "MOD_WHEEL_DCW");
    setupKnob(wheelToLfoRateKnob, "MOD_WHEEL_LFORATE");
    setupKnob(wheelToVibKnob, "MOD_WHEEL_VIB");
    setupKnob(atToDcwKnob, "MOD_AT_DCW");
    setupKnob(atToVibKnob, "MOD_AT_VIB");
    setupKnob(ktDcwKnob, "KEY_TRACK_DCW");
    setupKnob(ktPitchKnob, "KEY_TRACK_PITCH");

    audioProcessor.getParameters().getAPVTS().addParameterListener("OPERATION_MODE", this);
    updateVisibility();
}

ModulationMatrixComponent::~ModulationMatrixComponent()
{
    audioProcessor.getParameters().getAPVTS().removeParameterListener("OPERATION_MODE", this);
}

void ModulationMatrixComponent::parameterChanged(const juce::String&, float)
{
    juce::MessageManager::callAsync([this]() { updateVisibility(); });
}

void ModulationMatrixComponent::updateVisibility()
{
    // If in Classic mode (0 or 1), the matrix might be disabled or limited?
    // The requirement was: "Modern extends features".
    // If we want to hide it in classic modes:
    bool isModern = false;
    if (auto* p = audioProcessor.getParameters().getOperationMode())
        isModern = (p->getIndex() == 2);
        
    setVisible(isModern); // Example: Matrix only visible in modern mode

    bool visible = isModern; // Use isModern for individual knob visibility

    veloToDcwKnob.setVisible(visible);
    veloToDcaKnob.setVisible(visible);
    wheelToDcwKnob.setVisible(visible);
    wheelToLfoRateKnob.setVisible(visible);
    wheelToVibKnob.setVisible(visible);
    atToDcwKnob.setVisible(visible);
    atToVibKnob.setVisible(visible);
    
    // Key Tracking might be available in Classic? 
    // Usually Key Follow is standard. But these might be "extra" routings?
    // "Key Track DCW" / "Key Track Pitch"?
    // CZ-101 has fixed key tracking logic or minimal control.
    // If these represent modern flexible routing, hide them.
    // Assuming these are modern additions for now based on context.
    ktDcwKnob.setVisible(visible);
    ktPitchKnob.setVisible(visible);

    repaint();
}

void ModulationMatrixComponent::paint(juce::Graphics& g)
{
    auto& palette = SkinManager::getInstance().getCurrentPalette();
    g.fillAll(palette.sectionBackground); // Use themed background

    g.setFont(getScaledFont(14.0f).boldened());
    g.setColour(palette.textPrimary); // Use themed text
    
    auto bounds = getLocalBounds().toFloat();
    g.drawText("MODULATION MATRIX (MODERN)", bounds.removeFromTop(20), juce::Justification::centred, false);

    if (!veloToDcwKnob.isVisible())
    {
        g.setFont(getScaledFont(12.0f));
        g.setColour(palette.textSecondary);
        g.drawText("Not Available in Classic Mode", bounds, juce::Justification::centred, false);
    }
    else
    {
        // Draw Grid Labels
        g.setFont(getScaledFont(11.0f).boldened());
        g.setColour(palette.textPrimary.withAlpha(0.6f));

        auto gridArea = getLocalBounds().reduced(20);
        gridArea.removeFromTop(40); // Title + Header space

        int colW = gridArea.getWidth() / 6;
        int rowH = gridArea.getHeight() / 4;

        // Column headers
        juce::StringArray cols = { "TIMBRE", "AMP", "SPEED", "PITCH", "VIB" };
        float scale = getUiScale();
        for (int i = 0; i < 5; ++i) {
            g.drawText(cols[i], gridArea.getX() + colW + (i * colW), gridArea.getY() - (int)(20 * scale), colW, (int)(20 * scale), juce::Justification::centred);
        }

        // Row headers
        juce::StringArray rows = { "VELO", "WHEEL", "AT", "KT" };
        for (int i = 0; i < 4; ++i) {
            g.drawText(rows[i], gridArea.getX(), gridArea.getY() + (i * rowH), colW, rowH, juce::Justification::centredRight);
        }
    }
}

void ModulationMatrixComponent::resized()
{
    float scale = getUiScale();
    auto area = getLocalBounds().reduced((int)(20 * scale));
    area.removeFromTop((int)(40 * scale)); // Title + Header gap

    // REMOVED visibility check here to ensure layout is done even if initially hidden

    int colW = area.getWidth() / 6;
    int rowH = area.getHeight() / 4;

    auto getCell = [&](int row, int col) {
        return juce::Rectangle<int>(area.getX() + colW + (col * colW), 
                                  area.getY() + (row * rowH), 
                                  colW, rowH).reduced(4);
    };

    // VELO (Row 0)
    veloToDcwKnob.setBounds(getCell(0, 0));
    veloToDcaKnob.setBounds(getCell(0, 1));

    // WHEEL (Row 1)
    wheelToDcwKnob.setBounds(getCell(1, 0));
    wheelToLfoRateKnob.setBounds(getCell(1, 2));
    wheelToVibKnob.setBounds(getCell(1, 4));

    // AT (Row 2)
    atToDcwKnob.setBounds(getCell(2, 0));
    atToVibKnob.setBounds(getCell(2, 4));

    // KT (Row 3)
    ktDcwKnob.setBounds(getCell(3, 0));
    ktPitchKnob.setBounds(getCell(3, 3));
}

} // namespace UI
} // namespace CZ101
