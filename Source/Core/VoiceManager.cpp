#include "VoiceManager.h"
#include "../DSP/Modulation/LFO.h"
#include <algorithm>

namespace CZ101 {
namespace Core {

VoiceManager::VoiceManager()
{
    voices.resize(MAX_VOICES);
}

void VoiceManager::setSampleRate(double sampleRate) noexcept
{
    for (auto& voice : voices)
        voice.setSampleRate(sampleRate);
}

// Optimization: Use a macro or template if this gets too repetitive, 
// but for clarity we'll implement explicit loopers.

void VoiceManager::setOsc1Waveform(int waveformIndex) noexcept
{
    auto w = static_cast<DSP::PhaseDistOscillator::Waveform>(waveformIndex);
    for (auto& voice : voices) voice.setOsc1Waveform(w);
}

void VoiceManager::setOsc1Level(float level) noexcept
{
    for (auto& voice : voices) voice.setOsc1Level(level);
}

void VoiceManager::setOsc2Waveform(int waveformIndex) noexcept
{
    auto w = static_cast<DSP::PhaseDistOscillator::Waveform>(waveformIndex);
    for (auto& voice : voices) voice.setOsc2Waveform(w);
}

void VoiceManager::setOsc2Level(float level) noexcept
{
    for (auto& voice : voices) voice.setOsc2Level(level);
}

void VoiceManager::setOsc2Detune(float cents) noexcept
{
    for (auto& voice : voices) voice.setOsc2Detune(cents);
}

void VoiceManager::setDCWAttack(float seconds) noexcept { for (auto& v : voices) v.setDCWAttack(seconds); }
void VoiceManager::setDCWDecay(float seconds) noexcept { for (auto& v : voices) v.setDCWDecay(seconds); }
void VoiceManager::setDCWSustain(float level) noexcept { for (auto& v : voices) v.setDCWSustain(level); }
void VoiceManager::setDCWRelease(float seconds) noexcept { for (auto& v : voices) v.setDCWRelease(seconds); }

void VoiceManager::setDCAAttack(float seconds) noexcept { for (auto& v : voices) v.setDCAAttack(seconds); }
void VoiceManager::setDCADecay(float seconds) noexcept { for (auto& v : voices) v.setDCADecay(seconds); }
void VoiceManager::setDCASustain(float level) noexcept { for (auto& v : voices) v.setDCASustain(level); }
void VoiceManager::setDCARelease(float seconds) noexcept { for (auto& v : voices) v.setDCARelease(seconds); }

// 8-Stage Control
void VoiceManager::setDCWStage(int index, float rate, float level) noexcept { for (auto& v : voices) v.setDCWStage(index, rate, level); }
void VoiceManager::setDCWSustainPoint(int index) noexcept { for (auto& v : voices) v.setDCWSustainPoint(index); }
void VoiceManager::setDCWEndPoint(int index) noexcept { for (auto& v : voices) v.setDCWEndPoint(index); }

void VoiceManager::setDCAStage(int index, float rate, float level) noexcept { for (auto& v : voices) v.setDCAStage(index, rate, level); }
void VoiceManager::setDCASustainPoint(int index) noexcept { for (auto& v : voices) v.setDCASustainPoint(index); }
void VoiceManager::setDCAEndPoint(int index) noexcept { for (auto& v : voices) v.setDCAEndPoint(index); }

// Pitch Envelope
void VoiceManager::setPitchStage(int index, float rate, float level) noexcept { for (auto& v : voices) v.setPitchStage(index, rate, level); }
void VoiceManager::setPitchSustainPoint(int index) noexcept { for (auto& v : voices) v.setPitchSustainPoint(index); }
void VoiceManager::setPitchEndPoint(int index) noexcept { for (auto& v : voices) v.setPitchEndPoint(index); }

void VoiceManager::getDCWStage(int index, float& rate, float& level) const noexcept { voices[0].getDCWStage(index, rate, level); }
int VoiceManager::getDCWSustainPoint() const noexcept { return voices[0].getDCWSustainPoint(); }
int VoiceManager::getDCWEndPoint() const noexcept { return voices[0].getDCWEndPoint(); }

void VoiceManager::getDCAStage(int index, float& rate, float& level) const noexcept { voices[0].getDCAStage(index, rate, level); }
int VoiceManager::getDCASustainPoint() const noexcept { return voices[0].getDCASustainPoint(); }
int VoiceManager::getDCAEndPoint() const noexcept { return voices[0].getDCAEndPoint(); }

void VoiceManager::getPitchStage(int index, float& rate, float& level) const noexcept { voices[0].getPitchStage(index, rate, level); }
int VoiceManager::getPitchSustainPoint() const noexcept { return voices[0].getPitchSustainPoint(); }
int VoiceManager::getPitchEndPoint() const noexcept { return voices[0].getPitchEndPoint(); }

void VoiceManager::setHardSync(bool enabled) noexcept { for (auto& v : voices) v.setHardSync(enabled); }
void VoiceManager::setRingMod(bool enabled) noexcept { for (auto& v : voices) v.setRingMod(enabled); }
void VoiceManager::setGlideTime(float seconds) noexcept { for (auto& v : voices) v.setGlideTime(seconds); }
void VoiceManager::setMasterTune(float semitones) noexcept { for (auto& v : voices) v.setMasterTune(semitones); }
void VoiceManager::setPitchBend(float semitones) noexcept { for (auto& v : voices) v.setPitchBend(semitones); }
void VoiceManager::setVibratoDepth(float semitones) noexcept { for (auto& v : voices) v.setVibratoDepth(semitones); }
void VoiceManager::updateLFO(float val) noexcept { for (auto& v : voices) v.setLFOValue(val); }

void VoiceManager::noteOn(int midiNote, float velocity) noexcept
{
    lastMidiNote = midiNote;

    // Check if any voice is already playing this note (e.g. still in release phase)
    // If so, steal it (monophonic retrigger per key) to prevent duplicate voices for same note
    int voiceIndex = findVoicePlayingNote(midiNote);
    
    if (voiceIndex < 0)
        voiceIndex = findFreeVoice();
    
    if (voiceIndex < 0)
        voiceIndex = findVoiceToSteal();
    
    if (voiceIndex >= 0)
        voices[voiceIndex].noteOn(midiNote, velocity);
}

void VoiceManager::noteOff(int midiNote) noexcept
{
    // Safely turn off ALL voices playing this note (just in case multiple exist)
    for (auto& voice : voices)
    {
        if (voice.isActive() && voice.getCurrentNote() == midiNote)
        {
            voice.noteOff();
        }
    }
}

void VoiceManager::allNotesOff() noexcept
{
    for (auto& voice : voices)
        voice.noteOff();
}


void VoiceManager::renderNextBlock(float* outputL, float* outputR, int numSamples, DSP::LFO* lfo) noexcept
{
    // Local LFO caching variables if we didn't want to call getNextValue per sample for optimization, 
    // but the request is specific: LFO IS broken because it's block rate.
    
    for (int i = 0; i < numSamples; ++i)
    {
        // Update LFO if provided
        if (lfo)
        {
            float lfoVal = lfo->getNextValue();
            for (auto& voice : voices)
                voice.setLFOValue(lfoVal); // Direct setter to avoid vtable overhead if possible, but here virtual
                // Actually updateLFO was iterating voices. Here we do it per sample.
                // Optim: This is O(N_Voices * N_Samples). 64 voices * 44100 = 2.8M iterations/sec. 
                // Acceptable for modern CPU.
        }
        
        float sample = 0.0f;
        for (auto& voice : voices)
        {
            if (voice.isActive()) // Optimization: Only render active voices
                sample += voice.renderNextSample();
        }
        
        outputL[i] = sample;
        outputR[i] = sample;
    }
}

int VoiceManager::getActiveVoiceCount() const noexcept
{
    int count = 0;
    for (const auto& voice : voices)
        if (voice.isActive())
            ++count;
    return count;
}

int VoiceManager::findFreeVoice() const noexcept
{
    for (size_t i = 0; i < voices.size(); ++i)
        if (!voices[i].isActive())
            return static_cast<int>(i);
    return -1;
}

int VoiceManager::findVoiceToSteal() const noexcept
{
    // Simple: steal first active voice (oldest)
    for (size_t i = 0; i < voices.size(); ++i)
        if (voices[i].isActive())
            return static_cast<int>(i);
    return 0;
}

int VoiceManager::findVoicePlayingNote(int midiNote) const noexcept
{
    for (size_t i = 0; i < voices.size(); ++i)
        if (voices[i].getCurrentNote() == midiNote)
            return static_cast<int>(i);
    return -1;
}

} // namespace Core
} // namespace CZ101