#include "ModulationMatrixSection.h"
#include "../SkinManager.h"
#include "../DesignTokens.h"

namespace CZ101 {
namespace UI {

ModulationMatrixSection::ModulationMatrixSection(CZ101AudioProcessor& p)
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

ModulationMatrixSection::~ModulationMatrixSection()
{
    audioProcessor.getParameters().getAPVTS().removeParameterListener("OPERATION_MODE", this);
}

void ModulationMatrixSection::parameterChanged(const juce::String&, float)
{
    juce::MessageManager::callAsync([this]() { 
        updateVisibility(); 
        resized(); // Recalculate layout
    });
}

void ModulationMatrixSection::updateVisibility()
{
    bool isModern = false;
    if (auto* p = audioProcessor.getParameters().getOperationMode())
        isModern = (p->getIndex() == 2);
        
    setVisible(isModern);

    bool visible = isModern;

    veloToDcwKnob.setVisible(visible);
    veloToDcaKnob.setVisible(visible);
    wheelToDcwKnob.setVisible(visible);
    wheelToLfoRateKnob.setVisible(visible);
    wheelToVibKnob.setVisible(visible);
    atToDcwKnob.setVisible(visible);
    atToVibKnob.setVisible(visible);
    ktDcwKnob.setVisible(visible);
    ktPitchKnob.setVisible(visible);

    repaint();
}

void ModulationMatrixSection::paint(juce::Graphics& g)
{
    auto& palette = SkinManager::getInstance().getCurrentPalette();
    g.fillAll(palette.sectionBackground);

    g.setFont(getScaledFont(14.0f).boldened());
    g.setColour(palette.textPrimary);
    
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
        g.setFont(getScaledFont(11.0f).boldened());
        g.setColour(palette.textPrimary.withAlpha(0.6f));

        auto gridArea = getLocalBounds().reduced(20);
        gridArea.removeFromTop(40);

        int colW = gridArea.getWidth() / 6;
        int rowH = gridArea.getHeight() / 4;

        juce::StringArray cols = { "TIMBRE", "AMP", "SPEED", "PITCH", "VIB" };
        float scale = getUiScale();
        for (int i = 0; i < 5; ++i) {
            g.drawText(cols[i], gridArea.getX() + colW + (i * colW), gridArea.getY() - (int)(20 * scale), colW, (int)(20 * scale), juce::Justification::centred);
        }

        juce::StringArray rows = { "VELO", "WHEEL", "AT", "KT" };
        for (int i = 0; i < 4; ++i) {
            g.drawText(rows[i], gridArea.getX(), gridArea.getY() + (i * rowH), colW, rowH, juce::Justification::centredRight);
        }
    }
}

void ModulationMatrixSection::resized()
{
    float scale = getUiScale();
    auto area = getLocalBounds().reduced((int)(20 * scale));
    area.removeFromTop((int)(40 * scale));

    int colW = area.getWidth() / 6;
    int rowH = area.getHeight() / 4;

    auto getCell = [&](int row, int col) {
        return juce::Rectangle<int>(area.getX() + colW + (col * colW), 
                                  area.getY() + (row * rowH), 
                                  colW, rowH).reduced(4);
    };

    veloToDcwKnob.setBounds(getCell(0, 0));
    veloToDcaKnob.setBounds(getCell(0, 1));
    wheelToDcwKnob.setBounds(getCell(1, 0));
    wheelToLfoRateKnob.setBounds(getCell(1, 2));
    wheelToVibKnob.setBounds(getCell(1, 4));
    atToDcwKnob.setBounds(getCell(2, 0));
    atToVibKnob.setBounds(getCell(2, 4));
    ktDcwKnob.setBounds(getCell(3, 0));
    ktPitchKnob.setBounds(getCell(3, 3));
}

} // namespace UI
} // namespace CZ101
