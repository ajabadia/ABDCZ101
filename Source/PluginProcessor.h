#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Utils/PerformanceMonitor.h"
#include "Core/VoiceManager.h"
#include "MIDI/MIDIProcessor.h"
#include "MIDI/SysExManager.h"
#include "State/Parameters.h"
#include "State/PresetManager.h"
#include "DSP/Filters/ResonantFilter.h"
#include "DSP/Effects/Delay.h"
#include "DSP/Effects/Chorus.h"
#include "DSP/Effects/Reverb.h"
#include "DSP/Modulation/LFO.h"

class CZ101AudioProcessor : public juce::AudioProcessor
{
public:
    CZ101AudioProcessor();
    ~CZ101AudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    
    CZ101::State::PresetManager& getPresetManager() { return presetManager; }
    CZ101::MIDI::SysExManager& getSysExManager() { return sysExManager; }
    CZ101::MIDI::MIDIProcessor& getMidiProcessor() { return midiProcessor; }
    CZ101::State::Parameters& getParameters() { return parameters; }
    CZ101::Core::VoiceManager& getVoiceManager() { return voiceManager; }
    CZ101::Utils::PerformanceMonitor& getPerformanceMonitor() { return performanceMonitor; }

private:
    CZ101::Core::VoiceManager voiceManager;
    CZ101::MIDI::MIDIProcessor midiProcessor;
    CZ101::State::Parameters parameters;
    CZ101::State::PresetManager presetManager;
    CZ101::MIDI::SysExManager sysExManager{presetManager};
    
    CZ101::DSP::ResonantFilter filterL;
    CZ101::DSP::ResonantFilter filterR;
    CZ101::DSP::Effects::Delay delayL;
    CZ101::DSP::Effects::Delay delayR;
    
    juce::Reverb reverb;
    juce::Reverb::Parameters reverbParams;
    CZ101::DSP::Effects::Chorus chorus;
    
    // UI Update Tracking
    CZ101::DSP::LFO lfo;
    CZ101::Utils::PerformanceMonitor performanceMonitor;
    
    void updateParameters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CZ101AudioProcessor)
};
