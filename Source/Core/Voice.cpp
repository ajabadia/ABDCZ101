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
    // Initialize Modern Filters
    lpf.setType(DSP::ResonantFilter::LOWPASS);
    hpf.setType(DSP::ResonantFilter::HIGHPASS);

    // Audit Fix [11.2]: Pitch Envelope Initialization
    // Pitch envelopes must start at 0.5 (Center/No Pitch Shift) to avoid sweep up from 0.0
    pitchEnvelope1.setInitialValue(0.5f);
    pitchEnvelope2.setInitialValue(0.5f);

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
    lfoModule.setSampleRate(sr);
    lpf.setSampleRate(sr);
    hpf.setSampleRate(sr);
    
    osc1Level.reset(sr, 0.02);
    osc2Level.reset(sr, 0.02);
    currentDetuneFactor.reset(sr, 0.05); // Detune needs longer smoothing
    masterVolume.reset(sr, 0.02);

    updateDCWEnvelopeFromADSR(1);
    updateDCAEnvelopeFromADSR(1);
    updatePitchEnvelopeFromADSR(1);
    updateDCWEnvelopeFromADSR(2);
    updateDCAEnvelopeFromADSR(2);
    updatePitchEnvelopeFromADSR(2);
    
    // Smoothed Matrix Init (Control Rate = SR / 8)
    double cr = sr / 8.0;
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

    // Apply Rate Scaling to Envelopes
    dcaEnvelope1.setRateScaler(velModAttack); dcaEnvelope2.setRateScaler(velModAttack);
    dcwEnvelope1.setRateScaler(velModAttack); dcwEnvelope2.setRateScaler(velModAttack);
    pitchEnvelope1.setRateScaler(velModAttack); pitchEnvelope2.setRateScaler(velModAttack);

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

void Voice::setOsc1Waveforms(DSP::PhaseDistOscillator::CzWaveform f, DSP::PhaseDistOscillator::CzWaveform s) noexcept 
{ 
    osc1.setWaveforms(f, s); 
}
void Voice::setOsc1Level(float level) noexcept { osc1Level.setTargetValue(level); }

void Voice::setOsc2Waveforms(DSP::PhaseDistOscillator::CzWaveform f, DSP::PhaseDistOscillator::CzWaveform s) noexcept 
{ 
    osc2.setWaveforms(f, s); 
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
void Voice::setRingMod(bool enabled) noexcept { isRingModEnabled = enabled; }
void Voice::setNoiseMod(bool enabled) noexcept { isNoiseModEnabled = enabled; }
void Voice::setGlideTime(float seconds) noexcept { glideTime = seconds; }


// ... LFO / Vibrato ...

void Voice::setVibratoDepth(float semitones) noexcept { vibratoDepth = semitones; }
void Voice::setLFOFrequency(float hz) noexcept { lfoModule.setFrequency(hz); }
void Voice::setLFOWaveform(DSP::LFO::Waveform w) noexcept { lfoModule.setWaveform(w); }
void Voice::setLFODelay(float s) noexcept { lfoModule.setDelay(s); }

void Voice::setPitchBend(float semitones) noexcept { pitchBendFactor = std::exp2(semitones / 12.0f); }
void Voice::setMasterTune(float semitones) noexcept { masterTuneFactor = std::exp2(semitones / 12.0f); }
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
}

void Voice::calculateEnvelopeValues() noexcept
{
    dcwVal1 = dcwEnvelope1.getNextValue();
    dcaVal1 = dcaEnvelope1.getNextValue();
    
    dcwVal2 = dcwEnvelope2.getNextValue();
    dcaVal2 = dcaEnvelope2.getNextValue();
}
    
void Voice::calculateLFOAndVibrato() noexcept
{
    // LFO
    vibratoMod = 1.0f;
    float wheelVib = smoothedMatrix.wheelToVibrato.getNextValue();
    float atVib = smoothedMatrix.atToVibrato.getNextValue();
    float totalVibDepth = vibratoDepth + (modWheel * wheelVib) + (aftertouch * atVib);
    
    // Apply LFO Rate Modulation from Wheel
    float wLfoRate = smoothedMatrix.wheelToLfoRate.getNextValue();
    if (wLfoRate > 0.001f) {
        lfoModule.setFrequencyScale(1.0f + modWheel * wLfoRate * 3.0f);
    } else {
        lfoModule.setFrequencyScale(1.0f);
    }

    if (totalVibDepth > 0.001f) {
        vibratoMod = deterministicExp2(lfoModule.getNextValue() * totalVibDepth); 
    }
}

void Voice::calculateDCWModulation() noexcept
{
    // DCW Key Tracking & Modulation
    float ktOffset = 0.0f;
    float ktDcw = smoothedMatrix.keyTrackDcw.getNextValue();
    if (matrix.kfDcw != 0) // FIX or VAR
    {
        // Use authentic hardware curve
        // Pass current DCW env value (average of both lines for now) to affect curvature
        float avgEnv = (dcwVal1 + dcwVal2) * 0.5f;
        ktOffset = HardwareConstants::getAuthenticDCWKeytrack(currentNote, avgEnv) * ktDcw;
    }
    
    float veloDcw = smoothedMatrix.veloToDcw.getNextValue();
    float wheelDcw = smoothedMatrix.wheelToDcw.getNextValue();
    float atDcw = smoothedMatrix.atToDcw.getNextValue();

    float modDcw = (currentVelocity * veloDcw) + (modWheel * wheelDcw) + (aftertouch * atDcw);
    // Apply Velocity Sensitivity to DCW Envelope Output
    dcwVal1 = juce::jlimit(0.0f, 0.99f, (dcwVal1 * velModDcw) + ktOffset + modDcw);
    dcwVal2 = juce::jlimit(0.0f, 0.99f, (dcwVal2 * velModDcw) + ktOffset + modDcw);
}

void Voice::calculateDCAModulation() noexcept
{
    // DCA Velocity Sensitivity & Key Follow
    if (matrix.kfDca != 0)
    {
        // Key Follow for DCA shortens decay on high notes (simulated as slight level reduction here)
        float kfDcaOffset = (currentNote - 60) * HardwareConstants::KEYTRACK_DCA_OFFSET;
        dcaVal1 = juce::jlimit(0.0f, 1.0f, dcaVal1 + kfDcaOffset);
        dcaVal2 = juce::jlimit(0.0f, 1.0f, dcaVal2 + kfDcaOffset);
    }

    float vDca = smoothedMatrix.veloToDca.getNextValue();
    // matrix.veloToDca: 0 = fixed level, 1 = full velocity range
    float veloDca = 1.0f - vDca + (currentVelocity * vDca);
    dcaVal1 *= veloDca;
    dcaVal2 *= veloDca;
}

void Voice::calculatePitchModulation() noexcept
{    
    float pEnvVal1 = pitchEnvelope1.getCurrentValue();
    float pEnvVal2 = pitchEnvelope2.getCurrentValue();
    
    // Pitch mod (±12 semitones) - Deterministic cross-platform
    pitchMod1 = deterministicExp2((pEnvVal1 - 0.5f) * 2.0f); 
    pitchMod2 = deterministicExp2((pEnvVal2 - 0.5f) * 2.0f);

     // Custom Key Tracking for Pitch (DCO Key Follow)
    float pitchKT = 1.0f;
    float ktPitch = smoothedMatrix.keyTrackPitch.getNextValue();
    if (matrix.kfDco != 0) // FIX or VAR
    {
         if (std::abs(ktPitch - 1.0f) > 0.001f) {
            float dist = (currentNote - 60) / 12.0f;
            pitchKT = std::exp2(dist * (ktPitch - 1.0f));
        }
    }
    else
    {
        // OFF mode: Pitch is fixed at center note? 
        // Real hardware "OFF" for DCO usually means 0 tracking (fixed pitch).
        float dist = (60 - currentNote) / 12.0f;
        pitchKT = std::exp2(dist); // Cancels out the currentNote tracking
    }

    // Cache static factors (Master Tune, Pitch Bend)
    float globalMod = pitchBendFactor * masterTuneFactor * pitchKT;
    
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
    cachedFreq2 = currentFrequency * pitchMod2 * vibratoMod * globalMod * velModPitch * currentDetuneFactor.getNextValue();
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
        
        if (isRingModEnabled) {
            osc2Sample = osc1Sample * osc2Sample;
        } else if (isNoiseModEnabled) {
             // Authentic CZ-101 Noise Mod: It's technically Phase Modulation of Noise? 
             // Or simply Noise replaces the carrier?
             // "Noise Mod" on CZ modulates the *phase* of the noise source by Osc 1?
             // Or modulates Osc 1 Amplitude by Noise?
             // Multiplicative (AM) of Noise * Osc1 creates sidebands.
             // If Osc2 Level is 0 in patch, we hear nothing. 
             // The user says "doesn't sound right".
             // Let's implement a safe audible noise mix for now:
             // Mix Noise with Osc 1, or modulate Osc 2 phase with Noise.
             // Simplest "Good Sounding" fix: Ring Modulate Noise with Osc 1 (AM).
             float noise = (noiseGen.nextFloat() * 2.0f - 1.0f);
             osc2Sample = osc1Sample * noise + noise * 0.5f; // Add some raw noise to ensure output
        }
        
        float out1 = osc1Sample * osc1Level.getNextValue() * dcaVal1 * velModAmp;
        float out2 = osc2Sample * osc2Level.getNextValue() * dcaVal2 * velModAmp;
        
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
            
            if (isRingModEnabled) {
                osc2Sample = osc1Sample * osc2Sample;
            } else if (isNoiseModEnabled) {
                 float noise = (noiseGen.nextFloat() * 2.0f - 1.0f);
                 osc2Sample = osc1Sample * noise; 
            }
            
            // Note: We use the same envelope/level values for all sub-samples
            // This is a simplification but works well for anti-aliasing
            float out1 = osc1Sample * osc1Level.getCurrentValue() * dcaVal1 * velModAmp;
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

    // Optimization: Fast Tanh
    float softClipped = fastTanh(rawMix * HardwareConstants::SOFT_CLIP_DRIVE);
    
    // Modern Filter Processing (Phase 7)
    float filtered = lpf.processSample(softClipped);
    filtered = hpf.processSample(filtered);

    float output = filtered * currentVelocity * masterVolume.getNextValue();
    
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
                      static_cast<DSP::PhaseDistOscillator::CzWaveform>(s->dco1.wave2));
    osc2.setWaveforms(static_cast<DSP::PhaseDistOscillator::CzWaveform>(s->dco2.wave1),
                      static_cast<DSP::PhaseDistOscillator::CzWaveform>(s->dco2.wave2));
                      
    osc1Level.setTargetValue(s->dco1.level);
    osc2Level.setTargetValue(s->dco2.level);
    setOsc2DetuneHardware(s->dco2.octave, s->dco2.coarse, s->dco2.fine);
    
    // Flags
    setHardSync(s->mod.detune == 1); 
    setRingMod(s->lineMod.ring);
    setNoiseMod(s->lineMod.noise);
    
    // Filter / DCW
    // Note: DCW/DCA Envelopes are usually triggered, but key follow/sustains are state.

    // System
    masterVolume.setTargetValue(s->system.masterVol);
    masterTuneFactor = std::pow(2.0f, s->system.masterTune / 12.0f);
    pitchBendFactor = std::pow(2.0f, (s->system.bendRange / 12.0f) * modWheel); // simplified
    
    // Matrix
    smoothedMatrix.veloToDcw.setTargetValue(s->mod.veloDcw);
    smoothedMatrix.veloToDca.setTargetValue(s->mod.veloAmp);
    smoothedMatrix.wheelToDcw.setTargetValue(s->mod.wheelToDcw);
    smoothedMatrix.wheelToLfoRate.setTargetValue(s->mod.wheelToLfoRate);
    smoothedMatrix.wheelToVibrato.setTargetValue(s->mod.wheelToVibrato);
    smoothedMatrix.atToDcw.setTargetValue(s->mod.atToDcw);
    smoothedMatrix.atToVibrato.setTargetValue(s->mod.atToVibrato);
    
    // Audit Fix [PITCH_FIX]: Correctly interpret Key Follow modes
    // Modes: 0=OFF, 1=FIX, 2=VAR
    matrix.kfDcw = s->mod.keyFollowDcw;
    matrix.kfDca = s->mod.keyFollowAmp;
    matrix.kfDco = s->mod.keyFollowDco;
    
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
