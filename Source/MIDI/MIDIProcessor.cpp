#include "MIDIProcessor.h"

namespace CZ101 {
namespace MIDI {

MIDIProcessor::MIDIProcessor(Core::VoiceManager& vm, State::PresetManager& pm)
    : voiceManager(vm), presetManager(pm)
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
    // CZ-101 Spec: 8 bit resolution, 0-12 semitones.
    float normalized = (value - 8192) / 8192.0f;
    currentPitchBend = normalized * pitchBendRange;
    
    // Apply pitch bend to all voices
    voiceManager.setPitchBend(currentPitchBend);
}

void MIDIProcessor::handleControlChange(int cc, int value) noexcept
{
    float normValue = value / 127.0f;
    
    switch (cc)
    {
        case 1: // Vibrato On/Off (We map to Depth)
            // If value > 64 ? Depth = 0.1 : 0.0?
            // User requested mapping, let's map 0-127 to 0.0-1.0 depth (approx 1 semitone max)
            voiceManager.setVibratoDepth(normValue * 1.0f); 
            break;
            
        case 5: // Portamento Time
            portamentoTime = normValue * 2.0f; // Max 2 seconds
            if (portamentoEnabled)
                voiceManager.setGlideTime(portamentoTime);
            break;
            
        case 6: // Master Tune
            // Map 0-127 to +/- 1 semitone? Or +/- 100 cents?
            // Let's do +/- 100 cents (+/- 1 semitone)
            // Center 64 = 0.
            {
                float tune = (value - 64) / 64.0f; // -1 to +1 approx
                voiceManager.setMasterTune(tune);
            }
            break;
            
        case 65: // Portamento On/Off
            portamentoEnabled = (value >= 64);
            voiceManager.setGlideTime(portamentoEnabled ? portamentoTime : 0.0f);
            break;
            
        default:
            break;
    }
}

void MIDIProcessor::handleSysEx(const void* data, int size) noexcept
{
    if (sysExManager)
        sysExManager->handleSysEx(data, size, "SysEx Import");
}

} // namespace MIDI
} // namespace CZ101
