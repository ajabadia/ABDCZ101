#include "Voice.h"
#include <cmath>
#include <algorithm>

namespace CZ101 {
namespace Core {

Voice::Voice()
{
    // Default envelope settings
    // DCA (amplitude): Fast attack, medium release
    dcaEnvelope.setAttackTime(0.01f);
    dcaEnvelope.setDecayTime(0.1f);
    dcaEnvelope.setSustainLevel(0.7f);
    dcaEnvelope.setReleaseTime(0.2f);
    
    // DCW (timbre): Slower attack for evolving sound
    dcwEnvelope.setAttackTime(0.05f);
    dcwEnvelope.setDecayTime(0.2f);
    dcwEnvelope.setSustainLevel(0.5f);
    dcwEnvelope.setReleaseTime(0.3f);
}

void Voice::setSampleRate(double sampleRate) noexcept
{
    osc1.setSampleRate(sampleRate);
    osc2.setSampleRate(sampleRate);
    dcwEnvelope.setSampleRate(sampleRate);
    dcaEnvelope.setSampleRate(sampleRate);
}

void Voice::noteOn(int midiNote, float velocity) noexcept
{
    currentNote = midiNote;
    currentVelocity = velocity;
    
    // Calculate frequency
    float frequency = midiNoteToFrequency(midiNote);
    
    // Set oscillator frequencies
    osc1.setFrequency(frequency);
    
    // Osc2 with detune
    float detuneFactor = std::pow(2.0f, osc2Detune / 1200.0f);  // cents to ratio
    osc2.setFrequency(frequency * detuneFactor);
    
    // Trigger envelopes
    dcwEnvelope.noteOn();
    dcaEnvelope.noteOn();
    
    // Reset oscillator phases for consistent attack
    osc1.reset();
    osc2.reset();
}

void Voice::noteOff() noexcept
{
    dcwEnvelope.noteOff();
    dcaEnvelope.noteOff();
}

void Voice::reset() noexcept
{
    osc1.reset();
    osc2.reset();
    dcwEnvelope.reset();
    dcaEnvelope.reset();
    currentNote = -1;
}

void Voice::setOsc1Waveform(DSP::PhaseDistOscillator::Waveform waveform) noexcept
{
    osc1.setWaveform(waveform);
}

void Voice::setOsc1Level(float level) noexcept
{
    osc1Level = std::clamp(level, 0.0f, 1.0f);
}

void Voice::setOsc2Waveform(DSP::PhaseDistOscillator::Waveform waveform) noexcept
{
    osc2.setWaveform(waveform);
}

void Voice::setOsc2Level(float level) noexcept
{
    osc2Level = std::clamp(level, 0.0f, 1.0f);
}

void Voice::setOsc2Detune(float cents) noexcept
{
    osc2Detune = std::clamp(cents, -100.0f, 100.0f);
}

void Voice::setDCWAttack(float seconds) noexcept
{
    dcwEnvelope.setAttackTime(seconds);
}

void Voice::setDCWDecay(float seconds) noexcept
{
    dcwEnvelope.setDecayTime(seconds);
}

void Voice::setDCWSustain(float level) noexcept
{
    dcwEnvelope.setSustainLevel(level);
}

void Voice::setDCWRelease(float seconds) noexcept
{
    dcwEnvelope.setReleaseTime(seconds);
}

void Voice::setDCAAttack(float seconds) noexcept
{
    dcaEnvelope.setAttackTime(seconds);
}

void Voice::setDCADecay(float seconds) noexcept
{
    dcaEnvelope.setDecayTime(seconds);
}

void Voice::setDCASustain(float level) noexcept
{
    dcaEnvelope.setSustainLevel(level);
}

void Voice::setDCARelease(float seconds) noexcept
{
    dcaEnvelope.setReleaseTime(seconds);
}

float Voice::renderNextSample() noexcept
{
    if (!isActive())
        return 0.0f;
    
    // Get envelope values
    float dcwValue = dcwEnvelope.getNextValue();
    float dcaValue = dcaEnvelope.getNextValue();
    
    // Render oscillators with DCW (Timbre Modulation)
    // DCW value [0.0-1.0] controls the Phase Distortion amount
    float osc1Sample = osc1.renderNextSample(dcwValue);
    float osc2Sample = osc2.renderNextSample(dcwValue);
    
    // Mix oscillators
    float mixed = (osc1Sample * osc1Level) + (osc2Sample * osc2Level);
    
    // DCW is now correctly applied inside the oscillator (Phase Distortion)
    // No external amplitude modulation needed for DCW
    
    // Apply DCA (amplitude envelope)
    mixed *= dcaValue;
    
    // Apply velocity
    mixed *= currentVelocity;
    
    return mixed;
}

float Voice::midiNoteToFrequency(int midiNote) const noexcept
{
    // MIDI note 69 = A4 = 440 Hz
    // Formula: f = 440 * 2^((n-69)/12)
    return 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);
}

} // namespace Core
} // namespace CZ101
