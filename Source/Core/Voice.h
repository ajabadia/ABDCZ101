#pragma once

#include "../DSP/Oscillators/PhaseDistOsc.h"
#include "../DSP/Envelopes/MultiStageEnv.h"

namespace CZ101 {
namespace Core {

/**
 * @brief Voice - Complete synthesizer voice
 * 
 * Integrates oscillators and envelopes to create the CZ-101 sound.
 * Architecture: DCO (oscillators) → DCW (timbre envelope) → DCA (amplitude envelope)
 */
class Voice
{
public:
    Voice();
    
    void setSampleRate(double sampleRate) noexcept;
    
    // Note control
    void noteOn(int midiNote, float velocity) noexcept;
    void noteOff() noexcept;
    void reset() noexcept;
    
    // Oscillator 1 parameters
    void setOsc1Waveform(DSP::PhaseDistOscillator::Waveform waveform) noexcept;
    void setOsc1Level(float level) noexcept;
    
    // Oscillator 2 parameters
    void setOsc2Waveform(DSP::PhaseDistOscillator::Waveform waveform) noexcept;
    void setOsc2Level(float level) noexcept;
    void setOsc2Detune(float cents) noexcept;  // -100 to +100 cents
    
    /**
     * @brief Enable/Disable Hard Sync (Osc2 resets when Osc1 wraps)
     */
    void setHardSync(bool enabled) noexcept;

    /**
     * @brief Enable/Disable Ring Modulation (Osc2 output = Osc1 * Osc2)
     */
    /**
     * @brief Enable/Disable Ring Modulation (Osc2 output = Osc1 * Osc2)
     */
    void setRingMod(bool enabled) noexcept;

    /**
     * @brief Set Glide (Portamento) Time in seconds
     * @param seconds Slide time (0.0 to ~2.0)
     */
    /**
     * @brief Set Glide (Portamento) Time in seconds
     * @param seconds Slide time (0.0 to ~2.0)
     */
    void setGlideTime(float seconds) noexcept;

    // --- Modulation (LFO) ---
    void setVibratoDepth(float semitones) noexcept;
    void setLFOValue(float value) noexcept; // -1.0 to 1.0
    
    // --- Global Pitch ---
    void setPitchBend(float semitones) noexcept;
    void setMasterTune(float semitones) noexcept;

    // --- Pitch Envelope Controls ---
    
    // DCW Envelope (Legacy ADSR Wrappers)
    void setDCWAttack(float seconds) noexcept;
    void setDCWDecay(float seconds) noexcept;
    void setDCWSustain(float level) noexcept;
    void setDCWRelease(float seconds) noexcept;
    
    // DCW 8-Stage Control
    void setDCWStage(int index, float rate, float level) noexcept;
    void setDCWSustainPoint(int index) noexcept;
    void setDCWEndPoint(int index) noexcept;
    
    void getDCWStage(int index, float& rate, float& level) const noexcept;
    int getDCWSustainPoint() const noexcept;
    int getDCWEndPoint() const noexcept;
    
    // DCA Envelope (Legacy ADSR Wrappers)
    void setDCAAttack(float seconds) noexcept;
    void setDCADecay(float seconds) noexcept;
    void setDCASustain(float level) noexcept;
    void setDCARelease(float seconds) noexcept;

    // DCA 8-Stage Control
    void setDCAStage(int index, float rate, float level) noexcept;
    void setDCASustainPoint(int index) noexcept;
    void setDCAEndPoint(int index) noexcept;

    void getDCAStage(int index, float& rate, float& level) const noexcept;
    int getDCASustainPoint() const noexcept;
    int getDCAEndPoint() const noexcept;

    // Pitch Envelope (DCO) 8-Stage Control
    void setPitchStage(int index, float rate, float level) noexcept;
    void setPitchSustainPoint(int index) noexcept;
    void setPitchEndPoint(int index) noexcept;
    
    void getPitchStage(int index, float& rate, float& level) const noexcept;
    int getPitchSustainPoint() const noexcept;
    int getPitchEndPoint() const noexcept;
    
    // Rendering
    float renderNextSample() noexcept;
    
    bool isActive() const noexcept { return dcaEnvelope.isActive(); }
    int getCurrentNote() const noexcept { return currentNote; }
    
private:
    // Oscillators
    DSP::PhaseDistOscillator osc1;
    DSP::PhaseDistOscillator osc2;
    
    // Envelopes
    DSP::MultiStageEnvelope dcwEnvelope;  // Digital Controlled Wave (timbre)
    DSP::MultiStageEnvelope dcaEnvelope;  // Digital Controlled Amplifier (volume)
    DSP::MultiStageEnvelope pitchEnvelope; // DCO Pitch Envelope (new)
    
    // State
    int currentNote = -1;
    float currentVelocity = 1.0f;
    
    // Mix levels
    float osc1Level = 0.5f;
    float osc2Level = 0.5f;
    float osc2Detune = 0.0f;
    
    // Pitch Modulation State (Optimization)
    float baseFrequency = 440.0f;
    float currentDetuneFactor = 1.0f;
    
    bool isHardSyncEnabled = false;
    bool isRingModEnabled = false;
    
    float glideTime = 0.0f;
    float currentFrequency = 440.0f;
    float targetFrequency = 440.0f;
    
    // LFO State
    float vibratoDepth = 0.0f;
    float lfoValue = 0.0f;
    
    // Pitch Bend
    float pitchBendFactor = 1.0f;
    float masterTuneFactor = 1.0f;
    
    // Helper
    float midiNoteToFrequency(int midiNote) const noexcept;
};

} // namespace Core
} // namespace CZ101
