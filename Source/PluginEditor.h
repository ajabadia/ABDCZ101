#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

class CZ101AudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer,
                                   public juce::AudioProcessorValueTreeState::Listener,
                                   public CZ101::State::PresetManager::Listener
{
public:
    CZ101AudioProcessorEditor(CZ101AudioProcessor&);
    ~CZ101AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;

    CZ101AudioProcessor& getAudioProcessor() { return audioProcessor; }

    // APVTS Listener Override
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // Preset Listener Override
    void presetLoaded(int index) override;
    void bankUpdated() override;
    void presetRenamed(int index, const std::string& newName) override;

private:
    void timerCallback() override;
    
    // WebBrowserComponent bindings and callbacks
    void setupWebBrowserBindings();
    
    CZ101AudioProcessor& audioProcessor;
    
    std::unique_ptr<juce::WebBrowserComponent> webComponent;
    
    // We will use a flag to prevent echoing parameters back and forth
    std::atomic<bool> isUpdatingFromWeb{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CZ101AudioProcessorEditor)
};
