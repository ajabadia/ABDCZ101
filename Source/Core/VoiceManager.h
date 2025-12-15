#pragma once

#include "Voice.h"
#include <array>

namespace CZ101 {
namespace Core {

class VoiceManager
{
public:
    static constexpr int MAX_VOICES = 8;
    
    enum VoiceStealingMode
    {
        OLDEST,
        QUIETEST,
        RELEASE_PHASE
    };
    
    VoiceManager();
    
    void setSampleRate(double sampleRate) noexcept;
    void setVoiceStealingMode(VoiceStealingMode mode) noexcept { stealingMode = mode; }
    
    void noteOn(int midiNote, float velocity) noexcept;
    void noteOff(int midiNote) noexcept;
    void allNotesOff() noexcept;
    
    void renderNextBlock(float* outputL, float* outputR, int numSamples) noexcept;
    
    int getActiveVoiceCount() const noexcept;
    
private:
    std::array<Voice, MAX_VOICES> voices;
    VoiceStealingMode stealingMode = RELEASE_PHASE;
    
    int findFreeVoice() const noexcept;
    int findVoiceToSteal() const noexcept;
    int findVoicePlayingNote(int midiNote) const noexcept;
};

} // namespace Core
} // namespace CZ101
