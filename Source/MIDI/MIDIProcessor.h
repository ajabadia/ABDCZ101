#pragma once

#include "../Core/VoiceManager.h"
#include "SysExManager.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace CZ101 {
namespace MIDI {

class MIDIProcessor
{
public:
    MIDIProcessor(Core::VoiceManager& voiceManager, State::PresetManager& presetManager);
    
    void processMidiMessage(const juce::MidiMessage& message) noexcept;
    void setSysExManager(SysExManager* sysEx) { sysExManager = sysEx; }
    
    // Alias for external use
    void processMessage(const juce::MidiMessage& message) { processMidiMessage(message); }
    void processMidiBuffer(const juce::MidiBuffer& midiBuffer) noexcept;
    
    void setPitchBendRange(int semitones) noexcept { pitchBendRange = semitones; }
    
    // Activity Tracking
    bool hasRecentActivity() const noexcept { return activityFlag; }
    void clearActivityFlag() noexcept { activityFlag = false; }

private:
    Core::VoiceManager& voiceManager;
    State::PresetManager& presetManager; 
    
    SysExManager* sysExManager = nullptr;
    int pitchBendRange = 2;  // ±2 semitones
    float currentPitchBend = 0.0f;
    bool activityFlag = false;
    
    // MIDI State
    float portamentoTime = 0.0f;
    bool portamentoEnabled = false;
    bool sustainPedalActive = false;
    std::vector<int> sustainedNotes; // Notes that need a noteOff when pedal is released
    
    // MIDI Learn
    std::string learningParamId;
    bool isLearning = false;
    std::map<int, std::string> ccMapping; // CC Number -> Parameter ID
    juce::AudioProcessorValueTreeState* apvts = nullptr; // Reference to APVTS for generic parameter setting

public:
    void setAPVTS(juce::AudioProcessorValueTreeState* state) { apvts = state; }
    void learnNextCC(const std::string& paramId);
    void unmapCC(int cc);
    void clearLearnState() { isLearning = false; learningParamId.clear(); }
    int getCCForParam(const std::string& paramId) const; // Helper for UI
    
    void handleNoteOn(int note, float velocity) noexcept;
    void handleNoteOff(int note) noexcept;
    void handlePitchBend(int value) noexcept;
    void handleControlChange(int cc, int value) noexcept;
    void handleAftertouch(float value) noexcept;
    void handleSysEx(const void* data, int size) noexcept;
};

} // namespace MIDI
} // namespace CZ101
