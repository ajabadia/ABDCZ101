#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "../State/PresetManager.h"

namespace CZ101 {
namespace MIDI {

class SysExManager
{
public:
    SysExManager(State::PresetManager& presetManager);
    
    // Main entry point for incoming SysEx data
    // Option to provide a name (e.g. from filename)
    void handleSysEx(const void* data, int size, juce::String presetName = "");
    
    // Create a dump message for a specific preset
    juce::MidiMessage createDumpMessage(const State::Preset& preset);
    
    // Callback when a preset is parsed successfully
    std::function<void(const State::Preset&)> onPresetParsed;

private:
    State::PresetManager& presetManager;
    
    // Constants
    static constexpr juce::uint8 SYSEX_START = 0xF0;
    static constexpr juce::uint8 SYSEX_END = 0xF7;
    static constexpr juce::uint8 MANU_ID_CASIO = 0x44;
    
    // Helpers for CZ Nibble format
    juce::uint8 decodeByte(juce::uint8 lowNibble, juce::uint8 highNibble) const;
    
    // Parameter Mapping
    void parseToneData(const juce::uint8* payload, int payloadSize);
    
    // Parameter Setters helpers
    float mapRangeTo01(int value, int min, int max);
    float mapRateToSeconds(int rate); // CZ 0-99 to seconds
};

} // namespace MIDI
} // namespace CZ101
