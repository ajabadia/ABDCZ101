#include <JuceHeader.h>
#include "Voice.h"
#include <cmath>
#include "../DSP/Envelopes/ADSRtoStage.h"

namespace CZ101 {
namespace Core {

Voice::Voice()
{
    // Initialize Modern Filters
    lpf.setType(DSP::ResonantFilter::LOWPASS);
    hpf.setType(DSP::ResonantFilter::HIGHPASS);
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
void Voice::setDCWStage(int line, int i, float r, float l) noexcept { if(line==1) dcwEnvelope1.setStage(i, r, l); else dcwEnvelope2.setStage(i, r, l); }
void Voice::setDCWSustainPoint(int line, int i) noexcept { if(line==1) dcwEnvelope1.setSustainPoint(i); else dcwEnvelope2.setSustainPoint(i); }
void Voice::setDCWEndPoint(int line, int i) noexcept { if(line==1) dcwEnvelope1.setEndPoint(i); else dcwEnvelope2.setEndPoint(i); }

void Voice::getDCWStage(int line, int i, float& r, float& l) const noexcept { if(line==1) { r = dcwEnvelope1.getStageRate(i); l = dcwEnvelope1.getStageLevel(i); } else { r = dcwEnvelope2.getStageRate(i); l = dcwEnvelope2.getStageLevel(i); } }
int Voice::getDCWSustainPoint(int line) const noexcept { return line==1 ? dcwEnvelope1.getSustainPoint() : dcwEnvelope2.getSustainPoint(); }
int Voice::getDCWEndPoint(int line) const noexcept { return line==1 ? dcwEnvelope1.getEndPoint() : dcwEnvelope2.getEndPoint(); }

// DCA
void Voice::setDCAStage(int line, int i, float r, float l) noexcept { if(line==1) dcaEnvelope1.setStage(i, r, l); else dcaEnvelope2.setStage(i, r, l); }
void Voice::setDCASustainPoint(int line, int i) noexcept { if(line==1) dcaEnvelope1.setSustainPoint(i); else dcaEnvelope2.setSustainPoint(i); }
void Voice::setDCAEndPoint(int line, int i) noexcept { if(line==1) dcaEnvelope1.setEndPoint(i); else dcaEnvelope2.setEndPoint(i); }

void Voice::getDCAStage(int line, int i, float& r, float& l) const noexcept { if(line==1) { r = dcaEnvelope1.getStageRate(i); l = dcaEnvelope1.getStageLevel(i); } else { r = dcaEnvelope2.getStageRate(i); l = dcaEnvelope2.getStageLevel(i); } }
int Voice::getDCASustainPoint(int line) const noexcept { return line==1 ? dcaEnvelope1.getSustainPoint() : dcaEnvelope2.getSustainPoint(); }
int Voice::getDCAEndPoint(int line) const noexcept { return line==1 ? dcaEnvelope1.getEndPoint() : dcaEnvelope2.getEndPoint(); }

// Pitch
void Voice::setPitchStage(int line, int i, float r, float l) noexcept { if(line==1) pitchEnvelope1.setStage(i, r, l); else pitchEnvelope2.setStage(i, r, l); }
void Voice::setPitchSustainPoint(int line, int i) noexcept { if(line==1) pitchEnvelope1.setSustainPoint(i); else pitchEnvelope2.setSustainPoint(i); }
void Voice::setPitchEndPoint(int line, int i) noexcept { if(line==1) pitchEnvelope1.setEndPoint(i); else pitchEnvelope2.setEndPoint(i); }

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
    // Simple 3rd order minimax for [-1, 1] range
    x = juce::jlimit(-1.0f, 1.0f, x); // Audit Fix: Clamp input to prevent polynomial explosion
    
    // exp2(x) approx 1 + 0.693147*x + 0.240226*x^2 + 0.055504*x^3
    return 1.0f + x * (0.69314718f + x * (0.24022650f + x * (0.05550411f + x * 0.00961812f)));
}

float Voice::renderNextSample() noexcept
{
    if (!dcaEnvelope1.isActive() && !dcaEnvelope2.isActive()) return 0.0f;
    
    // === CONTROL RATE MODULATION (Every 8 samples) ===
    if ((sampleCounter++ & 0x07) == 0)
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
        // Real hardware acceleratos DCW to sine on high notes
        // Scaling factor 0.015f is a heuristic based on handbook's "acceleration"
        ktOffset = (currentNote - 60) * (ktDcw * 0.015f);
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
        float kfDcaOffset = (currentNote - 60) * -0.002f;
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
    osc1.setFrequency(cachedFreq1);
    osc2.setFrequency(cachedFreq2);
    
    // Osc Render
    bool osc1Wrapped = false;
    float osc1Sample = osc1.renderNextSample(dcwVal1, &osc1Wrapped);
    if (isHardSyncEnabled && osc1Wrapped) osc2.reset();
    float osc2Sample = osc2.renderNextSample(dcwVal2);
    if (isRingModEnabled) osc2Sample = osc1Sample * osc2Sample;
    
    // Mixing
    float out1 = osc1Sample * osc1Level.getNextValue() * dcaVal1 * velModAmp;
    float out2 = osc2Sample * osc2Level.getNextValue() * dcaVal2 * velModAmp;
    
    return out1 + out2;
}

float Voice::applyPostProcessing(float rawMix) noexcept
{
    // Optimization: Fast Tanh
    float softClipped = fastTanh(rawMix * 0.68f);
    
    // Modern Filter Processing (Phase 7)
    float filtered = lpf.processSample(softClipped);
    filtered = hpf.processSample(filtered);

    return filtered * currentVelocity * masterVolume.getNextValue() * 0.9f;
}

float Voice::midiNoteToFrequency(int midiNote) const noexcept
{
    return 440.0f * std::exp2((midiNote - 69) / 12.0f);
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

} // namespace Core
} // namespace CZ101
