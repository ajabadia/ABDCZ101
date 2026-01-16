#include "MIDIProcessor.h"
#include <algorithm>

namespace CZ101 {
namespace MIDI {

MIDIProcessor::MIDIProcessor(Core::VoiceManager& vm, State::PresetManager& pm)
    : voiceManager(vm), presetManager(pm)
{
    // Audit Fix [B]: Pre-allocate sustained notes vector to avoid reallocation in audio thread
    sustainedNotes.reserve(128); 
}

// Helper to get raw string from std::string
// (Since map uses std::string but JUCE uses String, we convert)
void MIDIProcessor::learnNextCC(const std::string& paramId)
{
    isLearning = true;
    learningParamId = paramId;
}

void MIDIProcessor::unmapCC(int cc)
{
    ccMapping.erase(cc);
}

int MIDIProcessor::getCCForParam(const std::string& paramId) const
{
    for (const auto& pair : ccMapping)
    {
        if (pair.second == paramId) return pair.first;
    }
    return -1;
}

void MIDIProcessor::processMidiMessage(const juce::MidiMessage& message) noexcept
{
    activityFlag = true;
    
    if (message.isNoteOn())
        handleNoteOn(message.getNoteNumber(), message.getFloatVelocity());
    else if (message.isNoteOff())
        handleNoteOff(message.getNoteNumber());
    else if (message.isPitchWheel())
        handlePitchBend(message.getPitchWheelValue());
    else if (message.isController())
        handleControlChange(message.getControllerNumber(), message.getControllerValue());
    else if (message.isChannelPressure())
        handleAftertouch(message.getChannelPressureValue() / 127.0f);
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
    // If note is already in sustained list, remove it (retrigger)
    auto it = std::find(sustainedNotes.begin(), sustainedNotes.end(), note);
    if (it != sustainedNotes.end()) sustainedNotes.erase(it);

    voiceManager.noteOn(note, velocity);
}

void MIDIProcessor::handleNoteOff(int note) noexcept
{
    if (sustainPedalActive)
    {
        // Add to sustained notes if not already there
        if (std::find(sustainedNotes.begin(), sustainedNotes.end(), note) == sustainedNotes.end())
            sustainedNotes.push_back(note);
    }
    else
    {
        voiceManager.noteOff(note);
    }
}

void MIDIProcessor::handlePitchBend(int value) noexcept
{
    float normalized = (value - 8192) / 8192.0f;
    currentPitchBend = normalized * (float)pitchBendRange;
    voiceManager.setPitchBend(currentPitchBend);
}

void MIDIProcessor::handleControlChange(int cc, int value) noexcept
{
    float normValue = value / 127.0f;
    
    // 1. MIDI Learn Logic
    if (isLearning)
    {
        if (!learningParamId.empty()) {
            ccMapping[cc] = learningParamId;
            isLearning = false;
            learningParamId.clear();
        }
        return; // Don't process the CC that was just learned
    }
    
    // 2. Mapped Parameter Control
    if (apvts && ccMapping.count(cc))
    {
        auto paramId = ccMapping[cc];
        // Must find parameter and set value.
        // NOTE: setParameterNotifyingHost needs normalized value usually, 
        // but APVTS params range might differ. 
        // Best way: getRawParameterValue gives primitive, getParameter() gives RangedAudioParameter.
        // We'll use the parameter object to convert.
        auto* param = apvts->getParameter(juce::String(paramId));
        if (param) 
        {
             // We need to allow this to run on audio thread safely?
             // beginChange/setValueNotifyingHost/endChange is standard but not RT safe if it locks.
             // For now, we assume direct normalized set is acceptable or we use a queue.
             // Actually, setValueNotifyingHost IS the standard way for automation.
             param->setValueNotifyingHost(normValue);
        }
        return; // Override default behavior if mapped
    }

    // 3. Default Hardcoded Behavior (Legacy)
    switch (cc)
    {
        case 1: // Vibrato Depth / Mod Wheel
            if (apvts) if (auto* p = apvts->getParameter("LFO_DEPTH")) p->setValueNotifyingHost(normValue);
            voiceManager.setModWheel(normValue);
            break;

        case 5: // Portamento Time
            if (apvts) if (auto* p = apvts->getParameter("GLIDE")) p->setValueNotifyingHost(normValue);
            break;

        case 6: // Master Tune (Data Entry)
            voiceManager.setMasterTune((value - 64) / 64.0f);
            break;

        case 7: // Volume
            if (apvts) if (auto* p = apvts->getParameter("MASTER_VOLUME")) p->setValueNotifyingHost(normValue);
            break;

        case 10: // Pan
            // Not mapped to APVTS yet
            break;

        case 64: // Sustain Pedal
        {
            bool active = (value >= 64);
            if (sustainPedalActive && !active) // Pedal released
            {
                for (int note : sustainedNotes)
                    voiceManager.noteOff(note);
                sustainedNotes.clear();
            }
            sustainPedalActive = active;
            break;
        }

        case 65: // Portamento On/Off
            // For Portamento, we don't have a bool parameter in APVTS for ON/OFF, 
            // but we can set the time to 0 if off. 
            // Better: just let the user use CC 5.
            break;

        case 71: // "Resonance" -> Modern LPF Reso
            if (apvts) if (auto* p = apvts->getParameter("MODERN_LPF_RESO")) p->setValueNotifyingHost(normValue);
            break;

        case 74: // "Cutoff" -> Modern LPF Cutoff
            if (apvts) if (auto* p = apvts->getParameter("MODERN_LPF_CUTOFF")) p->setValueNotifyingHost(normValue);
            break;

        case 120: // All Sound Off
        case 123: // All Notes Off
            voiceManager.allNotesOff();
            sustainedNotes.clear();
            break;
            
        default:
            break;
    }
}

void MIDIProcessor::handleAftertouch(float value) noexcept
{
    voiceManager.setAftertouch(value);
}

void MIDIProcessor::handleSysEx(const void* data, int size) noexcept
{
    if (sysExManager)
        // Pass the RAW data including F0/F7 for robust buffering
        sysExManager->handleSysEx(data, size, "MIDI Input");
}

} // namespace MIDI
} // namespace CZ101
