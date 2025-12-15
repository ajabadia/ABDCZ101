#include <JuceHeader.h>
#include "Voice.h"
#include <cmath>
#include "../DSP/Envelopes/ADSRtoStage.h"

namespace CZ101 {
namespace Core {

Voice::Voice()
{
}

void Voice::setSampleRate(double sr) noexcept
{
    sampleRate = sr;
    osc1.setSampleRate(sr);
    osc2.setSampleRate(sr);
    dcwEnvelope.setSampleRate(sr);
    dcaEnvelope.setSampleRate(sr);
    pitchEnvelope.setSampleRate(sr);

    // Re-calculate envelopes with new sample rate
    updateDCWEnvelopeFromADSR();
    updateDCAEnvelopeFromADSR();
    updatePitchEnvelopeFromADSR();
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

// ============================================================================
// DCW ENVELOPE - IMPLEMENTACIÓN MEJORADA
// ============================================================================

void Voice::setDCWAttack(float seconds) noexcept
{
    double ms = static_cast<double>(seconds) * 1000.0;
    dcwADSR.attackMs = static_cast<float>(std::clamp(ms, 0.5, 8000.0));
    updateDCWEnvelopeFromADSR();
}

void Voice::setDCWDecay(float seconds) noexcept
{
    double ms = static_cast<double>(seconds) * 1000.0;
    dcwADSR.decayMs = static_cast<float>(std::clamp(ms, 0.5, 8000.0));
    updateDCWEnvelopeFromADSR();
}

void Voice::setDCWSustain(float level) noexcept
{
    dcwADSR.sustainLevel = std::clamp(level, 0.0f, 1.0f);
    updateDCWEnvelopeFromADSR();
}

void Voice::setDCWRelease(float seconds) noexcept
{
    double ms = static_cast<double>(seconds) * 1000.0;
    dcwADSR.releaseMs = static_cast<float>(std::clamp(ms, 0.5, 8000.0));
    updateDCWEnvelopeFromADSR();
}

void Voice::setDCWStage(int i, float r, float l) noexcept { dcwEnvelope.setStage(i, r, l); }
void Voice::setDCWSustainPoint(int i) noexcept { dcwEnvelope.setSustainPoint(i); }
void Voice::setDCWEndPoint(int i) noexcept { dcwEnvelope.setEndPoint(i); }

void Voice::getDCWStage(int i, float& r, float& l) const noexcept { r = dcwEnvelope.getStageRate(i); l = dcwEnvelope.getStageLevel(i); }
int Voice::getDCWSustainPoint() const noexcept { return dcwEnvelope.getSustainPoint(); }
int Voice::getDCWEndPoint() const noexcept { return dcwEnvelope.getEndPoint(); }

// ============================================================================
// DCA ENVELOPE - IMPLEMENTACIÓN MEJORADA
// ============================================================================

void Voice::setDCAAttack(float seconds) noexcept
{
    double ms = static_cast<double>(seconds) * 1000.0;
    dcaADSR.attackMs = static_cast<float>(std::clamp(ms, 0.5, 8000.0));
    updateDCAEnvelopeFromADSR();
}

void Voice::setDCADecay(float seconds) noexcept
{
    double ms = static_cast<double>(seconds) * 1000.0;
    dcaADSR.decayMs = static_cast<float>(std::clamp(ms, 0.5, 8000.0));
    updateDCAEnvelopeFromADSR();
}

void Voice::setDCASustain(float level) noexcept
{
    dcaADSR.sustainLevel = std::clamp(level, 0.0f, 1.0f);
    updateDCAEnvelopeFromADSR();
}

void Voice::setDCARelease(float seconds) noexcept
{
    double ms = static_cast<double>(seconds) * 1000.0;
    dcaADSR.releaseMs = static_cast<float>(std::clamp(ms, 0.5, 8000.0));
    updateDCAEnvelopeFromADSR();
}

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

// ===== HELPER METHODS FOR ADSR CONSISTENCY =====

void Voice::updateDCWEnvelopeFromADSR() noexcept
{
    std::array<float, 8> rates, levels;
    int sus, end;
    
    DSP::ADSRtoStageConverter::convertADSR(
        dcwADSR.attackMs,
        dcwADSR.decayMs,
        dcwADSR.sustainLevel,
        dcwADSR.releaseMs,
        rates, levels, sus, end,
        sampleRate
    );
    
    // Aplicar todos los stages a la vez
    for (int i = 0; i < 4; ++i) {
        dcwEnvelope.setStage(i, rates[i], levels[i]);
    }
    dcwEnvelope.setSustainPoint(sus);
    dcwEnvelope.setEndPoint(end);
}

void Voice::updateDCAEnvelopeFromADSR() noexcept
{
    std::array<float, 8> rates, levels;
    int sus, end;
    
    DSP::ADSRtoStageConverter::convertADSR(
        dcaADSR.attackMs,
        dcaADSR.decayMs,
        dcaADSR.sustainLevel,
        dcaADSR.releaseMs,
        rates, levels, sus, end,
        sampleRate
    );
    
    for (int i = 0; i < 4; ++i) {
        dcaEnvelope.setStage(i, rates[i], levels[i]);
    }
    dcaEnvelope.setSustainPoint(sus);
    dcaEnvelope.setEndPoint(end);
}

void Voice::updatePitchEnvelopeFromADSR() noexcept
{
    std::array<float, 8> rates, levels;
    int sus, end;
    
    DSP::ADSRtoStageConverter::convertADSR(
        pitchADSR.attackMs,
        pitchADSR.decayMs,
        pitchADSR.sustainLevel,
        pitchADSR.releaseMs,
        rates, levels, sus, end,
        sampleRate
    );
    
    for (int i = 0; i < 4; ++i) {
        pitchEnvelope.setStage(i, rates[i], levels[i]);
    }
    pitchEnvelope.setSustainPoint(sus);
    pitchEnvelope.setEndPoint(end);
}

float Voice::renderNextSample() noexcept
{
    if (!dcaEnvelope.isActive()) return 0.0f;
    
    // === ENVELOPE VALUES ===
    float dcwValue = dcwEnvelope.getNextValue();         // Timbre (0-1)
    float dcaValue = dcaEnvelope.getNextValue();         // Amplitud (0-1)
    float pitchEnvVal = pitchEnvelope.getNextValue();    // Pitch mod (0-1)
    
    // === PITCH MODULATION ===
    // Pitch envelope: 0.0 = -1 octava, 0.5 = unison, 1.0 = +1 octava
    float semitones = (pitchEnvVal - 0.5f) * 100.0f;     // ±50 semitones
    float pitchMod = std::pow(2.0f, semitones / 12.0f);
    
    // === GLIDE (PORTAMENTO) ===
    if (glideTime > 0.001f && currentFrequency != targetFrequency) {
        float alpha = 1.0f / (float)(sampleRate * (glideTime + 0.001f));
        float diff = targetFrequency - currentFrequency;
        currentFrequency += diff * alpha * 4.0f;
        
        if (std::abs(diff) < 0.1f) currentFrequency = targetFrequency;
    } else {
        currentFrequency = targetFrequency;
    }
    
    // === LFO VIBRATO ===
    float vibratoMod = 1.0f;
    if (vibratoDepth > 0.001f) {
        float lfoSemitones = lfoValue * vibratoDepth;
        vibratoMod = std::pow(2.0f, lfoSemitones / 12.0f);
    }
    
    // === FINAL FREQUENCY ===
    // Combina: Pitch Env + Vibrato + Pitch Bend + Master Tune
    float finalFreq = currentFrequency * pitchMod * vibratoMod 
                    * pitchBendFactor * masterTuneFactor;
    
    osc1.setFrequency(finalFreq);
    osc2.setFrequency(finalFreq * currentDetuneFactor);
    
    // === OSCILLATOR RENDERING ===
    bool osc1Wrapped = false;
    float osc1Sample = osc1.renderNextSample(dcwValue, &osc1Wrapped);
    
    // Hard Sync: reset osc2 cuando osc1 wraps
    if (isHardSyncEnabled && osc1Wrapped) {
        osc2.reset();
    }
    
    float osc2Sample = osc2.renderNextSample(dcwValue);
    
    // Ring Modulation: osc2_out = osc1 * osc2
    if (isRingModEnabled) {
        osc2Sample = osc1Sample * osc2Sample;
    }
    
    // === OSCILLATOR MIX WITH NORMALIZATION ✅ ===
    // Importante: Evitar overshooting si osc1Level + osc2Level > 1.0
    float totalLevel = osc1Level + osc2Level;
    float normalizer = (totalLevel > 1.0f) ? (1.0f / totalLevel) : 1.0f;
    
    float mix = (osc1Sample * osc1Level + osc2Sample * osc2Level) * normalizer;
    
    // === FINAL OUTPUT WITH SAFETY ===
    // 0.9f headroom para prevenir clipping de efectos posteriores
    float output = mix * dcaValue * currentVelocity * 0.9f;
    
    // Clamp final para ultra-seguridad
    return std::clamp(output, -1.0f, 1.0f);
}

float Voice::midiNoteToFrequency(int midiNote) const noexcept
{
    return 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);
}

} // namespace Core
} // namespace CZ101
