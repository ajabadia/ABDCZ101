#include "OscillatorSection.h"
#include "../SkinManager.h"

namespace CZ101 {
namespace UI {

OscillatorSection::OscillatorSection(CZ101AudioProcessor& p)
    : audioProcessor(p),
      osc1LevelKnob("Lvl 1"),
      osc2LevelKnob("Lvl 2"),
      osc2DetuneKnob("Detune")
{
    auto& params = audioProcessor.getParameters();
    auto& palette = SkinManager::getInstance().getCurrentPalette();

    // === DCO 1 ===
    addAndMakeVisible(osc1WaveSelector);
    osc1WaveSelector.addItemList( { "Sawtooth", "Square", "Pulse", "Double Sine", "Saw-Pulse", "Resonance 1", "Resonance 2", "Resonance 3" }, 1 );
    if (params.getOsc1Waveform())
        osc1WaveAttachment = std::make_unique<ComboBoxAttachment>(*params.getOsc1Waveform(), osc1WaveSelector);
    
    addAndMakeVisible(osc1LevelKnob);
    if (params.getOsc1Level()) {
        osc1LevelAttachment = std::make_unique<SliderAttachment>(*params.getOsc1Level(), osc1LevelKnob.getSlider());
        osc1LevelKnob.getSlider().getProperties().set("paramId", params.getOsc1Level()->paramID);
    }

    // === DCO 2 ===
    addAndMakeVisible(osc2WaveSelector);
    osc2WaveSelector.addItemList( { "Sawtooth", "Square", "Pulse", "Double Sine", "Saw-Pulse", "Resonance 1", "Resonance 2", "Resonance 3" }, 1 );
    if (params.getOsc2Waveform())
        osc2WaveAttachment = std::make_unique<ComboBoxAttachment>(*params.getOsc2Waveform(), osc2WaveSelector);

    addAndMakeVisible(osc2LevelKnob);
    if (params.getOsc2Level()) {
        osc2LevelAttachment = std::make_unique<SliderAttachment>(*params.getOsc2Level(), osc2LevelKnob.getSlider());
        osc2LevelKnob.getSlider().getProperties().set("paramId", params.getOsc2Level()->paramID);
    }

    addAndMakeVisible(osc2DetuneKnob);
    if (params.getOsc2Detune()) {
        osc2DetuneAttachment = std::make_unique<SliderAttachment>(*params.getOsc2Detune(), osc2DetuneKnob.getSlider());
        osc2DetuneKnob.getSlider().getProperties().set("paramId", params.getOsc2Detune()->paramID);
    }

    // === Shared Controls ===
    addAndMakeVisible(hardSyncButton);
    hardSyncButton.setButtonText("Hard Sync");
    hardSyncButton.setClickingTogglesState(true);
    hardSyncButton.setColour(juce::TextButton::buttonOnColourId, palette.accentCyan);
    if (params.getHardSync())
        hardSyncAttachment = std::make_unique<ButtonAttachment>(*params.getHardSync(), hardSyncButton);

    addAndMakeVisible(ringModButton);
    ringModButton.setButtonText("Ring Mod");
    ringModButton.setClickingTogglesState(true);
    ringModButton.setColour(juce::TextButton::buttonOnColourId, palette.accentCyan);
    if (params.getRingMod())
        ringModAttachment = std::make_unique<ButtonAttachment>(*params.getRingMod(), ringModButton);

}

OscillatorSection::~OscillatorSection() {}

void OscillatorSection::paint(juce::Graphics& g) {
    auto& palette = SkinManager::getInstance().getCurrentPalette();
    
    // Draw Section Background
    g.fillAll(palette.sectionBackground);

    g.setFont(getScaledFont(14.0f).boldened());
    g.setColour(palette.textPrimary); // Changed to textPrimary for better contrast
    
    auto bounds = getLocalBounds().toFloat();
    g.drawText("OSCILLATORS (DCO)", bounds.removeFromTop(25), juce::Justification::centred, false);
}

void OscillatorSection::resized()
{
    auto area = getLocalBounds().reduced((int)(4 * getUiScale()));
    area.removeFromTop((int)(18 * getUiScale())); // Smaller title space

    auto dcoArea = area.removeFromTop(area.getHeight() * 0.75f);
    auto sharedArea = area;

    auto dco1Area = dcoArea.removeFromLeft(dcoArea.getWidth() / 2).reduced(2);
    auto dco2Area = dcoArea.reduced(2);

    // --- DCO 1 ---
    juce::FlexBox f1;
    float scale = getUiScale();
    f1.flexDirection = juce::FlexBox::Direction::column;
    f1.items.add(juce::FlexItem(osc1WaveSelector).withHeight(24 * scale).withMargin(2 * scale));
    f1.items.add(juce::FlexItem(osc1LevelKnob).withFlex(1.0f));
    f1.performLayout(dco1Area);

    // --- DCO 2 ---
    juce::FlexBox f2;
    f2.flexDirection = juce::FlexBox::Direction::column;
    f2.items.add(juce::FlexItem(osc2WaveSelector).withHeight(24 * scale).withMargin(2 * scale));
    
    juce::FlexBox f2k;
    f2k.items.add(juce::FlexItem(osc2LevelKnob).withFlex(1));
    f2k.items.add(juce::FlexItem(osc2DetuneKnob).withFlex(1));
    f2.items.add(juce::FlexItem(f2k).withFlex(1.0f));
    f2.performLayout(dco2Area);

    // --- Shared ---
    juce::FlexBox fs;
    fs.alignItems = juce::FlexBox::AlignItems::center;
    fs.items.add(juce::FlexItem(hardSyncButton).withFlex(1).withHeight(24 * scale).withMargin(2 * scale));
    fs.items.add(juce::FlexItem(ringModButton).withFlex(1).withHeight(24 * scale).withMargin(2 * scale));
    fs.performLayout(sharedArea);
}

} // namespace UI
} // namespace CZ101
