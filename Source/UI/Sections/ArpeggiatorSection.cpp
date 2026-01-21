#include "ArpeggiatorSection.h"
#include "../SkinManager.h"
#include "../../State/ParameterIDs.h"

namespace CZ101 {
namespace UI {

ArpeggiatorSection::ArpeggiatorSection(CZ101AudioProcessor& p)
    : audioProcessor(p),
      arpRateKnob("Arp Rate"),
      arpBpmKnob("Tempo"),
      arpGateKnob("Gate"),
      arpSwingKnob("Amount"),
      arpPatternKnob("Pattern"),
      arpOctaveKnob("Octaves"),
      arpLatchButton("LATCH"),
      arpEnableButton("ARP ON")
{
    auto& params = audioProcessor.getParameters();

    auto setupLabel = [this](juce::Label& l, const juce::String& text) {
        addAndMakeVisible(l);
        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setFont(getScaledFont(11.0f).boldened());
    };

    setupLabel(rateLabel, "RATE");
    setupLabel(tempoLabel, "TEMPO");
    setupLabel(patternLabel, "PATTERN");
    setupLabel(octLabel, "OCTAVES");
    setupLabel(gateLabel, "GATE");
    setupLabel(swingLabel, "SWING");
    setupLabel(typeLabel, "MODE");

    addAndMakeVisible(arpEnableButton);
    arpEnableButton.setClickingTogglesState(true);
    if (params.getArpEnabled()) {
        arpEnableAttachment = std::make_unique<ButtonAttachment>(*params.getArpEnabled(), arpEnableButton);
    }

    addAndMakeVisible(arpLatchButton);
    arpLatchButton.setClickingTogglesState(true);
    if (params.getArpLatch()) {
        arpLatchAttachment = std::make_unique<ButtonAttachment>(*params.getArpLatch(), arpLatchButton);
    }

    addAndMakeVisible(arpRateKnob);
    if (params.getArpRate()) {
        arpRateAttachment = std::make_unique<SliderAttachment>(*params.getArpRate(), arpRateKnob.getSlider());
        arpRateKnob.getSlider().getProperties().set("paramId", params.getArpRate()->paramID);
    }

    addAndMakeVisible(arpBpmKnob);
    if (params.getArpBpm()) {
        arpBpmAttachment = std::make_unique<SliderAttachment>(*params.getArpBpm(), arpBpmKnob.getSlider());
        arpBpmKnob.getSlider().getProperties().set("paramId", params.getArpBpm()->paramID);
    }

    addAndMakeVisible(arpGateKnob);
    if (params.getArpGate()) {
        arpGateAttachment = std::make_unique<SliderAttachment>(*params.getArpGate(), arpGateKnob.getSlider());
        arpGateKnob.getSlider().getProperties().set("paramId", params.getArpGate()->paramID);
    }

    addAndMakeVisible(arpSwingKnob);
    if (params.getArpSwing()) {
        arpSwingAttachment = std::make_unique<SliderAttachment>(*params.getArpSwing(), arpSwingKnob.getSlider());
        arpSwingKnob.getSlider().getProperties().set("paramId", params.getArpSwing()->paramID);
    }

    addAndMakeVisible(arpSwingModeCombo);
    if (params.getArpSwingMode()) {
        arpSwingModeCombo.addItemList(params.getArpSwingMode()->choices, 1);
        arpSwingModeAttachment = std::make_unique<ComboAttachment>(*params.getArpSwingMode(), arpSwingModeCombo);
    }

    addAndMakeVisible(arpPatternKnob);
    if (params.getArpPattern()) {
        arpPatternAttachment = std::make_unique<SliderAttachment>(*params.getArpPattern(), arpPatternKnob.getSlider());
        arpPatternKnob.getSlider().getProperties().set("paramId", params.getArpPattern()->paramID);
    }

    addAndMakeVisible(arpOctaveKnob);
    if (params.getArpOctave()) {
        arpOctaveAttachment = std::make_unique<SliderAttachment>(*params.getArpOctave(), arpOctaveKnob.getSlider());
        arpOctaveKnob.getSlider().getProperties().set("paramId", params.getArpOctave()->paramID);
    }

    // Classic Warning
    addChildComponent(classicWarningLabel);
    classicWarningLabel.setJustificationType(juce::Justification::centred);
    classicWarningLabel.setColour(juce::Label::textColourId, DesignTokens::Colors::czRed);

    // Register Listener
    params.getAPVTS().addParameterListener(ParameterIDs::operationMode, this);
    updateVisibility();
}

ArpeggiatorSection::~ArpeggiatorSection()
{
    audioProcessor.getParameters().getAPVTS().removeParameterListener(ParameterIDs::operationMode, this);
}

void ArpeggiatorSection::paint(juce::Graphics& g)
{
    // Draw semi-transparent overlay if disabled in Classic mode
    auto opMode = audioProcessor.getParameters().getOperationMode();
    if (opMode && opMode->getIndex() == 0) // Classic 101
    {
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
    }
}

void ArpeggiatorSection::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == ParameterIDs::operationMode)
    {
        juce::MessageManager::callAsync([this]() {
            updateVisibility();
            repaint();
        });
    }
}

void ArpeggiatorSection::updateVisibility()
{
    auto opMode = audioProcessor.getParameters().getOperationMode();
    bool isClassic = (opMode && opMode->getIndex() == 0);
    
    // Hide controls in Classic 101 mode
    bool controlsVisible = !isClassic;
    
    arpEnableButton.setVisible(controlsVisible);
    arpLatchButton.setVisible(controlsVisible);
    arpRateKnob.setVisible(controlsVisible);
    arpBpmKnob.setVisible(controlsVisible);
    arpPatternKnob.setVisible(controlsVisible);
    arpOctaveKnob.setVisible(controlsVisible);
    arpGateKnob.setVisible(controlsVisible);
    arpSwingKnob.setVisible(controlsVisible);
    arpSwingModeCombo.setVisible(controlsVisible);
    
    rateLabel.setVisible(controlsVisible);
    tempoLabel.setVisible(controlsVisible);
    patternLabel.setVisible(controlsVisible);
    octLabel.setVisible(controlsVisible);
    gateLabel.setVisible(controlsVisible);
    swingLabel.setVisible(controlsVisible);
    typeLabel.setVisible(controlsVisible);
    
    classicWarningLabel.setVisible(isClassic);
    
    // Force layout update when visibility changes
    resized();
}

void ArpeggiatorSection::resized()
{
    auto area = getLocalBounds().reduced(20, 10);
    
    if (classicWarningLabel.isVisible())
    {
        classicWarningLabel.setFont(getScaledFont(18.0f).boldened());
        classicWarningLabel.setBounds(getLocalBounds());
        return;
    }

    // Manual Layout - Grid 2x4
    int cols = 4;
    int rows = 2;
    int cellW = area.getWidth() / cols;
    int cellH = area.getHeight() / rows;

    auto getCell = [&](int row, int col) {
        return juce::Rectangle<int>(area.getX() + col * cellW, 
                                   area.getY() + row * cellH, 
                                   cellW, cellH).reduced(5);
    };

    // Row 0
    // Col 0: Buttons
    auto btnArea = getCell(0, 0);
    int bH = juce::jmin(24, btnArea.getHeight() / 2 - 4);
    arpEnableButton.setBounds(btnArea.removeFromTop(bH + 4).reduced(10, 2));
    arpLatchButton.setBounds(btnArea.removeFromTop(bH + 4).reduced(10, 2));

    auto layoutKnob = [&](int r, int c, juce::Label& l, juce::Component& k) {
        auto cell = getCell(r, c);
        l.setBounds(cell.removeFromTop(18));
        k.setBounds(cell);
    };

    layoutKnob(0, 1, rateLabel, arpRateKnob);
    layoutKnob(0, 2, tempoLabel, arpBpmKnob);
    layoutKnob(0, 3, patternLabel, arpPatternKnob);

    // Row 1
    layoutKnob(1, 0, octLabel, arpOctaveKnob);
    layoutKnob(1, 1, gateLabel, arpGateKnob);
    
    // Swing & Mode
    auto swingArea = getCell(1, 2);
    swingLabel.setBounds(swingArea.removeFromTop(18));
    arpSwingKnob.setBounds(swingArea.removeFromTop(swingArea.getHeight() - 30));
    typeLabel.setBounds(swingArea.removeFromTop(14));
    arpSwingModeCombo.setBounds(swingArea.reduced(5, 0));
}

void ArpeggiatorSection::refreshFromAPVTS()
{
    auto& params = audioProcessor.getParameters();
    
    if (auto* p = params.getArpEnabled()) arpEnableButton.setToggleState(p->get(), juce::dontSendNotification);
    if (auto* p = params.getArpLatch()) arpLatchButton.setToggleState(p->get(), juce::dontSendNotification);
    
    if (auto* p = params.getArpRate()) arpRateKnob.getSlider().setValue((double)p->getIndex(), juce::dontSendNotification);
    if (auto* p = params.getArpBpm()) arpBpmKnob.getSlider().setValue(p->get(), juce::dontSendNotification);
    if (auto* p = params.getArpGate()) arpGateKnob.getSlider().setValue(p->get(), juce::dontSendNotification);
    if (auto* p = params.getArpSwing()) arpSwingKnob.getSlider().setValue(p->get(), juce::dontSendNotification);
    
    if (auto* p = params.getArpSwingMode()) arpSwingModeCombo.setSelectedItemIndex(p->getIndex(), juce::dontSendNotification);
    
    if (auto* p = params.getArpPattern()) arpPatternKnob.getSlider().setValue((double)p->getIndex(), juce::dontSendNotification);
    if (auto* p = params.getArpOctave()) arpOctaveKnob.getSlider().setValue((double)p->get(), juce::dontSendNotification);

    updateVisibility();
    resized(); // Recalculate layout after visibility update
}

} // namespace UI
} // namespace CZ101
