#include "MIDIProcessor.h"

namespace CZ101 {
namespace MIDI {

MIDIProcessor::MIDIProcessor(Core::VoiceManager& vm)
    : voiceManager(vm)
{
}

void MIDIProcessor::processMidiMessage(const juce::MidiMessage& message) noexcept
{
    if (message.isNoteOn())
        handleNoteOn(message.getNoteNumber(), message.getFloatVelocity());
    else if (message.isNoteOff())
        handleNoteOff(message.getNoteNumber());
    else if (message.isPitchWheel())
        handlePitchBend(message.getPitchWheelValue());
    else if (message.isControllerOfType(1))
        handleControlChange(1, message.getControllerValue());
    else if (message.isSysEx())
        handleSysEx(message.getSysExData(), message.getSysExDataSize());
}

void MIDIProcessor::processMidiBuffer(const juce::MidiBuffer& midiBuffer) noexcept
{
    for (const auto metadata : midiBuffer)
        processMidiMessage(metadata.getMessage());
}

void MIDIProcessor::handleNoteOn(int note, float velocity) noexcept
{
    voiceManager.noteOn(note, velocity);
}

void MIDIProcessor::handleNoteOff(int note) noexcept
{
    voiceManager.noteOff(note);
}

void MIDIProcessor::handlePitchBend(int value) noexcept
{
    // Convert 0-16383 to -1.0 to +1.0
    currentPitchBend = ((value - 8192) / 8192.0f) * pitchBendRange;
    // TODO: Apply to all active voices
}

void MIDIProcessor::handleControlChange(int cc, int value) noexcept
{
    // TODO: Map CCs to parameters
    juce::ignoreUnused(cc, value);
}

void MIDIProcessor::handleSysEx(const void* data, int size) noexcept
{
    // TODO: Implement SysEx bank decoding
    // Standard CZ-101 SysEx starts with F0 44 00 00 ...
    juce::ignoreUnused(data, size);
}

} // namespace MIDI
} // namespace CZ101
