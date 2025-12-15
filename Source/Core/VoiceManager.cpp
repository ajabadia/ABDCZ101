#include "VoiceManager.h"
#include <algorithm>

namespace CZ101 {
namespace Core {

VoiceManager::VoiceManager()
{
}

void VoiceManager::setSampleRate(double sampleRate) noexcept
{
    for (auto& voice : voices)
        voice.setSampleRate(sampleRate);
}

void VoiceManager::noteOn(int midiNote, float velocity) noexcept
{
    int voiceIndex = findFreeVoice();
    
    if (voiceIndex < 0)
        voiceIndex = findVoiceToSteal();
    
    if (voiceIndex >= 0)
        voices[voiceIndex].noteOn(midiNote, velocity);
}

void VoiceManager::noteOff(int midiNote) noexcept
{
    int voiceIndex = findVoicePlayingNote(midiNote);
    if (voiceIndex >= 0)
        voices[voiceIndex].noteOff();
}

void VoiceManager::allNotesOff() noexcept
{
    for (auto& voice : voices)
        voice.noteOff();
}

void VoiceManager::renderNextBlock(float* outputL, float* outputR, int numSamples) noexcept
{
    for (int i = 0; i < numSamples; ++i)
    {
        float sample = 0.0f;
        for (auto& voice : voices)
            sample += voice.renderNextSample();
        
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
    for (int i = 0; i < MAX_VOICES; ++i)
        if (!voices[i].isActive())
            return i;
    return -1;
}

int VoiceManager::findVoiceToSteal() const noexcept
{
    // Simple: steal oldest (first active voice)
    for (int i = 0; i < MAX_VOICES; ++i)
        if (voices[i].isActive())
            return i;
    return 0;
}

int VoiceManager::findVoicePlayingNote(int midiNote) const noexcept
{
    for (int i = 0; i < MAX_VOICES; ++i)
        if (voices[i].getCurrentNote() == midiNote)
            return i;
    return -1;
}

} // namespace Core
} // namespace CZ101
