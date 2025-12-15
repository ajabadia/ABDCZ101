#pragma once

#include "../Core/VoiceManager.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace CZ101 {
namespace MIDI {

class MIDIProcessor
{
public:
    MIDIProcessor(Core::VoiceManager& voiceManager);
    
    void processMidiMessage(const juce::MidiMessage& message) noexcept;
    // Alias for external use
    void processMessage(const juce::MidiMessage& message) { processMidiMessage(message); }
    
    void processMidiBuffer(const juce::MidiBuffer& midiBuffer) noexcept;
    
    void setPitchBendRange(int semitones) noexcept { pitchBendRange = semitones; }
    
private:
    Core::VoiceManager& voiceManager;
    int pitchBendRange = 2;  // ±2 semitones
    float currentPitchBend = 0.0f;
    
    void handleNoteOn(int note, float velocity) noexcept;
    void handleNoteOff(int note) noexcept;
    void handlePitchBend(int value) noexcept;
    void handleControlChange(int cc, int value) noexcept;
    void handleSysEx(const void* data, int size) noexcept;
};

} // namespace MIDI
} // namespace CZ101
