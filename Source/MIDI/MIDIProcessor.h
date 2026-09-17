#pragma once

#include "../Core/VoiceManager.h"
#include "SysExManager.h"
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace juce { class AudioProcessorValueTreeState; }

namespace CZ101 {
namespace MIDI {

class MIDIProcessor
{
public:
    // NOTE: The PresetManager reference was removed in the WASM-integration
    // refactor — it was stored but never used (dead dependency that prevented
    // compiling this class in the standalone WASM build).
    explicit MIDIProcessor(Core::VoiceManager& voiceManager);
    
    void processMidiMessage(const juce::MidiMessage& message) noexcept;
    void setSysExManager(SysExManager* sysEx) { sysExManager = sysEx; }
    
    // Alias for external use
    void processMessage(const juce::MidiMessage& message) { processMidiMessage(message); }
    void processMidiBuffer(const juce::MidiBuffer& midiBuffer) noexcept;
    
    void setPitchBendRange(int semitones) noexcept { pitchBendRange = semitones; }
    void setMidiChannel(int channel) noexcept { listenChannel = channel; }
    void setKeyTranspose(int semitones) noexcept { keyTranspose = semitones; }
    void setOctaveShift(int octaves) noexcept { octaveShift = octaves; }
    
    // Activity Tracking
    bool hasRecentActivity() const noexcept { return activityFlag; }
    void clearActivityFlag() noexcept { activityFlag = false; }

private:
    Core::VoiceManager& voiceManager;
    
    // Audit Fix 10.1: Lock-free callback for parameter updates
    std::function<void(const char*, float)> onMidiParamChange; 
    
public:
    void setParamChangeCallback(std::function<void(const char*, float)> cb) { onMidiParamChange = cb; }

private:
    
    SysExManager* sysExManager = nullptr;
    int pitchBendRange = 2;  // ±2 semitones
    int listenChannel = 0;   // 0 = OMNI, 1-16 = Single Channel
    int keyTranspose = 0;    // KEY_TRANSPOSE (-12..+12 semitones)
    int octaveShift = 0;      // OCTAVE from SysEx (-1..+1 = ±12 semitones)
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
