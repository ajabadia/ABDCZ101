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
    
    // Parameter Control (Proxy to all voices)
    // Oscillator 1
    void setOsc1Waveform(int waveformIndex) noexcept;
    void setOsc1Level(float level) noexcept;
    
    // Oscillator 2
    void setOsc2Waveform(int waveformIndex) noexcept;
    void setOsc2Level(float level) noexcept;
    void setOsc2Detune(float cents) noexcept;
    
    // DCW Envelope
    void setDCWAttack(float seconds) noexcept;
    void setDCWDecay(float seconds) noexcept;
    void setDCWSustain(float level) noexcept;
    void setDCWRelease(float seconds) noexcept;
    
    // DCA Envelope
    void setDCAAttack(float seconds) noexcept;
    void setDCADecay(float seconds) noexcept;
    void setDCASustain(float level) noexcept;
    void setDCARelease(float seconds) noexcept;
    
    // 8-Stage Envelope Control
    void setDCWStage(int index, float rate, float level) noexcept;
    void setDCWSustainPoint(int index) noexcept;
    void setDCWEndPoint(int index) noexcept;

    // Getters for UI
    void getDCWStage(int index, float& rate, float& level) const noexcept;
    int getDCWSustainPoint() const noexcept;
    int getDCWEndPoint() const noexcept;

    void getDCAStage(int index, float& rate, float& level) const noexcept;
    int getDCASustainPoint() const noexcept;
    int getDCAEndPoint() const noexcept;
    
    void getPitchStage(int index, float& rate, float& level) const noexcept;
    int getPitchSustainPoint() const noexcept;
    int getPitchEndPoint() const noexcept;
    
    void setDCAStage(int index, float rate, float level) noexcept;
    void setDCASustainPoint(int index) noexcept;
    void setDCAEndPoint(int index) noexcept;

    // Pitch Envelope (DCO)
    void setPitchStage(int index, float rate, float level) noexcept;
    void setPitchSustainPoint(int index) noexcept;
    void setPitchEndPoint(int index) noexcept;

    // Hard Sync
    void setHardSync(bool enabled) noexcept;

    // Ring Mod
    void setRingMod(bool enabled) noexcept;

    // Glide
    void setGlideTime(float seconds) noexcept;
    void setMasterTune(float semitones) noexcept;
    void setPitchBend(float semitones) noexcept;
    
    // LFO Control
    void setVibratoDepth(float semitones) noexcept;
    void updateLFO(float currentLFOValue) noexcept;

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
