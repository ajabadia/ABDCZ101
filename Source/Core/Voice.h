#pragma once

#include "../DSP/Oscillators/PhaseDistOsc.h"
#include "../DSP/Envelopes/MultiStageEnv.h"
#include "../DSP/Modulation/LFO.h"
#include "../DSP/VelocitySensitivityCurves.h" // [NEW]
#include "../DSP/Filters/ResonantFilter.h" // [NEW]
#include <array>
#include <cstdint>

namespace CZ101 {

namespace Core {

struct ParameterSnapshot; // Forward declaration (Correct Namespace)

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
    
    // Phase 7: Snapshot System
    void applySnapshot(const ParameterSnapshot* snapshot) noexcept;
    
    void setSampleRate(double sampleRate) noexcept;

    // Audit Fix [2.2]: Model Selection
    void setModel(DSP::MultiStageEnvelope::Model newModel) noexcept;

    // Global envelope rate multiplier from the "Tone" performance macro
    // (combined with the per-note velocity attack response at noteOn).
    void setToneRateScale(float scale) noexcept;
    
    // Note control
    void noteOn(int midiNote, float velocity) noexcept;
    void noteOff() noexcept;
    void reset() noexcept;
    
    // Oscillator 1 parameters
    // Oscillator 1 parameters
    void setOsc1Waveforms(DSP::PhaseDistOscillator::CzWaveform first, DSP::PhaseDistOscillator::CzWaveform second, DSP::PhaseDistOscillator::CzWindow window = DSP::PhaseDistOscillator::WIN_NONE) noexcept;
    void setOsc1Level(float level) noexcept;
    
    // Oscillator 2 parameters
    void setOsc2Waveforms(DSP::PhaseDistOscillator::CzWaveform first, DSP::PhaseDistOscillator::CzWaveform second, DSP::PhaseDistOscillator::CzWindow window = DSP::PhaseDistOscillator::WIN_NONE) noexcept;
    void setOsc2Level(float level) noexcept;
    void setOsc2Detune(float semitones) noexcept;  // -12 to +12 semitones (APVTS OSC2_DETUNE)
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
    void setLineModulation(int mode) noexcept;
    void setModSpecial(bool enabled) noexcept;

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
        
        int opMode = 0; // 0:101, 1:5000, 2:CZ-1, 3:Modern
        float line1VeloPitch = 0.0f, line1VeloDcw = 0.0f, line1VeloDca = 0.0f;
        float line2VeloPitch = 0.0f, line2VeloDcw = 0.0f, line2VeloDca = 0.0f;
        
        float line1KfPitch = 0.0f, line1KfDcw = 0.0f, line1KfDca = 0.0f;
        float line2KfPitch = 0.0f, line2KfDcw = 0.0f, line2KfDca = 0.0f;

        // ── Free matrix (ABDEEP-style): 8 slots, each a Source → Dest → Depth
        //    routing chosen by the user instead of the fixed hardware routes.
        //    Sources: 0=None 1=Velocity 2=ModWheel 3=Aftertouch 4=KeyTrack
        //             5=LFO 6=EnvDCW 7=EnvDCA 8=EnvPitch(bipolar)
        //             9=PitchBend 10=Noise 11=AuthKeyTrack (hardware curve)
        //    Dests:   0=None 1=DCW 2=DCA 3=Pitch 4=VibratoDepth 5=LFORate
        //             6=Osc2Detune(semitones) 7=Pan
        struct ModSlot {
            int source = 0;
            int dest = 0;
            float depth = 0.0f; // bipolar -1..1
        };
        static constexpr int kNumModSlots = 8;
        ModSlot slots[kNumModSlots];
    };
    void setModulationMatrix(const ModulationMatrix& m) noexcept;
    float getModSlotSourceValue(int source) const noexcept;
    float getModSlotContribution(int dest) const noexcept;
    // Current per-voice pan position (0=L .. 1=R) driven by free-matrix dest 7.
    float getPan() const noexcept { return panValue.getCurrentValue(); }
    
    // Phase 5.1: Oversampling
    void setOversamplingFactor(int factor) noexcept { oversamplingFactor = juce::jlimit(1, 4, factor); }
    
    // Phase 9
    void setHardwareNoiseEnabled(bool enabled) noexcept { hardwareNoiseEnabled = enabled; }

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
    float applyLineModulation(float osc1Sample, float osc2Sample, bool osc1Wrapped) noexcept;

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
    int lineModulation = 0; // 0=Off, 1=Ring1, 2=Noise1, 3=Ring2, 4=Ring3, 5=Noise2
    bool modSpecial = false; // SYSEX-only: mute Line 1, only modulated output
    float currentNoiseVal = 0.0f;
    float modDetuneDelay = 0.0f; // previous osc1Sample, for Ring 2/3 detuned flavor
    
    float glideTime = 0.0f;
    float currentFrequency = 440.0f;
    float targetFrequency = 440.0f;
    
    // LFO State
    DSP::LFO lfoModule;
    juce::LinearSmoothedValue<float> vibratoDepth { 0.0f };
    float lastLfoValue = 0.0f; // cached once per control-rate block (free matrix source 5)
    
    // Pitch Bend factors
    juce::LinearSmoothedValue<float> pitchBendFactor { 1.0f };
    float pitchBendSemitones = 0.0f; // raw semitones, free-matrix source 9
    juce::LinearSmoothedValue<float> masterTuneFactor { 1.0f };

    // Free-matrix state
    juce::LinearSmoothedValue<float> panValue { 0.5f }; // per-voice pan (dest 7), center by default
    float lastNoiseValue = 0.0f; // cached once per control-rate block (source 10)
    
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
    
    // Phase 9: Authentic Noise
    bool hardwareNoiseEnabled = false;
    float getAuthenticNoise(int note, float dcwLevel) noexcept;
    juce::Random noiseGen;

    void setMasterBend(float semitones) noexcept { pitchBendFactor.setTargetValue(std::exp2(semitones / 12.0f)); }

    int64_t lastNoteOnTime = 0; // [NEW] For Voice Stealing
    
    // Rendering Helpers (Refactoring Phase 8)
    float toneRateScale = 1.0f; // [NEW] Tone macro envelope rate multiplier
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
    float dcwVal1 = 0.0f, dcaVal1 = 0.0f, pitchVal1 = 0.5f;
    float dcwVal2 = 0.0f, dcaVal2 = 0.0f, pitchVal2 = 0.5f;
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
