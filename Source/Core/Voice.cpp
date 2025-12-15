#include <JuceHeader.h>
#include "Voice.h"
#include <cmath>

namespace CZ101 {
namespace Core {

Voice::Voice()
{
}

void Voice::setSampleRate(double sr) noexcept
{
    osc1.setSampleRate(sr);
    osc2.setSampleRate(sr);
    dcwEnvelope.setSampleRate(sr);
    dcaEnvelope.setSampleRate(sr);
    pitchEnvelope.setSampleRate(sr);
}

void Voice::noteOn(int midiNote, float velocity) noexcept
{
    // Frequency
    currentNote = midiNote;
    currentVelocity = velocity;
    
    // Convert note
    baseFrequency = midiNoteToFrequency(midiNote);
    targetFrequency = baseFrequency;
    currentFrequency = baseFrequency; // Start at note (glide handles transition)
    
    // Reset phase if hard sync or just standard gate behavior
    // CZ-101 resets on note on usually
    osc1.reset();
    osc2.reset();
    
    // Envelopes
    dcwEnvelope.noteOn();
    dcaEnvelope.noteOn();
    pitchEnvelope.noteOn();
}

void Voice::noteOff() noexcept
{
    dcwEnvelope.noteOff();
    dcaEnvelope.noteOff();
    pitchEnvelope.noteOff();
}

void Voice::reset() noexcept
{
    dcwEnvelope.reset();
    dcaEnvelope.reset();
    pitchEnvelope.reset();
}

// ... Oscillators ...

void Voice::setOsc1Waveform(DSP::PhaseDistOscillator::Waveform w) noexcept { osc1.setWaveform(w); }
void Voice::setOsc1Level(float level) noexcept { osc1Level = level; }

void Voice::setOsc2Waveform(DSP::PhaseDistOscillator::Waveform w) noexcept { osc2.setWaveform(w); }
void Voice::setOsc2Level(float level) noexcept { osc2Level = level; }
void Voice::setOsc2Detune(float cents) noexcept 
{ 
    osc2Detune = cents; 
    currentDetuneFactor =std::pow(2.0f, cents / 1200.0f);
}

void Voice::setHardSync(bool enabled) noexcept { isHardSyncEnabled = enabled; }
void Voice::setRingMod(bool enabled) noexcept { isRingModEnabled = enabled; }
void Voice::setGlideTime(float seconds) noexcept { glideTime = seconds; }

// ... DCW Envelope ...
void Voice::setVibratoDepth(float semitones) noexcept { vibratoDepth = semitones; }
void Voice::setLFOValue(float val) noexcept { lfoValue = val; }
void Voice::setPitchBend(float semitones) noexcept { pitchBendFactor = std::pow(2.0f, semitones / 12.0f); }
void Voice::setMasterTune(float semitones) noexcept { masterTuneFactor = std::pow(2.0f, semitones / 12.0f); }

// ... DCW Envelope ...

void Voice::setDCWAttack(float s) noexcept { juce::ignoreUnused(s); }
void Voice::setDCWDecay(float s) noexcept { juce::ignoreUnused(s); }
void Voice::setDCWSustain(float l) noexcept { juce::ignoreUnused(l); }
void Voice::setDCWRelease(float s) noexcept { juce::ignoreUnused(s); }

void Voice::setDCWStage(int i, float r, float l) noexcept { dcwEnvelope.setStage(i, r, l); }
void Voice::setDCWSustainPoint(int i) noexcept { dcwEnvelope.setSustainPoint(i); }
void Voice::setDCWEndPoint(int i) noexcept { dcwEnvelope.setEndPoint(i); }

void Voice::getDCWStage(int i, float& r, float& l) const noexcept { r = dcwEnvelope.getStageRate(i); l = dcwEnvelope.getStageLevel(i); }
int Voice::getDCWSustainPoint() const noexcept { return dcwEnvelope.getSustainPoint(); }
int Voice::getDCWEndPoint() const noexcept { return dcwEnvelope.getEndPoint(); }

// ... DCA Envelope ...

void Voice::setDCAAttack(float s) noexcept { }
void Voice::setDCADecay(float s) noexcept { }
void Voice::setDCASustain(float l) noexcept { }
void Voice::setDCARelease(float s) noexcept { }

void Voice::setDCAStage(int i, float r, float l) noexcept { dcaEnvelope.setStage(i, r, l); }
void Voice::setDCASustainPoint(int i) noexcept { dcaEnvelope.setSustainPoint(i); }
void Voice::setDCAEndPoint(int i) noexcept { dcaEnvelope.setEndPoint(i); }

void Voice::getDCAStage(int i, float& r, float& l) const noexcept { r = dcaEnvelope.getStageRate(i); l = dcaEnvelope.getStageLevel(i); }
int Voice::getDCASustainPoint() const noexcept { return dcaEnvelope.getSustainPoint(); }
int Voice::getDCAEndPoint() const noexcept { return dcaEnvelope.getEndPoint(); }

// ... Pitch Envelope ...

void Voice::setPitchStage(int i, float r, float l) noexcept { pitchEnvelope.setStage(i, r, l); }
void Voice::setPitchSustainPoint(int i) noexcept { pitchEnvelope.setSustainPoint(i); }
void Voice::setPitchEndPoint(int i) noexcept { pitchEnvelope.setEndPoint(i); }

void Voice::getPitchStage(int i, float& r, float& l) const noexcept { r = pitchEnvelope.getStageRate(i); l = pitchEnvelope.getStageLevel(i); }
int Voice::getPitchSustainPoint() const noexcept { return pitchEnvelope.getSustainPoint(); }
int Voice::getPitchEndPoint() const noexcept { return pitchEnvelope.getEndPoint(); }

float Voice::renderNextSample() noexcept
{
    if (!dcaEnvelope.isActive()) return 0.0f;
    
    // Get Envelope Values
    float dcwValue = dcwEnvelope.getNextValue(); // Timbre
    float dcaValue = dcaEnvelope.getNextValue(); // Amp
    float pitchEnvVal = pitchEnvelope.getNextValue(); // Pitch Mod (0-1)
    
    // ... Processing Logic (Same as before) ...
    
    // Pitch Modulation
    // 0.0 -> -1 Octave, 0.5 -> Unison, 1.0 -> +1 Octave
    // Modulate Pitch (Envelope 0..1  ->  +/- 50 Semitones)
    // 0.5 = No Mod. 1.0 = +50 semi. 0.0 = -50 semi.
    float semitones = (pitchEnvVal - 0.5f) * 100.0f;
    float pitchMod = std::pow(2.0f, semitones / 12.0f); 

    // Glide Processing
    if (glideTime > 0.001f && currentFrequency != targetFrequency)
    {
        // Simple 1-pole lowpass
        // 4.0f scale factor to make "1.0" feel like approx 1 sec at 44.1k
        float alpha = 1.0f / (44100.0f * (glideTime + 0.001f)); 
        float diff = targetFrequency - currentFrequency;
        currentFrequency += diff * alpha * 4.0f; 
        
        // Snap if close
        if (std::abs(diff) < 0.1f) currentFrequency = targetFrequency;
    }
    else
    {
        currentFrequency = targetFrequency;
    }
    
    // Apply LFO (Vibrato)
    float vibratoMod = 1.0f;
    if (vibratoDepth > 0.001f)
    {
        // LFO value is -1 to 1. scaling to semitones
        float lfoSemitones = lfoValue * vibratoDepth;
        vibratoMod = std::pow(2.0f, lfoSemitones / 12.0f);
    }
    
    // Apply modulated frequency to Base (currentFrequency)
    // Combine Pitch Envelope (pitchMod), Vibrato (vibratoMod), Pitch Bend, and Master Tune
    float finalFreq = currentFrequency * pitchMod * vibratoMod * pitchBendFactor * masterTuneFactor;
    osc1.setFrequency(finalFreq);
    osc2.setFrequency(finalFreq * currentDetuneFactor);
    
    bool osc1Wrapped = false;
    float osc1Sample = osc1.renderNextSample(dcwValue, &osc1Wrapped);
    
    // Hard Sync Logic: If Osc1 wrapped, reset Osc2
    if (isHardSyncEnabled && osc1Wrapped)
    {
        osc2.reset();
    }
    
    // Osc 2
    float osc2Sample = osc2.renderNextSample(dcwValue);
    
    // Ring Mod?
    if (isRingModEnabled)
    {
        // Ring mod: Osc2 is modulated by Osc1
        // Usually, Ring Mod replaces Osc2 output, or mixes.
        // In CZ: Ring Mod output replaces standard Osc2.
        // Osc2 Output = Osc1 * Osc2
        osc2Sample = osc1Sample * osc2Sample;
    }
    
    // Mix
    float mix = (osc1Sample * osc1Level) + (osc2Sample * osc2Level);
    
    // Apply VCA (DCA) and Velocity
    return mix * dcaValue * currentVelocity;
}

float Voice::midiNoteToFrequency(int midiNote) const noexcept
{
    return 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);
}

} // namespace Core
} // namespace CZ101
