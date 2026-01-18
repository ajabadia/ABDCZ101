#pragma once

#include "../DSP/Oscillators/PhaseDistOsc.h"
#include "../DSP/Envelopes/MultiStageEnv.h"
#include "../DSP/Modulation/LFO.h"
#include "../DSP/Envelopes/MultiStageEnv.h"
#include "../DSP/Modulation/LFO.h"
#include "../DSP/VelocitySensitivityCurves.h" // [NEW]
#include "../DSP/Filters/ResonantFilter.h" // [NEW]
#include <array>
#include <array>
#include <cstdint>

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

    // Audit Fix [2.2]: Model Selection
    void setModel(DSP::MultiStageEnvelope::Model newModel) noexcept;
    
    // Note control
    void noteOn(int midiNote, float velocity) noexcept;
    void noteOff() noexcept;
    void reset() noexcept;
    
    // Oscillator 1 parameters
    // Oscillator 1 parameters
    void setOsc1Waveforms(DSP::PhaseDistOscillator::CzWaveform first, DSP::PhaseDistOscillator::CzWaveform second) noexcept;
    void setOsc1Level(float level) noexcept;
    
    // Oscillator 2 parameters
    void setOsc2Waveforms(DSP::PhaseDistOscillator::CzWaveform first, DSP::PhaseDistOscillator::CzWaveform second) noexcept;
    void setOsc2Level(float level) noexcept;
    void setOsc2Detune(float cents) noexcept;  // -100 to +100 cents
    void setOsc2DetuneHardware(int oct, int coarse, int fineCents) noexcept;
    
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
    void setGlideTime(float seconds) noexcept;

    // --- Modulation (LFO) ---
    void setVibratoDepth(float semitones) noexcept;
    
    // Config LFO (Per-Voice)
    void setLFOFrequency(float hz) noexcept;
    void setLFOWaveform(DSP::LFO::Waveform waveform) noexcept;
    void setLFODelay(float seconds) noexcept;
    
    // --- Global Pitch ---
    void setPitchBend(float semitones) noexcept;
    void setMasterTune(float semitones) noexcept;
    void setMasterVolume(float level) noexcept;

    // --- Modern Filter (Phase 7) ---
    void setFilterCutoff(float frequency) noexcept { lpf.setCutoff(frequency); }
    void setFilterResonance(float reso) noexcept { lpf.setResonance(reso); }
    void setHPF(float frequency) noexcept { hpf.setCutoff(frequency); }
    
    // --- Modulation Sources ---
    void setModWheel(float value) noexcept { modWheel = value; }
    void setAftertouch(float value) noexcept { aftertouch = value; }
    
    struct ModulationMatrix {
        float veloToDcw = 0.0f;
        float veloToDca = 1.0f;
        float wheelToDcw = 0.0f;
        float wheelToLfoRate = 0.0f;
        float wheelToVibrato = 0.0f;
        float atToDcw = 0.0f;
        float atToVibrato = 0.0f;
        float keyTrackDcw = 0.0f;
        float keyTrackPitch = 1.0f;
        int kfDco = 0; // 0:OFF, 1:FIX, 2:VAR
        int kfDcw = 0;
        int kfDca = 0;
    };
    void setModulationMatrix(const ModulationMatrix& m) noexcept;
    
    // Phase 5.1: Oversampling
    void setOversamplingFactor(int factor) noexcept { oversamplingFactor = juce::jlimit(1, 4, factor); }

    // --- Pitch Envelope Controls ---
    
    // DCW Envelope (Legacy ADSR Wrappers)
    void setDCWAttack(float seconds) noexcept;
    void setDCWDecay(float seconds) noexcept;
    void setDCWSustain(float level) noexcept;
    void setDCWRelease(float seconds) noexcept;
    
    // DCW 8-Stage Control
    void setDCWStage(int line, int index, float rate, float level) noexcept;
    void setDCWSustainPoint(int line, int index) noexcept;
    void setDCWEndPoint(int line, int index) noexcept;
    
    void getDCWStage(int line, int index, float& rate, float& level) const noexcept;
    int getDCWSustainPoint(int line) const noexcept;
    int getDCWEndPoint(int line) const noexcept;
    
    // DCA 8-Stage Control
    void setDCAStage(int line, int index, float rate, float level) noexcept;
    void setDCASustainPoint(int line, int index) noexcept;
    void setDCAEndPoint(int line, int index) noexcept;

    void getDCAStage(int line, int index, float& rate, float& level) const noexcept;
    int getDCASustainPoint(int line) const noexcept;
    int getDCAEndPoint(int line) const noexcept;


    // DCA Envelope (Legacy ADSR Wrappers)
    void setDCAAttack(float seconds) noexcept;
    void setDCADecay(float seconds) noexcept;
    void setDCASustain(float level) noexcept;
    void setDCARelease(float seconds) noexcept;

    // Pitch Envelope (DCO) 8-Stage Control
    void setPitchStage(int line, int index, float rate, float level) noexcept;
    void setPitchSustainPoint(int line, int index) noexcept;
    void setPitchEndPoint(int line, int index) noexcept;
    
    void getPitchStage(int line, int index, float& rate, float& level) const noexcept;
    int getPitchSustainPoint(int line) const noexcept;
    int getPitchEndPoint(int line) const noexcept;
    
    // Rendering
    float renderNextSample() noexcept;
    
    bool isActive() const noexcept { return dcaEnvelope1.isActive() || dcaEnvelope2.isActive(); }
    bool isReleasing() const noexcept { return dcaEnvelope1.isReleased() || dcaEnvelope2.isReleased(); }
    int getCurrentNote() const noexcept { return currentNote; }
    int64_t getLastNoteOnTime() const noexcept { return lastNoteOnTime; }
    
private:
    // Oscillators
    DSP::PhaseDistOscillator osc1;
    DSP::PhaseDistOscillator osc2;
    
    // Modern Filters
    DSP::ResonantFilter lpf;
    DSP::ResonantFilter hpf;
    
    // Envelopes
    DSP::MultiStageEnvelope dcwEnvelope1;  // Timbre Line 1
    DSP::MultiStageEnvelope dcaEnvelope1;  // Volume Line 1
    DSP::MultiStageEnvelope pitchEnvelope1; // Pitch Line 1
    
    DSP::MultiStageEnvelope dcwEnvelope2;  // Timbre Line 2
    DSP::MultiStageEnvelope dcaEnvelope2;  // Volume Line 2
    DSP::MultiStageEnvelope pitchEnvelope2; // Pitch Line 2
    
    // State
    int currentNote = -1;
    float currentVelocity = 1.0f;
    
    // Mix levels (Smoothed)
    juce::LinearSmoothedValue<float> osc1Level { 0.5f };
    juce::LinearSmoothedValue<float> osc2Level { 0.5f };
    juce::LinearSmoothedValue<float> osc2Detune { 0.0f };
    
    // Pitch Modulation State (Optimization)
    float baseFrequency = 440.0f;
    juce::LinearSmoothedValue<float> currentDetuneFactor { 1.0f };
    
    bool isHardSyncEnabled = false;
    bool isRingModEnabled = false;
    
    float glideTime = 0.0f;
    float currentFrequency = 440.0f;
    float targetFrequency = 440.0f;
    
    // LFO State
    DSP::LFO lfoModule;
    float vibratoDepth = 0.0f;
    
    // Pitch Bend factors
    float pitchBendFactor = 1.0f;
    float masterTuneFactor = 1.0f;
    
    // Velocity Sensitivity [NEW]
    CZ101::DSP::VelocityCurve velocityCurve;
    float velModAmp = 1.0f;
    float velModPitch = 1.0f;
    float velModAttack = 1.0f;
    float velModDcw = 1.0f;
    float velModVibDepth = 1.0f;
    
    // Internal cache
    // Modulation Sources
    float modWheel = 0.0f;
    float aftertouch = 0.0f;
    ModulationMatrix matrix;

    void setMasterBend(float semitones) noexcept { pitchBendFactor = std::exp2(semitones / 12.0f); }

    int64_t lastNoteOnTime = 0; // [NEW] For Voice Stealing
    
    // Rendering Helpers (Refactoring Phase 8)
    void processControlRate() noexcept;
    void calculateEnvelopeValues() noexcept;
    void calculateLFOAndVibrato() noexcept;
    void calculateDCWModulation() noexcept;
    void calculateDCAModulation() noexcept;
    void calculatePitchModulation() noexcept;

    float renderOscillators() noexcept;
    float applyPostProcessing(float rawMix) noexcept;

    // Helper
    float midiNoteToFrequency(int midiNote) const noexcept;

private:
    juce::LinearSmoothedValue<float> masterVolume { 1.0f };
    
    // Phase 5.1: Oversampling
    int oversamplingFactor = 1; // 1x, 2x, or 4x

private:
    // ===== ADSR STATE (NEW) =====
    struct ADSRParams {
        float attackMs = 10.0f;
        float decayMs = 200.0f;
        float sustainLevel = 0.5f;
        float releaseMs = 100.0f;
    };
    
    ADSRParams dcwADSR1, dcwADSR2;
    ADSRParams dcaADSR1, dcaADSR2;
    ADSRParams pitchADSR1, pitchADSR2;
    
    // Helper to update envelopes from ADSR using stored sampleRate
    void updateDCWEnvelopeFromADSR(int line) noexcept;
    void updateDCAEnvelopeFromADSR(int line) noexcept;
    void updatePitchEnvelopeFromADSR(int line) noexcept;

    double sampleRate = 44100.0;
    
    // === OPTIMIZATION STATE ===
    uint32_t sampleCounter = 0;
    float cachedFreq1 = 440.0f;
    float cachedFreq2 = 440.0f;
    float dcwVal1 = 0.0f, dcaVal1 = 0.0f;
    float dcwVal2 = 0.0f, dcaVal2 = 0.0f;
    float pitchMod1 = 1.0f, pitchMod2 = 1.0f;
    float vibratoMod = 1.0f;

    struct SmoothedModulationMatrix {
        juce::LinearSmoothedValue<float> veloToDcw { 0.0f };
        juce::LinearSmoothedValue<float> veloToDca { 1.0f };
        juce::LinearSmoothedValue<float> wheelToDcw { 0.0f };
        juce::LinearSmoothedValue<float> wheelToLfoRate { 0.0f };
        juce::LinearSmoothedValue<float> wheelToVibrato { 0.0f };
        juce::LinearSmoothedValue<float> atToDcw { 0.0f };
        juce::LinearSmoothedValue<float> atToVibrato { 0.0f };
        juce::LinearSmoothedValue<float> keyTrackDcw { 0.0f };
        juce::LinearSmoothedValue<float> keyTrackPitch { 1.0f };
        // Integer switches don't need smoothing
    } smoothedMatrix;
    
};
} // namespace Core
} // namespace CZ101
