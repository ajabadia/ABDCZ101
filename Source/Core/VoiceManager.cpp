#include "VoiceManager.h"
#include "../DSP/Modulation/LFO.h"
#include <algorithm>

namespace CZ101 {
namespace Core {

VoiceManager::VoiceManager()
{
    // voices array is fixed size now
}

void VoiceManager::setSampleRate(double sampleRate) noexcept
{
    applyToAllVoices([sampleRate](Voice& v) { v.setSampleRate(sampleRate); });
    referenceVoice.setSampleRate(sampleRate); // Audit Fix: Initialize reference voice to prevent div-by-zero
}

void VoiceManager::setOsc1Waveforms(int fIdx, int sIdx) noexcept
{
    auto f = static_cast<DSP::PhaseDistOscillator::CzWaveform>(fIdx);
    auto s = static_cast<DSP::PhaseDistOscillator::CzWaveform>(sIdx);
    applyToAllVoices([f, s](Voice& v) { v.setOsc1Waveforms(f, s); });
}

void VoiceManager::setOsc1Level(float level) noexcept
{
    applyToAllVoices([level](Voice& v) { v.setOsc1Level(level); });
}

void VoiceManager::setOsc2Waveforms(int fIdx, int sIdx) noexcept
{
    auto f = static_cast<DSP::PhaseDistOscillator::CzWaveform>(fIdx);
    auto s = static_cast<DSP::PhaseDistOscillator::CzWaveform>(sIdx);
    applyToAllVoices([f, s](Voice& v) { v.setOsc2Waveforms(f, s); });
}

void VoiceManager::setOsc2Level(float level) noexcept
{
    applyToAllVoices([level](Voice& v) { v.setOsc2Level(level); });
}

void VoiceManager::setOsc2Detune(float cents) noexcept { applyToAllVoices([cents](Voice& v) { v.setOsc2Detune(cents); }); }

void VoiceManager::setOsc2DetuneHardware(int oct, int coarse, int fineCents) noexcept 
{ 
    applyToAllVoices([oct, coarse, fineCents](Voice& v) { v.setOsc2DetuneHardware(oct, coarse, fineCents); }); 
}

void VoiceManager::setDCWAttack(float s) noexcept { applyToAllVoices([s](Voice& v){ v.setDCWAttack(s); }); }
void VoiceManager::setDCWDecay(float s) noexcept { applyToAllVoices([s](Voice& v){ v.setDCWDecay(s); }); }
void VoiceManager::setDCWSustain(float l) noexcept { applyToAllVoices([l](Voice& v){ v.setDCWSustain(l); }); }
void VoiceManager::setDCWRelease(float s) noexcept { applyToAllVoices([s](Voice& v){ v.setDCWRelease(s); }); }

void VoiceManager::setDCAAttack(float s) noexcept { applyToAllVoices([s](Voice& v){ v.setDCAAttack(s); }); }
void VoiceManager::setDCADecay(float s) noexcept { applyToAllVoices([s](Voice& v){ v.setDCADecay(s); }); }
void VoiceManager::setDCASustain(float l) noexcept { applyToAllVoices([l](Voice& v){ v.setDCASustain(l); }); }
void VoiceManager::setDCARelease(float s) noexcept { applyToAllVoices([s](Voice& v){ v.setDCARelease(s); }); }

// 8-Stage Control
// 8-Stage Control
void VoiceManager::setDCWStage(int line, int idx, float r, float l) noexcept { applyToAllVoices([=](Voice& v){ v.setDCWStage(line, idx, r, l); }); }
void VoiceManager::setDCWSustainPoint(int line, int idx) noexcept { applyToAllVoices([=](Voice& v){ v.setDCWSustainPoint(line, idx); }); }
void VoiceManager::setDCWEndPoint(int line, int idx) noexcept { applyToAllVoices([=](Voice& v){ v.setDCWEndPoint(line, idx); }); }

void VoiceManager::setDCAStage(int line, int idx, float r, float l) noexcept { applyToAllVoices([=](Voice& v){ v.setDCAStage(line, idx, r, l); }); }
void VoiceManager::setDCASustainPoint(int line, int idx) noexcept { applyToAllVoices([=](Voice& v){ v.setDCASustainPoint(line, idx); }); }
void VoiceManager::setDCAEndPoint(int line, int idx) noexcept { applyToAllVoices([=](Voice& v){ v.setDCAEndPoint(line, idx); }); }

void VoiceManager::setPitchStage(int line, int idx, float r, float l) noexcept { applyToAllVoices([=](Voice& v){ v.setPitchStage(line, idx, r, l); }); }
void VoiceManager::setPitchSustainPoint(int line, int idx) noexcept { applyToAllVoices([=](Voice& v){ v.setPitchSustainPoint(line, idx); }); }
void VoiceManager::setPitchEndPoint(int line, int idx) noexcept { applyToAllVoices([=](Voice& v){ v.setPitchEndPoint(line, idx); }); }

// Getters 
void VoiceManager::getDCWStage(int line, int idx, float& r, float& l) const noexcept { referenceVoice.getDCWStage(line, idx, r, l); }
int VoiceManager::getDCWSustainPoint(int line) const noexcept { return referenceVoice.getDCWSustainPoint(line); }
int VoiceManager::getDCWEndPoint(int line) const noexcept { return referenceVoice.getDCWEndPoint(line); }

void VoiceManager::getDCAStage(int line, int idx, float& r, float& l) const noexcept { referenceVoice.getDCAStage(line, idx, r, l); }
int VoiceManager::getDCASustainPoint(int line) const noexcept { return referenceVoice.getDCASustainPoint(line); }
int VoiceManager::getDCAEndPoint(int line) const noexcept { return referenceVoice.getDCAEndPoint(line); }

void VoiceManager::getPitchStage(int line, int idx, float& r, float& l) const noexcept { referenceVoice.getPitchStage(line, idx, r, l); }
int VoiceManager::getPitchSustainPoint(int line) const noexcept { return referenceVoice.getPitchSustainPoint(line); }
int VoiceManager::getPitchEndPoint(int line) const noexcept { return referenceVoice.getPitchEndPoint(line); }

void VoiceManager::setHardSync(bool e) noexcept { applyToAllVoices([e](Voice& v){ v.setHardSync(e); }); }
void VoiceManager::setRingMod(bool e) noexcept { applyToAllVoices([e](Voice& v){ v.setRingMod(e); }); }
void VoiceManager::setGlideTime(float s) noexcept { applyToAllVoices([s](Voice& v){ v.setGlideTime(s); }); }
void VoiceManager::setMasterTune(float s) noexcept { applyToAllVoices([s](Voice& v){ v.setMasterTune(s); }); }
void VoiceManager::setMasterVolume(float l) noexcept { applyToAllVoices([l](Voice& v){ v.setMasterVolume(l); }); }
void VoiceManager::setPitchBend(float s) noexcept { applyToAllVoices([s](Voice& v){ v.setPitchBend(s); }); }

void VoiceManager::setFilterCutoff(float f) noexcept { applyToAllVoices([f](Voice& v){ v.setFilterCutoff(f); }); }
void VoiceManager::setFilterResonance(float r) noexcept { applyToAllVoices([r](Voice& v){ v.setFilterResonance(r); }); }
void VoiceManager::setHPF(float f) noexcept { applyToAllVoices([f](Voice& v){ v.setHPF(f); }); }

void VoiceManager::setModWheel(float value) noexcept { applyToAllVoices([value](Voice& v){ v.setModWheel(value); }); }
void VoiceManager::setAftertouch(float value) noexcept { applyToAllVoices([value](Voice& v){ v.setAftertouch(value); }); }
void VoiceManager::setModulationMatrix(const Voice::ModulationMatrix& matrix) noexcept { applyToAllVoices([&matrix](Voice& v){ v.setModulationMatrix(matrix); }); }

void VoiceManager::setVibratoDepth(float s) noexcept { applyToAllVoices([s](Voice& v){ v.setVibratoDepth(s); }); }
void VoiceManager::setLFOFrequency(float hz) noexcept { applyToAllVoices([hz](Voice& v){ v.setLFOFrequency(hz); }); }
void VoiceManager::setLFOWaveform(DSP::LFO::Waveform w) noexcept { applyToAllVoices([w](Voice& v){ v.setLFOWaveform(w); }); }
void VoiceManager::setLFODelay(float s) noexcept { applyToAllVoices([s](Voice& v){ v.setLFODelay(s); }); }

// Renamed internal helper
void VoiceManager::startInternalVoice(int midiNote, float velocity) noexcept
{
    lastMidiNote = midiNote;
    int voiceIndex = findVoicePlayingNote(midiNote);
    if (voiceIndex < 0) voiceIndex = findFreeVoice();
    if (voiceIndex < 0) voiceIndex = findVoiceToSteal();
    if (voiceIndex >= 0) voices[voiceIndex].noteOn(midiNote, velocity);
}

void VoiceManager::noteOn(int midiNote, float velocity) noexcept
{
    if (arpeggiator.isEnabled()) {
        arpeggiator.noteOn(midiNote, velocity);
    } else {
        startInternalVoice(midiNote, velocity);
    }
}

// Renamed internal helper
void VoiceManager::stopInternalVoice(int midiNote) noexcept
{
    for (auto& voice : voices)
        if (voice.isActive() && voice.getCurrentNote() == midiNote)
            voice.noteOff();
}

void VoiceManager::noteOff(int midiNote) noexcept
{
    if (arpeggiator.isEnabled()) {
        arpeggiator.noteOff(midiNote);
    } else {
        stopInternalVoice(midiNote);
    }
}

void VoiceManager::allNotesOff() noexcept { for (auto& voice : voices) voice.noteOff(); }

void VoiceManager::renderNextBlock(float* outputL, float* outputR, int numSamples) noexcept
{
    // Arpeggiator Processing
    if (arpeggiator.isEnabled()) {
        // Static buffer to reuse memory
        // Thread safety: This is only called from Audio Thread
        static std::vector<DSP::Arpeggiator::ArpEvent> events;
        events.clear();
        arpeggiator.process(numSamples, events);
        
        for (const auto& evt : events) {
            if (evt.isNoteOn) startInternalVoice(evt.note, evt.velocity);
            else stopInternalVoice(evt.note);
        }
    }

    for (int i = 0; i < numSamples; ++i)
    {
        float sample = 0.0f;
        for (auto& voice : voices)
            if (voice.isActive())
                sample += voice.renderNextSample();
        
        outputL[i] = sample;
        outputR[i] = sample;
    }
}

int VoiceManager::getActiveVoiceCount() const noexcept
{
    int count = 0;
    for (const auto& voice : voices) if (voice.isActive()) ++count;
    return count;
}

int VoiceManager::findFreeVoice() const noexcept
{
    for (size_t i = 0; i < voices.size(); ++i) if (!voices[i].isActive()) return static_cast<int>(i);
    return -1;
}

int VoiceManager::findVoiceToSteal() const noexcept
{
    for (size_t i = 0; i < voices.size(); ++i) if (voices[i].isReleasing()) return static_cast<int>(i);
    for (size_t i = 0; i < voices.size(); ++i) if (voices[i].isActive()) return static_cast<int>(i);
    return 0;
}

int VoiceManager::findVoicePlayingNote(int midiNote) const noexcept
{
    for (size_t i = 0; i < voices.size(); ++i) if (voices[i].getCurrentNote() == midiNote) return static_cast<int>(i);
    return -1;
}

} // namespace Core
} // namespace CZ101
