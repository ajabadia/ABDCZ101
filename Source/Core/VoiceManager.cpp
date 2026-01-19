#include "VoiceManager.h"
#include "CZ5000VoiceStrategy.h"
#include "AudioThreadSnapshot.h" // Required for parameter snapshot definition
#include "../DSP/Modulation/LFO.h"
#include <algorithm>

namespace CZ101 {
namespace Core {

VoiceManager::VoiceManager()
{
    // voices array is fixed size now
    // Default Strategy
    updateStrategy();
}

#include "CZ5000VoiceStrategy.h" // [NEW]

void VoiceManager::updateStrategy()
{
    // [NEW] Phase 4.3: Initialize Strategy
    // Initialize with default (Oldest)
    strategy = std::make_unique<OldestVoiceStrategy>();
}

// Audit Fix [2.2]: Hardware Model Selection
void VoiceManager::setSynthModel(DSP::MultiStageEnvelope::Model model) noexcept
{
    applyToAllVoices([model](Voice& v) { v.setModel(model); });
    
    // Switch strategy based on model
    if (model == DSP::MultiStageEnvelope::Model::CZ5000) {
        strategy = std::make_unique<CZ5000VoiceStrategy>();
    } else {
        strategy = std::make_unique<OldestVoiceStrategy>();
    }
    
    // Audit Fix [2.3]: Dynamic Voice Count
    
    // Audit Fix [2.3]: Dynamic Voice Count
    // CZ-101: 4 Voices (8 DCOs)
    // CZ-5000: 8 Voices (16 DCOs)
    // Modern: MAX_VOICES (16)
    // Note: The Model enum only tracks envelope behavior. We need to infer the voice limit
    // based on the model or if we are in "Modern" mode which uses CZ5000 envelopes but higher poly.
    // However, setSynthModel is called with CZ5000 for both CZ-5000 and Modern.
    // We should rely on PluginProcessor to set this up correctly, OR infer it.
    // Ideally, PluginProcessor should pass the max voices explicitely or we set it here mapping 
    // CZ101 -> 4, CZ5000 -> 8. But Modern uses CZ5000 model but wants 16 voices.
    // For now, let's strictly map the Hardware Models to their original polyphony.
    
    // NOTE: PluginProcessor calling logic:
    // Classic 101 -> Model::CZ101
    // Classic 5000 -> Model::CZ5000
    // Modern -> Model::CZ5000
    // This makes it ambiguous here.
    
    // REVISION: We need a separate setter for Voice Count or pass it in here.
    // Current strategy: default to MAX. 
    // Better strategy for Phase 2.3: Add setMaxVoices(int) or deduce.
    // Let's deduce:
    // If Model::CZ101 -> 4 voices
    // If Model::CZ5000 -> 8 voices
    // If Modern override -> 16 voices (handled by PluginProcessor calling setMaxVoices separately?)
    // No, PluginProcessor isn't calling setMaxVoices.
    
    // Let's modify logic: 
    // 101 -> 4 voices.
    // 5000 -> 8 voices.
    // Modern -> We need to know if it's Modern.
    
    // Given the previous step in PluginProcessor:
    // "Classic 5000" and "Modern" both map to Model::CZ5000 envelope behavior.
    // But they have different polyphony.
    // We need to support `setVoiceLimit(int)` and call it from PluginProcessor.
    
    if (model == DSP::MultiStageEnvelope::Model::CZ101) maxActiveVoices = 4;
    else maxActiveVoices = 8; // Default for CZ-5000 hardware
}

// [NEW] Helper for PluginProcessor to override limit (e.g. for Modern Mode)
void VoiceManager::setVoiceLimit(int limit) noexcept
{
    maxActiveVoices = std::min(limit, (int)MAX_VOICES);
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

// Phase 5.1: Oversampling
void VoiceManager::setOversamplingFactor(int factor) noexcept { applyToAllVoices([factor](Voice& v){ v.setOversamplingFactor(factor); }); }

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
    // Fix: Only stop voices within the active range?
    for (int i = 0; i < maxActiveVoices; ++i)
        if (voices[i].isActive() && voices[i].getCurrentNote() == midiNote)
            voices[i].noteOff();
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
        // Limit processing to maxActiveVoices
        for (int v = 0; v < maxActiveVoices; ++v)
            if (voices[v].isActive())
                sample += voices[v].renderNextSample();
        
        outputL[i] = sample;
        outputR[i] = sample;
    }
}

int VoiceManager::getActiveVoiceCount() const noexcept
{
    int count = 0;
    for (int i = 0; i < maxActiveVoices; ++i) 
        if (voices[i].isActive()) ++count;
    return count;
}

int VoiceManager::findFreeVoice() const noexcept
{
    for (int i = 0; i < maxActiveVoices; ++i) 
        if (!voices[i].isActive()) return i;
    return -1;
}

int VoiceManager::findVoiceToSteal() const noexcept
{
    // 1. First Pass: Try to find a releasing voice
    for (int i = 0; i < maxActiveVoices; ++i) 
        if (voices[i].isReleasing()) return i;

    // 2. Second Pass: Use Strategy (e.g. Oldest)
    if (strategy)
    {
        return strategy->findVoiceToSteal(voices.data(), (int)voices.size(), maxActiveVoices);
    }
    
    // Fallback if no strategy (should not happen if initialized)
    for (int i = 0; i < maxActiveVoices; ++i) 
        if (voices[i].isActive()) return i;
        
    return 0;
}

int VoiceManager::findVoicePlayingNote(int midiNote) const noexcept
{
    for (int i = 0; i < maxActiveVoices; ++i) 
        if (voices[i].getCurrentNote() == midiNote) return i;
    return -1;
}

// Phase 7: Snapshot System
void VoiceManager::applySnapshot(const ParameterSnapshot* snapshot) noexcept
{
    if (!snapshot) return;
    
    // Global parameters affecting logic
    maxActiveVoices = snapshot->system.voiceLimit;
    
    // Propagate to all voices
    applyToAllVoices([snapshot](Voice& v) { 
        v.applySnapshot(snapshot); 
    });
    
    // Audit Fix [2.4]: Apply Arpeggiator Snapshot
    auto& arp = getArpeggiator();
    arp.setEnabled(snapshot->arp.enabled);
    arp.setLatch(snapshot->arp.latch);
    arp.setRate(static_cast<DSP::Arpeggiator::Rate>(snapshot->arp.rate));
    arp.setPattern(static_cast<DSP::Arpeggiator::Pattern>(snapshot->arp.pattern));
    arp.setOctaveRange(snapshot->arp.octave);
    arp.setGateTime(snapshot->arp.gate);
    arp.setSwing(snapshot->arp.swing);
    arp.setSwingMode(static_cast<DSP::Arpeggiator::SwingMode>(snapshot->arp.swingMode));
}

} // namespace Core
} // namespace CZ101
