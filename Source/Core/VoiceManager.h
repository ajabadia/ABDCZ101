#pragma once

#include "Voice.h"
#include "Voice.h"
#include "../DSP/Arpeggiator.h"
#include <vector>

namespace CZ101 {

namespace DSP { class LFO; } // Forward declaration of LFO

namespace Core {

class VoiceManager
{
public:
    static constexpr int MAX_VOICES = 16; // 8 notes * 2 lines (or 16 notes single line mode?)
    
    enum VoiceStealingMode
    {
        NONE,
        OLDEST,
        QUIETEST,
        RELEASE_PHASE
    };
    
    VoiceManager();
    
    // Audit Fix [2.2]
    void setSynthModel(DSP::MultiStageEnvelope::Model model) noexcept;
    
    // Audit Fix [2.3]
    void setVoiceLimit(int limit) noexcept;

    void setSampleRate(double sampleRate) noexcept;
    void setVoiceStealingMode(VoiceStealingMode mode) noexcept { stealingMode = mode; }
    
    // Parameter Control (Proxy to all voices)
    // Oscillator 1
    // Oscillator 1
    void setOsc1Waveforms(int firstIndex, int secondIndex) noexcept;
    void setOsc1Level(float level) noexcept;
    
    // Oscillator 2
    void setOsc2Waveforms(int firstIndex, int secondIndex) noexcept;
    void setOsc2Level(float level) noexcept;
    void setOsc2Detune(float cents) noexcept;
    void setOsc2DetuneHardware(int oct, int coarse, int fineCents) noexcept;
    
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
    void setDCWStage(int line, int index, float rate, float level) noexcept;
    void setDCWSustainPoint(int line, int index) noexcept;
    void setDCWEndPoint(int line, int index) noexcept;

    // Getters for UI
    void getDCWStage(int line, int index, float& rate, float& level) const noexcept;
    int getDCWSustainPoint(int line) const noexcept;
    int getDCWEndPoint(int line) const noexcept;

    void getDCAStage(int line, int index, float& rate, float& level) const noexcept;
    int getDCASustainPoint(int line) const noexcept;
    int getDCAEndPoint(int line) const noexcept;
    
    void getPitchStage(int line, int index, float& rate, float& level) const noexcept;
    int getPitchSustainPoint(int line) const noexcept;
    int getPitchEndPoint(int line) const noexcept;
    
    void setDCAStage(int line, int index, float rate, float level) noexcept;
    void setDCASustainPoint(int line, int index) noexcept;
    void setDCAEndPoint(int line, int index) noexcept;

    // Pitch Envelope (DCO)
    void setPitchStage(int line, int index, float rate, float level) noexcept;
    void setPitchSustainPoint(int line, int index) noexcept;
    void setPitchEndPoint(int line, int index) noexcept;

    // Hard Sync
    void setHardSync(bool enabled) noexcept;

    // Ring Mod
    void setRingMod(bool enabled) noexcept;

    // Glide
    void setGlideTime(float seconds) noexcept;
    void setMasterTune(float semitones) noexcept;
    void setMasterVolume(float level) noexcept;
    void setPitchBend(float semitones) noexcept;

    // Modern Filter
    void setFilterCutoff(float frequency) noexcept;
    void setFilterResonance(float reso) noexcept;
    void setHPF(float frequency) noexcept;
    
    // Modulation Routing
    void setModWheel(float value) noexcept;
    void setAftertouch(float value) noexcept;
    void setModulationMatrix(const Voice::ModulationMatrix& matrix) noexcept;
    
    // LFO Control
    void setVibratoDepth(float semitones) noexcept;
    void setLFOFrequency(float hz) noexcept;
    void setLFOWaveform(DSP::LFO::Waveform waveform) noexcept;
    void setLFODelay(float seconds) noexcept;

    void noteOn(int midiNote, float velocity) noexcept;
    void noteOff(int midiNote) noexcept;
    void allNotesOff() noexcept;
    
    // Audio Processing
    // LFO is now internal to Voices
    void renderNextBlock(float* outputL, float* outputR, int numSamples) noexcept;
    
    int getActiveVoiceCount() const noexcept;
    int getCurrentNote() const noexcept { return lastMidiNote; }

    DSP::Arpeggiator& getArpeggiator() { return arpeggiator; } // [NEW]

private:
    void startInternalVoice(int note, float velocity) noexcept;
    void stopInternalVoice(int note) noexcept;

    std::array<Voice, MAX_VOICES> voices; // Audit Fix 6.1: Fixed size array for memory stability
    Voice referenceVoice;      // Audit Fix 1.5: Reference voice for stable parameter reading
    DSP::Arpeggiator arpeggiator; // [NEW]
    
    // Audit Fix [2.3]: Dynamic Voice Count
    int maxActiveVoices = MAX_VOICES; 
    
    VoiceStealingMode stealingMode = RELEASE_PHASE;
    int lastMidiNote = -1;
    
    int findFreeVoice() const noexcept;
    int findVoiceToSteal() const noexcept;
    int findVoicePlayingNote(int midiNote) const noexcept;
    
    // Helper to reduce repetition
    template <typename Func>
    void applyToAllVoices(Func&& func)
    {
        for (auto& v : voices) func(v);
        func(referenceVoice);
    }
};

} // namespace Core
} // namespace CZ101
