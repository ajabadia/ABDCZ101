#pragma once

#include "../DSP/Oscillators/PhaseDistOsc.h"
#include "../DSP/Envelopes/ADSREnvelope.h"

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
    
    // DCW Envelope (timbre modulation)
    void setDCWAttack(float seconds) noexcept;
    void setDCWDecay(float seconds) noexcept;
    void setDCWSustain(float level) noexcept;
    void setDCWRelease(float seconds) noexcept;
    
    // DCA Envelope (amplitude)
    void setDCAAttack(float seconds) noexcept;
    void setDCADecay(float seconds) noexcept;
    void setDCASustain(float level) noexcept;
    void setDCARelease(float seconds) noexcept;
    
    // Rendering
    float renderNextSample() noexcept;
    
    bool isActive() const noexcept { return dcaEnvelope.isActive(); }
    int getCurrentNote() const noexcept { return currentNote; }
    
private:
    // Oscillators
    DSP::PhaseDistOscillator osc1;
    DSP::PhaseDistOscillator osc2;
    
    // Envelopes
    DSP::ADSREnvelope dcwEnvelope;  // Digital Controlled Wave (timbre)
    DSP::ADSREnvelope dcaEnvelope;  // Digital Controlled Amplifier (volume)
    
    // State
    int currentNote = -1;
    float currentVelocity = 1.0f;
    
    // Mix levels
    float osc1Level = 0.5f;
    float osc2Level = 0.5f;
    float osc2Detune = 0.0f;
    
    // Helper
    float midiNoteToFrequency(int midiNote) const noexcept;
};

} // namespace Core
} // namespace CZ101
