#include <JuceHeader.h>
#include "Voice.h"
#include "AudioThreadSnapshot.h" // [NEW]
#include "HardwareConstants.h"
#include "AuthenticHardware.h"
#include <cmath>
#include "../DSP/Envelopes/ADSRtoStage.h"

namespace CZ101 {
namespace Core {

Voice::Voice()
{
    // Initialize Modern Filters with full open pass-through
    lpf.setType(DSP::ResonantFilter::LOWPASS);
    lpf.setCutoff(20000.0f);
    hpf.setType(DSP::ResonantFilter::HIGHPASS);
    hpf.setCutoff(20.0f);

    // Audit Fix [11.2]: Pitch Envelope Initialization
    // Pitch envelopes must start at 0.5 (Center/No Pitch Shift) to avoid sweep up from 0.0
    pitchEnvelope1.setInitialValue(0.5f);
    pitchEnvelope2.setInitialValue(0.5f);
    
    // Set Authentic Hardware Envelope Types
    dcaEnvelope1.setType(DSP::MultiStageEnvelope::EnvType::DCA);
    dcaEnvelope2.setType(DSP::MultiStageEnvelope::EnvType::DCA);
    dcwEnvelope1.setType(DSP::MultiStageEnvelope::EnvType::DCW);
    dcwEnvelope2.setType(DSP::MultiStageEnvelope::EnvType::DCW);
    pitchEnvelope1.setType(DSP::MultiStageEnvelope::EnvType::DCO);
    pitchEnvelope2.setType(DSP::MultiStageEnvelope::EnvType::DCO);

    // Audit Fix 1.5: Safe Initialization
    // Initialize with a default valid sample rate to prevent div-by-zero 
    // in envelope calculations if accessed before prepareToPlay.
    setSampleRate(44100.0);
}

// Audit Fix [2.2]
void Voice::setModel(DSP::MultiStageEnvelope::Model newModel) noexcept
{
    dcwEnvelope1.setModel(newModel);
    dcaEnvelope1.setModel(newModel);
    pitchEnvelope1.setModel(newModel);
    dcwEnvelope2.setModel(newModel);
    dcaEnvelope2.setModel(newModel);
    pitchEnvelope2.setModel(newModel);
}

void Voice::setSampleRate(double sr) noexcept
{
    sampleRate = sr;
    osc1.setSampleRate(sr);
    osc2.setSampleRate(sr);
    dcwEnvelope1.setSampleRate(sr);
    dcaEnvelope1.setSampleRate(sr);
    pitchEnvelope1.setSampleRate(sr);
    dcwEnvelope2.setSampleRate(sr);
    dcaEnvelope2.setSampleRate(sr);
    pitchEnvelope2.setSampleRate(sr);
    lpf.setSampleRate(sr);
    hpf.setSampleRate(sr);
    
    osc1Level.reset(sr, 0.016);
    osc2Level.reset(sr, 0.016);
    masterVolume.reset(sr, 0.016);
    panValue.reset(sr, 0.016);
    
    // Smoothed Matrix & LFO Init (Control Rate = SR / 8)
    double cr = sr / 8.0;
    
    currentDetuneFactor.reset(cr, 0.016); // ~16ms smoothing for 60Hz delta updates
    vibratoDepth.reset(cr, 0.016);
    pitchBendFactor.reset(cr, 0.016);
    masterTuneFactor.reset(cr, 0.016);
    
    lfoModule.setSampleRate(cr);
    smoothedMatrix.veloToDcw.reset(cr, 0.05);
    smoothedMatrix.veloToDca.reset(cr, 0.05);
    smoothedMatrix.wheelToDcw.reset(cr, 0.05);
    smoothedMatrix.wheelToLfoRate.reset(cr, 0.05);
    smoothedMatrix.wheelToVibrato.reset(cr, 0.05);
    smoothedMatrix.atToDcw.reset(cr, 0.05);
    smoothedMatrix.atToVibrato.reset(cr, 0.05);
    smoothedMatrix.keyTrackDcw.reset(cr, 0.05);
    smoothedMatrix.keyTrackPitch.reset(cr, 0.05);
}

void Voice::noteOn(int midiNote, float velocity) noexcept
{
    currentNote = midiNote;
    // Quantize velocity to 0-7 range (3 bits) as per hardware spec
    currentVelocity = std::floor(velocity * 7.0f + 0.5f) / 7.0f;
    
    // [NEW] Record timestamp for Voice Stealing
    lastNoteOnTime = juce::Time::getMillisecondCounter();

    // Velocity Sensitivity Calculation [NEW]
    float normVel = velocity; 
    velModAmp = DSP::VelocitySensitivityProcessor::apply(normVel, velocityCurve.amplitudeResponse);
    velModPitch = DSP::VelocitySensitivityProcessor::applyPitch(normVel, velocityCurve.pitchResponse);
    velModDcw = DSP::VelocitySensitivityProcessor::apply(normVel, velocityCurve.dcwResponse);
    velModVibDepth = DSP::VelocitySensitivityProcessor::apply(normVel, velocityCurve.vibratoDepthResponse);
    velModAttack = DSP::VelocitySensitivityProcessor::apply(normVel, velocityCurve.attackResponse);

    // Key Follow Calculation (Pivot note C4 = 60)
    float keyDiff = (midiNote - 60.0f) / 12.0f;
    float kfScalePitch1 = std::pow(2.0f, keyDiff * (matrix.line1KfPitch / 9.0f));
    float kfScaleDcw1   = std::pow(2.0f, keyDiff * (matrix.line1KfDcw / 9.0f));
    float kfScaleDca1   = std::pow(2.0f, keyDiff * (matrix.line1KfDca / 9.0f));
    float kfScalePitch2 = std::pow(2.0f, keyDiff * (matrix.line2KfPitch / 9.0f));
    float kfScaleDcw2   = std::pow(2.0f, keyDiff * (matrix.line2KfDcw / 9.0f));
    float kfScaleDca2   = std::pow(2.0f, keyDiff * (matrix.line2KfDca / 9.0f));

    // Apply Rate Scaling to Envelopes (velocity attack response * Tone macro * KF)
    const float baseRateScale = velModAttack * toneRateScale;
    
    // CZ-101 global DCA Key Follow (0-9) changes the envelope rate (makes higher notes decay faster)
    float legacyDcaKf = 1.0f;
    if (matrix.kfDca > 0) {
        legacyDcaKf = std::pow(2.0f, keyDiff * (matrix.kfDca / 9.0f));
    }
    
    dcaEnvelope1.setRateScaler(baseRateScale * kfScaleDca1 * legacyDcaKf); 
    dcaEnvelope2.setRateScaler(baseRateScale * kfScaleDca2 * legacyDcaKf);
    dcwEnvelope1.setRateScaler(baseRateScale * kfScaleDcw1); 
    dcwEnvelope2.setRateScaler(baseRateScale * kfScaleDcw2);
    pitchEnvelope1.setRateScaler(baseRateScale * kfScalePitch1); 
    pitchEnvelope2.setRateScaler(baseRateScale * kfScalePitch2);

    // Apply Level Scaling to Envelopes (CZ-1 / Modern Mode)
    // Formula: Level = (MIDI Velocity) ^ (Sensitivity / 15)
    // When Sensitivity is 0, the exponent is 0 -> Level is 1.0 (no effect).
    // When Sensitivity is 15, the exponent is 1 -> Level is scaled exactly by velocity.
    float dcaScale1 = 1.0f, dcwScale1 = 1.0f, pitchScale1 = 1.0f;
    float dcaScale2 = 1.0f, dcwScale2 = 1.0f, pitchScale2 = 1.0f;
    
    // CZ-1 is opMode 2, Modern is 3. Classic 101/5000 is 0/1.
    if (matrix.opMode >= 2) 
    {
        // Add safety to avoid 0^0 which is undefined
        float safeVel = std::max(0.0001f, normVel);
        dcaScale1 = std::pow(safeVel, matrix.line1VeloDca / 15.0f);
        dcwScale1 = std::pow(safeVel, matrix.line1VeloDcw / 15.0f);
        pitchScale1 = std::pow(safeVel, matrix.line1VeloPitch / 15.0f);
        
        dcaScale2 = std::pow(safeVel, matrix.line2VeloDca / 15.0f);
        dcwScale2 = std::pow(safeVel, matrix.line2VeloDcw / 15.0f);
        pitchScale2 = std::pow(safeVel, matrix.line2VeloPitch / 15.0f);
    }
    
    dcaEnvelope1.setLevelScaler(dcaScale1); dcaEnvelope2.setLevelScaler(dcaScale2);
    dcwEnvelope1.setLevelScaler(dcwScale1); dcwEnvelope2.setLevelScaler(dcwScale2);
    pitchEnvelope1.setLevelScaler(pitchScale1); pitchEnvelope2.setLevelScaler(pitchScale2);

    baseFrequency = midiNoteToFrequency(midiNote);
    targetFrequency = baseFrequency;
    currentFrequency = baseFrequency;
    
    osc1.reset();
    osc2.reset();
    lfoModule.reset();
    
    dcwEnvelope1.noteOn();
    dcaEnvelope1.noteOn();
    pitchEnvelope1.noteOn();
    dcwEnvelope2.noteOn();
    dcaEnvelope2.noteOn();
    pitchEnvelope2.noteOn();

#ifdef OMEGA_DEBUG_VOICE
    juce::Logger::writeToLog("Voice::noteOn: note=" + juce::String(midiNote) + 
                             " freq=" + juce::String(baseFrequency) + 
                             " dca1_lvl0=" + juce::String(dcaEnvelope1.getStageLevel(0)) + 
                             " dca1_sus=" + juce::String(dcaEnvelope1.getSustainPoint()) + 
                             " dca1_end=" + juce::String(dcaEnvelope1.getEndPoint()) + 
                             " osc1_lvl=" + juce::String(osc1Level.getTargetValue()) + 
                             " masterVol=" + juce::String(masterVolume.getTargetValue()));
#endif
}

void Voice::noteOff() noexcept
{
    dcwEnvelope1.noteOff();
    dcaEnvelope1.noteOff();
    pitchEnvelope1.noteOff();
    dcwEnvelope2.noteOff();
    dcaEnvelope2.noteOff();
    pitchEnvelope2.noteOff();
}

void Voice::reset() noexcept
{
    dcwEnvelope1.reset();
    dcaEnvelope1.reset();
    pitchEnvelope1.reset();
    pitchEnvelope1.setCurrentValue(0.5f); // Audit Fix 1.1: Center Pitch Envelope
    
    dcwEnvelope2.reset();
    dcaEnvelope2.reset();
    pitchEnvelope2.reset();
    pitchEnvelope2.setCurrentValue(0.5f); // Audit Fix 1.1: Center Pitch Envelope
    
    lfoModule.reset();
    lpf.reset();
    hpf.reset();
}

// ... Oscillators ...

void Voice::setOsc1Waveforms(DSP::PhaseDistOscillator::CzWaveform f, DSP::PhaseDistOscillator::CzWaveform s, DSP::PhaseDistOscillator::CzWindow w) noexcept 
{ 
    osc1.setWaveforms(f, s, w); 
}
void Voice::setOsc1Level(float level) noexcept { osc1Level.setTargetValue(level); }

void Voice::setOsc2Waveforms(DSP::PhaseDistOscillator::CzWaveform f, DSP::PhaseDistOscillator::CzWaveform s, DSP::PhaseDistOscillator::CzWindow w) noexcept 
{ 
    osc2.setWaveforms(f, s, w); 
}
void Voice::setOsc2Level(float level) noexcept { osc2Level.setTargetValue(level); }
void Voice::setOsc2Detune(float semitones) noexcept 
{ 
    // This is legacy, the processor now calls setModulationMatrix or specific setters
    osc2Detune.setTargetValue(semitones); 
    currentDetuneFactor.setTargetValue(std::exp2(semitones / 12.0f)); 
}

void Voice::setOsc2DetuneHardware(int oct, int coarse, int fineCents) noexcept
{
    float totalSemitones = (oct * 12.0f) + coarse + (fineCents / 100.0f);
    osc2Detune.setTargetValue(totalSemitones);
    currentDetuneFactor.setTargetValue(std::exp2(totalSemitones / 12.0f));
}

void Voice::setHardSync(bool enabled) noexcept { isHardSyncEnabled = enabled; }
void Voice::setLineModulation(int mode) noexcept { lineModulation = mode; }
void Voice::setModSpecial(bool enabled) noexcept { modSpecial = enabled; }

// ── Shared line modulation DSP: invoked per sample (or sub-sample) ──
// 0=Off  1=Ring 1 (standard)  2=Noise 1  3=Ring 2 (detuned)
// 4=Ring 3 (1+2)  5=Noise 2 (milder)
float Voice::applyLineModulation(float osc1Sample, float osc2Sample, bool osc1Wrapped) noexcept
{
    switch (lineModulation) {
        case 1: // Ring 1: DCO1 × DCO2
            return osc1Sample * osc2Sample;
        case 3: { // Ring 2: detuned ring (1-sample delay on osc1)
            const float delayed1 = modDetuneDelay;
            modDetuneDelay = osc1Sample;
            return 0.5f * (osc1Sample + delayed1) * osc2Sample;
        }
        case 4: { // Ring 3: Ring 1 (75%) + Ring 2 (25%)
            const float delayed1 = modDetuneDelay;
            modDetuneDelay = osc1Sample;
            return (0.75f * osc1Sample + 0.25f * delayed1) * osc2Sample;
        }
        case 2: // Noise 1 (Authentic CZ Noise is pitch mod, not amp mod)
            return osc2Sample;
        case 5: // Noise 2 (Milder noise extension)
            return osc2Sample;
        default:
            return osc2Sample;
    }
}

void Voice::setToneRateScale(float scale) noexcept
{
    toneRateScale = (scale > 0.0f) ? scale : 1.0f;
}
void Voice::setGlideTime(float seconds) noexcept { glideTime = seconds; }


// ... LFO / Vibrato ...

void Voice::setVibratoDepth(float semitones) noexcept { vibratoDepth.setTargetValue(semitones); }
void Voice::setLFOFrequency(float hz) noexcept { lfoModule.setFrequency(hz); }
void Voice::setLFOWaveform(DSP::LFO::Waveform w) noexcept { lfoModule.setWaveform(w); }
void Voice::setLFODelay(float s) noexcept { lfoModule.setDelay(s); }

void Voice::setMasterTune(float semitones) noexcept { masterTuneFactor.setTargetValue(std::exp2(semitones / 12.0f)); }
void Voice::setMasterVolume(float level) noexcept { masterVolume.setTargetValue(level); }

// ============================================================================
// ENVELOPES - DUAL LINE SUPPORT
// ============================================================================

// DCW
void Voice::setDCWStage(int line, int i, float r, float l) noexcept { jassert(i >= 0 && i < 8); if(line==1) dcwEnvelope1.setStage(i, r, l); else dcwEnvelope2.setStage(i, r, l); }
void Voice::setDCWSustainPoint(int line, int i) noexcept { jassert(i >= -1 && i < 8); if(line==1) dcwEnvelope1.setSustainPoint(i); else dcwEnvelope2.setSustainPoint(i); }
void Voice::setDCWEndPoint(int line, int i) noexcept { jassert(i >= 0 && i < 8); if(line==1) dcwEnvelope1.setEndPoint(i); else dcwEnvelope2.setEndPoint(i); }

void Voice::getDCWStage(int line, int i, float& r, float& l) const noexcept { if(line==1) { r = dcwEnvelope1.getStageRate(i); l = dcwEnvelope1.getStageLevel(i); } else { r = dcwEnvelope2.getStageRate(i); l = dcwEnvelope2.getStageLevel(i); } }
int Voice::getDCWSustainPoint(int line) const noexcept { return line==1 ? dcwEnvelope1.getSustainPoint() : dcwEnvelope2.getSustainPoint(); }
int Voice::getDCWEndPoint(int line) const noexcept { return line==1 ? dcwEnvelope1.getEndPoint() : dcwEnvelope2.getEndPoint(); }

// DCA
void Voice::setDCAStage(int line, int i, float r, float l) noexcept { jassert(i >= 0 && i < 8); if(line==1) dcaEnvelope1.setStage(i, r, l); else dcaEnvelope2.setStage(i, r, l); }
void Voice::setDCASustainPoint(int line, int i) noexcept { jassert(i >= -1 && i < 8); if(line==1) dcaEnvelope1.setSustainPoint(i); else dcaEnvelope2.setSustainPoint(i); }
void Voice::setDCAEndPoint(int line, int i) noexcept { jassert(i >= 0 && i < 8); if(line==1) dcaEnvelope1.setEndPoint(i); else dcaEnvelope2.setEndPoint(i); }

void Voice::getDCAStage(int line, int i, float& r, float& l) const noexcept { if(line==1) { r = dcaEnvelope1.getStageRate(i); l = dcaEnvelope1.getStageLevel(i); } else { r = dcaEnvelope2.getStageRate(i); l = dcaEnvelope2.getStageLevel(i); } }
int Voice::getDCASustainPoint(int line) const noexcept { return line==1 ? dcaEnvelope1.getSustainPoint() : dcaEnvelope2.getSustainPoint(); }
int Voice::getDCAEndPoint(int line) const noexcept { return line==1 ? dcaEnvelope1.getEndPoint() : dcaEnvelope2.getEndPoint(); }

// Pitch
void Voice::setPitchStage(int line, int i, float r, float l) noexcept { jassert(i >= 0 && i < 8); if(line==1) pitchEnvelope1.setStage(i, r, l); else pitchEnvelope2.setStage(i, r, l); }
void Voice::setPitchSustainPoint(int line, int i) noexcept { jassert(i >= -1 && i < 8); if(line==1) pitchEnvelope1.setSustainPoint(i); else pitchEnvelope2.setSustainPoint(i); }
void Voice::setPitchEndPoint(int line, int i) noexcept { jassert(i >= 0 && i < 8); if(line==1) pitchEnvelope1.setEndPoint(i); else pitchEnvelope2.setEndPoint(i); }

void Voice::getPitchStage(int line, int i, float& r, float& l) const noexcept { if(line==1) { r = pitchEnvelope1.getStageRate(i); l = pitchEnvelope1.getStageLevel(i); } else { r = pitchEnvelope2.getStageRate(i); l = pitchEnvelope2.getStageLevel(i); } }
int Voice::getPitchSustainPoint(int line) const noexcept { return line==1 ? pitchEnvelope1.getSustainPoint() : pitchEnvelope2.getSustainPoint(); }
int Voice::getPitchEndPoint(int line) const noexcept { return line==1 ? pitchEnvelope1.getEndPoint() : pitchEnvelope2.getEndPoint(); }

// ============================================================================
// ADSR WRAPPERS (LEGACY SUPPORT)
// ============================================================================

void Voice::setDCWAttack(float seconds) noexcept { dcwADSR1.attackMs = seconds * 1000.0f; dcwADSR2.attackMs = seconds * 1000.0f; updateDCWEnvelopeFromADSR(1); updateDCWEnvelopeFromADSR(2); }
void Voice::setDCWDecay(float seconds) noexcept { dcwADSR1.decayMs = seconds * 1000.0f; dcwADSR2.decayMs = seconds * 1000.0f; updateDCWEnvelopeFromADSR(1); updateDCWEnvelopeFromADSR(2); }
void Voice::setDCWSustain(float level) noexcept { dcwADSR1.sustainLevel = level; dcwADSR2.sustainLevel = level; updateDCWEnvelopeFromADSR(1); updateDCWEnvelopeFromADSR(2); }
void Voice::setDCWRelease(float seconds) noexcept { dcwADSR1.releaseMs = seconds * 1000.0f; dcwADSR2.releaseMs = seconds * 1000.0f; updateDCWEnvelopeFromADSR(1); updateDCWEnvelopeFromADSR(2); }

void Voice::setDCAAttack(float seconds) noexcept { dcaADSR1.attackMs = seconds * 1000.0f; dcaADSR2.attackMs = seconds * 1000.0f; updateDCAEnvelopeFromADSR(1); updateDCAEnvelopeFromADSR(2); }
void Voice::setDCADecay(float seconds) noexcept { dcaADSR1.decayMs = seconds * 1000.0f; dcaADSR2.decayMs = seconds * 1000.0f; updateDCAEnvelopeFromADSR(1); updateDCAEnvelopeFromADSR(2); }
void Voice::setDCASustain(float level) noexcept { dcaADSR1.sustainLevel = level; dcaADSR2.sustainLevel = level; updateDCAEnvelopeFromADSR(1); updateDCAEnvelopeFromADSR(2); }
void Voice::setDCARelease(float seconds) noexcept { dcaADSR1.releaseMs = seconds * 1000.0f; dcaADSR2.releaseMs = seconds * 1000.0f; updateDCAEnvelopeFromADSR(1); updateDCAEnvelopeFromADSR(2); }

// ===== HELPER METHODS FOR ADSR CONSISTENCY =====

void Voice::updateDCWEnvelopeFromADSR(int line) noexcept
{
    std::array<float, 8> rates, levels;
    int sus, end;
    auto& adsr = (line == 1) ? dcwADSR1 : dcwADSR2;
    auto& env = (line == 1) ? dcwEnvelope1 : dcwEnvelope2;
    
    DSP::ADSRtoStageConverter::convertADSR(adsr.attackMs, adsr.decayMs, adsr.sustainLevel, adsr.releaseMs, rates, levels, sus, end, sampleRate);
    
    for (int i = 0; i < 4; ++i) env.setStage(i, rates[i], levels[i]);
    env.setSustainPoint(sus);
    env.setEndPoint(end);
}

void Voice::updateDCAEnvelopeFromADSR(int line) noexcept
{
    std::array<float, 8> rates, levels;
    int sus, end;
    auto& adsr = (line == 1) ? dcaADSR1 : dcaADSR2;
    auto& env = (line == 1) ? dcaEnvelope1 : dcaEnvelope2;
    
    DSP::ADSRtoStageConverter::convertADSR(adsr.attackMs, adsr.decayMs, adsr.sustainLevel, adsr.releaseMs, rates, levels, sus, end, sampleRate);
    
    for (int i = 0; i < 4; ++i) env.setStage(i, rates[i], levels[i]);
    env.setSustainPoint(sus);
    env.setEndPoint(end);
}

void Voice::updatePitchEnvelopeFromADSR(int line) noexcept
{
    std::array<float, 8> rates, levels;
    int sus, end;
    auto& adsr = (line == 1) ? pitchADSR1 : pitchADSR2;
    auto& env = (line == 1) ? pitchEnvelope1 : pitchEnvelope2;
    
    DSP::ADSRtoStageConverter::convertADSR(adsr.attackMs, adsr.decayMs, adsr.sustainLevel, adsr.releaseMs, rates, levels, sus, end, sampleRate);
    
    for (int i = 0; i < 4; ++i) env.setStage(i, rates[i], levels[i]);
    env.setSustainPoint(sus);
    env.setEndPoint(end);
}

// ============================================================================
// RENDERING
// ============================================================================

// Fast Tanh Approximation (Rational type)
// Much faster than std::tanh for real-time audio
inline float fastTanh(float x) noexcept
{
    // Audit Fix [Denormals]: Ensure x is not a denormal
    if (std::abs(x) < 1e-9f) return x; 

    float x2 = x * x;
    float a = x * (135135.0f + x2 * (17325.0f + x2 * (378.0f + x2)));
    float b = 135135.0f + x2 * (62370.0f + x2 * (3150.0f + x2 * 28.0f));
    return (b < 1e-20f) ? std::copysign(1.0f, x) : a / b;
}

// Deterministic exp2 approximation for cross-platform bit-identicality
// Using a 5th order polynomial or high precision rational
inline float deterministicExp2(float x) noexcept
{
    // Simple 3rd order minimax for larger range [-12, 12] semitones? 
    // Input x is "semitones / 12 * 2" ? No, input to exp2 is the power. 2^x.
    // Pitch mod +1 oct = 2^1. x=1.
    // Pitch mod +2 oct = 2^2. x=2.
    // If we clamp to [-1, 1], we limit pitch mod to ±1 octave.
    // CZ can do several octaves.
    // We should widen the range. The polynomial approx diverges outside [-1,1].
    // Better to use std::exp2 for full range fidelity if we want "Professional" results.
    // Or use a range reduction technique.
    // Given modern CPU, std::exp2 is often intrinsic and fast enough.
    // Replacing with std::exp2 for safety and range.
    return std::exp2(x); 
    // return 1.0f + x * (0.69314718f + x * (0.24022650f + x * (0.05550411f + x * 0.00961812f)));
}

float Voice::renderNextSample() noexcept
{
    if (!dcaEnvelope1.isActive() && !dcaEnvelope2.isActive()) return 0.0f;
    
    // === CONTROL RATE MODULATION (Every 8 samples) ===
    if ((sampleCounter++ & HardwareConstants::CONTROL_RATE_MASK) == 0)
    {
        processControlRate();
    }
    
    float rawMix = renderOscillators();
    return applyPostProcessing(rawMix);
}

void Voice::processControlRate() noexcept
{
    calculateEnvelopeValues();
    calculateLFOAndVibrato();
    calculateDCWModulation();
    calculateDCAModulation();
    calculatePitchModulation();
    // Free matrix: Pan destination (dest 7) → per-voice pan position.
    // depth 1.0 = full left, -1.0 = full right (0.5 = center, default).
    panValue.setTargetValue(juce::jlimit(0.0f, 1.0f, 0.5f + getModSlotContribution(7) * 0.5f));
}

void Voice::calculateEnvelopeValues() noexcept
{
    dcwVal1 = dcwEnvelope1.getNextValue();
    dcaVal1 = dcaEnvelope1.getNextValue();
    pitchVal1 = pitchEnvelope1.getNextValue();
    
    dcwVal2 = dcwEnvelope2.getNextValue();
    dcaVal2 = dcaEnvelope2.getNextValue();
    pitchVal2 = pitchEnvelope2.getNextValue();
}
    
void Voice::calculateLFOAndVibrato() noexcept
{
    // LFO — advance once per control-rate block and cache for the free matrix
    vibratoMod = 1.0f;
    lastLfoValue = lfoModule.getNextValue();
    // Noise — cache one random value per control-rate block (free-matrix source 10)
    lastNoiseValue = noiseGen.nextFloat() * 2.0f - 1.0f;

    float wheelVib = smoothedMatrix.wheelToVibrato.getNextValue();
    float atVib = smoothedMatrix.atToVibrato.getNextValue();
    // Free matrix: vibrato-depth routes (dest 4)
    float freeVib = getModSlotContribution(4);
    float totalVibDepth = vibratoDepth.getNextValue() + (modWheel * wheelVib) + (aftertouch * atVib) + freeVib;
    
    // Apply LFO Rate Modulation from Wheel + free matrix (dest 5)
    float wLfoRate = smoothedMatrix.wheelToLfoRate.getNextValue();
    float freeLfoRate = getModSlotContribution(5);
    lfoModule.setFrequencyScale(1.0f + modWheel * wLfoRate * 3.0f + freeLfoRate * 3.0f);

    if (totalVibDepth > 0.001f) {
        // vibratoDepth is in semitones; exp2 requires octaves (semitones / 12)
        vibratoMod = deterministicExp2((lastLfoValue * totalVibDepth) / 12.0f); 
    }
}

void Voice::calculateDCWModulation() noexcept
{
    // DCW Key Tracking & Modulation
    // Note: The authentic hardware DCW limit (anti-aliasing) is now applied directly in PhaseDistOscillator
    // so we no longer apply a heuristic ktOffset here. The envelope naturally gets clamped at high pitches.
    
    float veloDcw = smoothedMatrix.veloToDcw.getNextValue();
    float wheelDcw = smoothedMatrix.wheelToDcw.getNextValue();
    float atDcw = smoothedMatrix.atToDcw.getNextValue();

    float modDcw = (currentVelocity * veloDcw) + (modWheel * wheelDcw) + (aftertouch * atDcw);
    // Free matrix: DCW routes (dest 1)
    modDcw += getModSlotContribution(1);
    
    // Apply Velocity Sensitivity to DCW Envelope Output
    dcwVal1 = juce::jlimit(0.0f, 0.99f, (dcwVal1 * velModDcw) + modDcw);
    dcwVal2 = juce::jlimit(0.0f, 0.99f, (dcwVal2 * velModDcw) + modDcw);
}

void Voice::calculateDCAModulation() noexcept
{
    // DCA Velocity Sensitivity & Key Follow
    float vDca = smoothedMatrix.veloToDca.getNextValue();
    // matrix.veloToDca: 0 = fixed level, 1 = full velocity range
    float veloDca = 1.0f - vDca + (currentVelocity * vDca);

    // Free matrix: DCA routes (dest 2)
    float freeDca = getModSlotContribution(2);

    dcaVal1 = juce::jlimit(0.0f, 1.0f, (dcaVal1 * veloDca) + freeDca);
    dcaVal2 = juce::jlimit(0.0f, 1.0f, (dcaVal2 * veloDca) + freeDca);
}

void Voice::calculatePitchModulation() noexcept
{    
    // Pitch mod (±12 semitones) - Deterministic cross-platform
    pitchMod1 = deterministicExp2((pitchVal1 - 0.5f) * 2.0f); 
    pitchMod2 = deterministicExp2((pitchVal2 - 0.5f) * 2.0f);

    // Authentic CZ Noise Pitch Modulation (runs at control rate, every 8 samples)
    if (lineModulation == 2 || lineModulation == 5) // Noise 1 or Noise 2
    {
        // 50% chance to jump pitch by +32 semitones
        if (noiseGen.nextFloat() > 0.5f) {
            // 2^(32/12) = 6.3496042f
            pitchMod2 *= 6.3496042f; 
        }
    }

    // Key Tracking for Pitch (DCO Key Follow: 1:1 standard tracking)
    float pitchKT = 1.0f;
    float ktPitch = smoothedMatrix.keyTrackPitch.getNextValue();
    if (std::abs(ktPitch - 1.0f) > 0.001f) {
        float dist = (currentNote - 60) / 12.0f;
        pitchKT = std::exp2(dist * (ktPitch - 1.0f));
    }

    // Free matrix: Pitch routes (dest 3) as semitone offsets, folded into the
    // cached static factors (deterministic exp2 of the summed contribution).
    float freePitch = getModSlotContribution(3);
    float freePitchFactor = deterministicExp2(freePitch / 12.0f); // depth 1.0 ≈ +1 semitone

    // Free matrix: Osc2 Detune (dest 6) as semitone offsets on the detune factor
    float freeDetune = getModSlotContribution(6); // depth 1.0 ≈ +1 semitone

    // Cache static factors (Master Tune, Pitch Bend)
    float globalMod = pitchBendFactor.getNextValue() * masterTuneFactor.getNextValue() * pitchKT * freePitchFactor;
    
    // Target Frequencies (Control Rate)
    if (glideTime > 0.001f && currentFrequency != targetFrequency) {
        float currentLog = std::log2(currentFrequency);
        float targetLog = std::log2(targetFrequency);
        float diffLog = targetLog - currentLog;
        float step = (8.0f / (float)sampleRate) / (glideTime + 0.001f);
        
        if (std::abs(diffLog) <= step) {
            currentFrequency = targetFrequency;
        } else {
            currentLog += (diffLog > 0 ? step : -step);
            currentFrequency = std::exp2(currentLog);
        }
    } else {
        currentFrequency = targetFrequency;
    }

    cachedFreq1 = currentFrequency * pitchMod1 * vibratoMod * globalMod * velModPitch;
    cachedFreq2 = currentFrequency * pitchMod2 * vibratoMod * globalMod * velModPitch
                * currentDetuneFactor.getNextValue() * deterministicExp2(freeDetune / 12.0f);
}

float Voice::renderOscillators() noexcept
{
    // Phase 5.1: Oversampling Implementation
    // If oversamplingFactor > 1, we render multiple sub-samples and average them
    // This reduces aliasing at the cost of CPU
    
    if (oversamplingFactor <= 1)
    {
        // Standard path (1x - no oversampling)
        osc1.setFrequency(cachedFreq1);
        osc2.setFrequency(cachedFreq2);
        
        bool osc1Wrapped = false;
        float osc1Sample = osc1.renderNextSample(dcwVal1, &osc1Wrapped);
        if (isHardSyncEnabled && osc1Wrapped) osc2.reset();
        float osc2Sample = osc2.renderNextSample(dcwVal2);
        
        // 1. Shape oscillators with DCA before Ring Mod
        float shaped1 = osc1Sample * osc1Level.getNextValue() * dcaVal1 * velModAmp;
        float shaped2 = osc2Sample * osc2Level.getNextValue() * dcaVal2 * velModAmp;

        // 2. Line modulation (Ring Mod / Noise) using shaped Osc 1
        shaped2 = applyLineModulation(shaped1, shaped2, osc1Wrapped);

        // 3. Mod Special (Mute Line 1 in the final mix)
        float out1 = modSpecial ? 0.0f : shaped1;
        float out2 = shaped2;
        
        return HardwareConstants::mixLines(out1, out2);
    }
    else
    {
        // Oversampled path (2x or 4x)
        osc1.setFrequency(cachedFreq1);
        osc2.setFrequency(cachedFreq2);
        
        float accumulator = 0.0f;
        
        // Render N sub-samples
        for (int i = 0; i < oversamplingFactor; ++i)
        {
            bool osc1Wrapped = false;
            float osc1Sample = osc1.renderNextSample(dcwVal1, &osc1Wrapped);
            if (isHardSyncEnabled && osc1Wrapped) osc2.reset();
            float osc2Sample = osc2.renderNextSample(dcwVal2);
            
            osc2Sample = applyLineModulation(osc1Sample, osc2Sample, osc1Wrapped);

            // Note: We use the same envelope/level values for all sub-samples
            // This is a simplification but works well for anti-aliasing
            float out1 = modSpecial ? 0.0f : (osc1Sample * osc1Level.getCurrentValue() * dcaVal1 * velModAmp);
            float out2 = osc2Sample * osc2Level.getCurrentValue() * dcaVal2 * velModAmp;
            
            accumulator += HardwareConstants::mixLines(out1, out2);
        }
        
        // Average (boxcar filter / decimation)
        float result = accumulator / (float)oversamplingFactor;
        
        // Advance smoothed values once per output sample
        osc1Level.getNextValue();
        osc2Level.getNextValue();
        
        return result;
    }
}

float Voice::applyPostProcessing(float rawMix) noexcept
{
    // Phase 9: Authentic Hardware Noise
    if (hardwareNoiseEnabled) {
         // Mix in the "dirty" noise before the filter/VCA chain? 
         // Real hardware: DAC noise is post-DCW but pre-Analog VDA/Filter? No, CZ is digital DCW/DCA.
         // DAC is at the very end. So noise should be added here.
         float dcwMix = (dcwVal1 + dcwVal2) * 0.5f;
         rawMix += getAuthenticNoise(currentNote, dcwMix);
    }

    // Optimization: Digital path has no analog soft clip pre-filter.
    float softClipped = rawMix;
    
    // Modern Filter Processing (Phase 7)
    float filtered = lpf.processSample(softClipped);
    filtered = hpf.processSample(filtered);

    float output = filtered * currentVelocity * masterVolume.getNextValue();
    
    // Advance pan smoothing every sample (the ramp target is set by
    // processControlRate → calculatePanAndEffects). VoiceManager reads the
    // smoothed value via getPan() for stereo panning.
    panValue.getNextValue();
    
    // Phase 9: 12-bit DAC Compression Simulation
    if (hardwareNoiseEnabled) {
        // Apply non-linear compression to the final output
        float absOut = std::abs(output);
        float sign = (output >= 0.0f) ? 1.0f : -1.0f;
        output = sign * HardwareConstants::applyDACCompression(absOut);
    }
    
    return output * HardwareConstants::MASTER_HEADROOM_GAIN;
}

// Audit Fix [Div-0/NaN]: Clamp MIDI note to valid range to prevent denormals/NaNs 
float Voice::midiNoteToFrequency(int midiNote) const noexcept
{
    midiNote = juce::jlimit(0, 127, midiNote); 
    return 440.0f * std::exp2((static_cast<float>(midiNote) - 69.0f) * 0.083333333f); // slightly faster than /12.0f
}

void Voice::setPitchBend(float semitones) noexcept
{
    pitchBendSemitones = semitones;
    pitchBendFactor.setTargetValue(std::exp2(semitones / 12.0f));
}

void Voice::setModulationMatrix(const ModulationMatrix& m) noexcept
{
    matrix = m;
    smoothedMatrix.veloToDcw.setTargetValue(m.veloToDcw);
    smoothedMatrix.veloToDca.setTargetValue(m.veloToDca);
    smoothedMatrix.wheelToDcw.setTargetValue(m.wheelToDcw);
    smoothedMatrix.wheelToLfoRate.setTargetValue(m.wheelToLfoRate);
    smoothedMatrix.wheelToVibrato.setTargetValue(m.wheelToVibrato);
    smoothedMatrix.atToDcw.setTargetValue(m.atToDcw);
    smoothedMatrix.atToVibrato.setTargetValue(m.atToVibrato);
    smoothedMatrix.keyTrackDcw.setTargetValue(m.keyTrackDcw);
    smoothedMatrix.keyTrackPitch.setTargetValue(m.keyTrackPitch);
    
    // Cache the velocity sensitivities and opMode directly into matrix 
    matrix.line1VeloPitch = m.line1VeloPitch;
    matrix.line1VeloDcw = m.line1VeloDcw;
    matrix.line1VeloDca = m.line1VeloDca;
    matrix.line2VeloPitch = m.line2VeloPitch;
    matrix.line2VeloDcw = m.line2VeloDcw;
    matrix.line2VeloDca = m.line2VeloDca;
    matrix.opMode = m.opMode;
}

// Free matrix (ABDEEP-style). Sources: 0=None 1=Velocity 2=ModWheel
// 3=Aftertouch 4=KeyTrack 5=LFO 6=EnvDCW 7=EnvDCA 8=EnvPitch(bipolar)
// 9=PitchBend 10=Noise 11=AuthKeyTrack (authentic hardware curve)
float Voice::getModSlotSourceValue(int source) const noexcept
{
    switch (source)
    {
        case 1:  return currentVelocity;
        case 2:  return modWheel;
        case 3:  return aftertouch;
        case 4:  return currentNote >= 0 ? (currentNote - 60) / 60.0f : 0.0f;
        case 5:  return lastLfoValue; // cached once per control-rate block
        case 6:  return dcwEnvelope1.getCurrentValue();
        case 7:  return dcaEnvelope1.getCurrentValue();
        // Pitch envelope is 0..1 centered at 0.5 → bipolar -1..1 (centre = no mod)
        case 8:  return (pitchEnvelope1.getCurrentValue() - 0.5f) * 2.0f;
        // Pitch bend in raw semitones, normalized so a full octave = ±1
        case 9:  return juce::jlimit(-1.0f, 1.0f, pitchBendSemitones / 12.0f);
        // White noise, cached once per control-rate block (-1..1)
        case 10: return lastNoiseValue;
        // Authentic hardware Key Follow curve (DCW). Evaluated with the current
        // DCW envelope value so the envelope interaction of the real CZ is
        // replicated exactly: depth 1.0 reproduces Key Follow FIX/VAR 1:1.
        case 11:
        {
            const float avgEnv = (dcwVal1 + dcwVal2) * 0.5f;
            return HardwareConstants::getAuthenticDCWKeytrack(currentNote, avgEnv, matrix.kfDcw);
        }
        default: return 0.0f;
    }
}

// Sums the weighted source values of every free slot routed to `dest`.
float Voice::getModSlotContribution(int dest) const noexcept
{
    float total = 0.0f;
    for (int i = 0; i < ModulationMatrix::kNumModSlots; ++i)
    {
        const auto& slot = matrix.slots[i];
        if (slot.dest != dest || slot.source == 0 || slot.depth == 0.0f)
            continue;
        total += getModSlotSourceValue(slot.source) * slot.depth;
    }
    return total;
}

float Voice::getAuthenticNoise(int note, float dcwLevel) noexcept
{
    // Ruido escalado por DCW (16-bit DAC crunch)
    float dacNoise = (noiseGen.nextFloat() * 2.0f - 1.0f) * (HardwareConstants::DAC_NOISE_FLOOR + dcwLevel * 0.0003f);
    
    // Key click artificial (env spike al inicio)
    // We approximate the spike using the start of the DCA envelope
    float keyClick = 0.0f;
    // Audit Fix [KeyClick]: Use time-based logic instead of envelope state for consistency
    // Impulse spike at note start (first 5ms)
    // We can use sampleCounter reset at NoteOn? 
    // Let's use `dcaEnvelope1.isActive()` combined with a transient check
    // Or better: pass `isNoteStart` flag?
    // Actually, checking "Attacking and Level < 0.1" is okay but fragile with slow attacks.
    // Authentic Key Click happens due to VCA offset/leakage at Note On.
    // Let's simulate it by adding a tiny impulse if `dcaEnvelope1` is in Stage 0.
    if (dcaEnvelope1.getCurrentStage() == 0) {
         // Decay the click rapidly based on level
         float clickEnv = 1.0f - (dcaEnvelope1.getCurrentValue() * 10.0f);
         if (clickEnv > 0.0f) keyClick = HardwareConstants::KEY_CLICK_SPIKE * (note / 127.0f) * clickEnv;
    }
    
    return dacNoise + keyClick;
}

void Voice::applySnapshot(const ParameterSnapshot* s) noexcept
{
    if (!s) return;
    
    // Optimized Parameter Updates from Snapshot
    // Only update smoothed targets, avoiding complex logic
    
    // Oscillators
    osc1.setWaveforms(static_cast<DSP::PhaseDistOscillator::CzWaveform>(s->dco1.wave1),
                      static_cast<DSP::PhaseDistOscillator::CzWaveform>(s->dco1.wave2),
                      static_cast<DSP::PhaseDistOscillator::CzWindow>(s->dco1.window));
    osc2.setWaveforms(static_cast<DSP::PhaseDistOscillator::CzWaveform>(s->dco2.wave1),
                      static_cast<DSP::PhaseDistOscillator::CzWaveform>(s->dco2.wave2),
                      static_cast<DSP::PhaseDistOscillator::CzWindow>(s->dco2.window));
                      
    osc1Level.setTargetValue(s->dco1.level);
    osc2Level.setTargetValue(s->dco2.level);
    float totalDetune = s->dco2.legacyDetune + (s->dco2.octave * 12.0f) + s->dco2.coarse + (s->dco2.fine / 100.0f);
    setOsc2Detune(totalDetune);
    
    // Flags
    setHardSync(s->mod.detune == 1); 
    setLineModulation(s->lineMod.mode);
    setModSpecial(s->lineMod.special);
    
    // Filter / DCW
    // Note: DCW/DCA Envelopes are usually triggered, but key follow/sustains are state.

    // System
    masterVolume.setTargetValue(s->system.masterVol);
    masterTuneFactor = std::pow(2.0f, s->system.masterTune / 12.0f);
    pitchBendFactor = std::pow(2.0f, (s->system.bendRange / 12.0f) * modWheel); // simplified
    
    // Matrix. The routing matrix (wheel->dcw, wheel->lfo rate, aftertouch
    // routes) is a Modern-only feature of this emulator; CZ-101/CZ-5000 only
    // had the hardware sensitivities below (velocity, wheel->vibrato, key
    // track/follow), which stay active in every mode.
    const bool modMatrixEnabled = (s->system.opMode == 3);
    smoothedMatrix.veloToDcw.setTargetValue(s->mod.veloDcw);
    // In Modern the free matrix owns the routing: the WebUI seeds a
    // Velocity→DCA slot on new presets, so the fixed hardware Velo→DCA route
    // is zeroed here to avoid double velocity→amp. Classic modes keep the
    // authentic hardware behavior.
    smoothedMatrix.veloToDca.setTargetValue(modMatrixEnabled ? 0.0f : s->mod.veloAmp);
    smoothedMatrix.wheelToDcw.setTargetValue(modMatrixEnabled ? s->mod.wheelToDcw : 0.0f);
    smoothedMatrix.wheelToLfoRate.setTargetValue(modMatrixEnabled ? s->mod.wheelToLfoRate : 0.0f);
    smoothedMatrix.wheelToVibrato.setTargetValue(s->mod.wheelToVibrato);
    smoothedMatrix.atToDcw.setTargetValue(modMatrixEnabled ? s->mod.atToDcw : 0.0f);
    smoothedMatrix.atToVibrato.setTargetValue(modMatrixEnabled ? s->mod.atToVibrato : 0.0f);
    
    // Audit Fix [PITCH_FIX]: Correctly interpret Key Follow modes
    // Modes: 0=OFF, 1=FIX, 2=VAR
    matrix.kfDcw = s->mod.keyFollowDcw;
    matrix.kfDca = s->mod.keyFollowAmp;
    matrix.kfDco = s->mod.keyFollowDco;
    matrix.opMode = s->system.opMode;
    
    matrix.line1VeloPitch = s->mod.line1VeloPitch;
    matrix.line1VeloDcw = s->mod.line1VeloDcw;
    matrix.line1VeloDca = s->mod.line1VeloDca;
    matrix.line2VeloPitch = s->mod.line2VeloPitch;
    matrix.line2VeloDcw = s->mod.line2VeloDcw;
    matrix.line2VeloDca = s->mod.line2VeloDca;
    
    matrix.line1KfPitch = s->mod.line1KfPitch;
    matrix.line1KfDcw = s->mod.line1KfDcw;
    matrix.line1KfDca = s->mod.line1KfDca;
    matrix.line2KfPitch = s->mod.line2KfPitch;
    matrix.line2KfDcw = s->mod.line2KfDcw;
    matrix.line2KfDca = s->mod.line2KfDca;
    
    // amounts (Wait, were these amounts in snapshot?)
    // Snapshot doesn't have explicit amounts for KF yet, assuming 1.0 (Standard Tracking) if ON.
    smoothedMatrix.keyTrackDcw.setTargetValue(matrix.kfDcw > 0 ? 1.0f : 0.0f);
    smoothedMatrix.keyTrackPitch.setTargetValue(matrix.kfDco > 0 ? 1.0f : 0.0f);

    hardwareNoiseEnabled = s->system.hardwareNoise;
    
    // LFO
    lfoModule.setFrequency(s->lfo.rate);
    lfoModule.setWaveform(static_cast<DSP::LFO::Waveform>(s->lfo.waveform));
    vibratoDepth = s->lfo.depth;
    lfoModule.setDelay(s->lfo.delay);

    // Envelopes (Restoration)
    auto applyEnv = [](DSP::MultiStageEnvelope& env, const Core::ParameterSnapshot::EnvParam& src) {
        for(int i=0; i<8; ++i) env.setStage(i, src.rates[i], src.levels[i]);
        env.setSustainPoint(src.sustain);
        env.setEndPoint(src.end);
    };
    
    if (s->envelopes.dca1.sustain != -2) { // Check sentinel? Or just apply.
        applyEnv(dcwEnvelope1, s->envelopes.dcw1);
        applyEnv(dcwEnvelope2, s->envelopes.dcw2);
        applyEnv(dcaEnvelope1, s->envelopes.dca1);
        applyEnv(dcaEnvelope2, s->envelopes.dca2);
        applyEnv(pitchEnvelope1, s->envelopes.pitch1);
        applyEnv(pitchEnvelope2, s->envelopes.pitch2);
    }
}

// Duplicate applySnapshot removed.

} // namespace Core


} // namespace CZ101
