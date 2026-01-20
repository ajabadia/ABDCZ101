#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <map>
#include <string>

namespace CZ101 {
namespace State {

class Parameters
{
public:
    Parameters(juce::AudioProcessor& processor, juce::UndoManager* undoManager = nullptr);
    
    void createParameters();

    juce::AudioProcessorValueTreeState& getAPVTS() { return *apvts; }
    
    // --- SAFER PARAMETER ACCESS ---
    // Instead of raw pointers, we use getters that check for validity if ever needed,
    // although in JUCE APVTS parameters are generally static after creation.
    juce::AudioParameterChoice* getLineSelect() const { return lineSelect; }
    juce::AudioParameterChoice* getOsc1Waveform() const { return osc1Waveform; }
    juce::AudioParameterChoice* getOsc1Waveform2() const { return osc1Waveform2; }
    juce::AudioParameterFloat* getOsc1Level() const { return osc1Level; }
    juce::AudioParameterChoice* getOsc2Waveform() const { return osc2Waveform; }
    juce::AudioParameterChoice* getOsc2Waveform2() const { return osc2Waveform2; }
    juce::AudioParameterFloat* getOsc2Level() const { return osc2Level; }
    juce::AudioParameterFloat* getOsc2Detune() const { return osc2Detune; }
    juce::AudioParameterFloat* getLineMix() const { return lineMix; } // Audit Fix [2.4]
    juce::AudioParameterInt*   getDetuneOctave() const { return detuneOct; }
    juce::AudioParameterInt*   getDetuneCoarse() const { return detuneCoarse; }
    juce::AudioParameterInt*   getDetuneFine() const { return detuneFine; }
    juce::AudioParameterBool*  getHardSync() const { return hardSync; }
    juce::AudioParameterBool*  getRingMod() const { return ringMod; }
    juce::AudioParameterBool*  getNoiseMod() const { return noiseMod; } // Audit Fix
    juce::AudioParameterFloat* getGlideTime() const { return glideTime; }

    juce::AudioParameterFloat* getMasterVolume() const { return masterVolume; }
    juce::AudioParameterBool*  getSystemPrg() const { return systemPrg; }
    juce::AudioParameterBool*  getBypass() const { return bypass; }
    juce::AudioParameterChoice* getOperationMode() const { return operationMode; } // Audit Fix [2.2a]
    
    juce::AudioParameterInt*   getMidiChannel() const { return midiChannel; }
    juce::AudioParameterFloat* getMasterTune() const { return masterTune; }
    juce::AudioParameterInt*   getPitchBendRange() const { return pitchBendRange; }
    juce::AudioParameterInt*   getKeyTranspose() const { return keyTranspose; }
    
    juce::AudioParameterBool*  getProtectSwitch() const { return protectSwitch; } // Audit Fix [2.5]
    juce::AudioParameterBool*  getHardwareNoise() const { return hardwareNoise; }

    // Envelopes
    juce::AudioParameterFloat* getDcaAttack() const { return dcaAttack; }
    juce::AudioParameterFloat* getDcaDecay() const { return dcaDecay; }
    juce::AudioParameterFloat* getDcaSustain() const { return dcaSustain; }
    juce::AudioParameterFloat* getDcaRelease() const { return dcaRelease; }
    juce::AudioParameterFloat* getDcwAttack() const { return dcwAttack; }
    juce::AudioParameterFloat* getDcwDecay() const { return dcwDecay; }
    juce::AudioParameterFloat* getDcwSustain() const { return dcwSustain; }
    juce::AudioParameterFloat* getDcwRelease() const { return dcwRelease; }

    // LFO / Effects
    juce::AudioParameterChoice* getLfoWaveform() const { return lfoWaveform; }
    juce::AudioParameterFloat*  getLfoRate() const { return lfoRate; }
    juce::AudioParameterFloat*  getLfoDepth() const { return lfoDepth; }
    juce::AudioParameterFloat*  getLfoDelay() const { return lfoDelay; }
    
    // Drive
    juce::AudioParameterFloat*  getDriveAmount() const { return driveAmount; }
    juce::AudioParameterFloat*  getDriveColor() const { return driveColor; }
    juce::AudioParameterFloat*  getDriveMix() const { return driveMix; }

    juce::AudioParameterFloat*  getChorusRate() const { return chorusRate; }
    juce::AudioParameterFloat*  getChorusDepth() const { return chorusDepth; }
    juce::AudioParameterFloat*  getChorusMix() const { return chorusMix; }
    juce::AudioParameterFloat*  getDelayTime() const { return delayTime; }
    juce::AudioParameterFloat*  getDelayFeedback() const { return delayFeedback; }
    juce::AudioParameterFloat*  getDelayMix() const { return delayMix; }
    juce::AudioParameterFloat*  getReverbSize() const { return reverbSize; }
    juce::AudioParameterFloat*  getReverbMix() const { return reverbMix; }

    // Modern / Matrix
    juce::AudioParameterFloat* getModernLpfCutoff() const { return modernLpfCutoff; }
    juce::AudioParameterFloat* getModernLpfReso() const { return modernLpfReso; }
    juce::AudioParameterFloat* getModernHpfCutoff() const { return modernHpfCutoff; }
    juce::AudioParameterFloat* getModVeloToDcw() const { return modVeloToDcw; }
    juce::AudioParameterFloat* getModVeloToDca() const { return modVeloToDca; }
    juce::AudioParameterFloat* getModWheelToDcw() const { return modWheelToDcw; }
    juce::AudioParameterFloat* getModWheelToLfoRate() const { return modWheelToLfoRate; }
    juce::AudioParameterFloat* getModWheelToVibrato() const { return modWheelToVibrato; }
    juce::AudioParameterFloat* getModAtToDcw() const { return modAtToDcw; }
    juce::AudioParameterFloat* getModAtToVibrato() const { return modAtToVibrato; }
    juce::AudioParameterFloat* getKeyTrackDcw() const { return keyTrackDcw; }
    juce::AudioParameterFloat* getKeyTrackPitch() const { return keyTrackPitch; }
    juce::AudioParameterChoice* getKeyFollowDco() const { return keyFollowDco; }
    juce::AudioParameterChoice* getKeyFollowDcw() const { return keyFollowDcw; }
    juce::AudioParameterChoice* getKeyFollowDca() const { return keyFollowDca; }
    
    // Phase 5.1: Oversampling Quality
    juce::AudioParameterChoice* getOversamplingQuality() const { return oversamplingQuality; }

    juce::RangedAudioParameter* getParameter(const juce::String& paramId) const;
    const std::map<juce::String, juce::RangedAudioParameter*>& getParameterMap() const { return parameterMap; }
    
private:
    juce::AudioProcessor& audioProcessor;
    std::unique_ptr<juce::AudioProcessorValueTreeState> apvts;
    std::map<juce::String, juce::RangedAudioParameter*> parameterMap;

    // Raw Pointers (Now private to encourage use of getters)
    juce::AudioParameterChoice* lineSelect = nullptr;
    juce::AudioParameterChoice* osc1Waveform = nullptr;
    juce::AudioParameterChoice* osc1Waveform2 = nullptr;
    juce::AudioParameterFloat* osc1Level = nullptr;
    juce::AudioParameterChoice* osc2Waveform = nullptr;
    juce::AudioParameterChoice* osc2Waveform2 = nullptr;
    juce::AudioParameterFloat* osc2Level = nullptr;
    juce::AudioParameterFloat* osc2Detune = nullptr;
    // Audit Fix [2.4]: Tone Mix
    juce::AudioParameterFloat* lineMix = nullptr;
    juce::AudioParameterInt*   detuneOct = nullptr;
    juce::AudioParameterInt*   detuneCoarse = nullptr;
    juce::AudioParameterInt*   detuneFine = nullptr;
    juce::AudioParameterBool* hardSync = nullptr;
    juce::AudioParameterBool* ringMod = nullptr;
    juce::AudioParameterBool* noiseMod = nullptr; // Audit Fix
    juce::AudioParameterFloat* glideTime = nullptr;

    juce::AudioParameterChoice* lfoWaveform = nullptr;
    juce::AudioParameterFloat* lfoRate = nullptr;
    juce::AudioParameterFloat* lfoDepth = nullptr;
    juce::AudioParameterFloat* lfoDelay = nullptr;

    juce::AudioParameterFloat* driveAmount = nullptr;
    juce::AudioParameterFloat* driveColor = nullptr;
    juce::AudioParameterFloat* driveMix = nullptr;

    juce::AudioParameterFloat* chorusRate = nullptr;
    juce::AudioParameterFloat* chorusDepth = nullptr;
    juce::AudioParameterFloat* chorusMix = nullptr;
    juce::AudioParameterFloat* delayTime = nullptr;
    juce::AudioParameterFloat* delayFeedback = nullptr;
    juce::AudioParameterFloat* delayMix = nullptr;
    juce::AudioParameterFloat* reverbSize = nullptr;
    juce::AudioParameterFloat* reverbMix = nullptr;

    juce::AudioParameterFloat* dcaAttack = nullptr;
    juce::AudioParameterFloat* dcaDecay = nullptr;
    juce::AudioParameterFloat* dcaSustain = nullptr;
    juce::AudioParameterFloat* dcaRelease = nullptr;

    juce::AudioParameterFloat* dcwAttack = nullptr;
    juce::AudioParameterFloat* dcwDecay = nullptr;
    juce::AudioParameterFloat* dcwSustain = nullptr;
    juce::AudioParameterFloat* dcwRelease = nullptr;

    juce::AudioParameterBool* protectSwitch = nullptr;
    juce::AudioParameterBool* systemPrg = nullptr;
    juce::AudioParameterFloat* masterVolume = nullptr;
    juce::AudioParameterBool*  hardwareNoise = nullptr;
    juce::AudioParameterBool*  bypass = nullptr;

    juce::AudioParameterChoice* operationMode = nullptr; // Audit Fix [2.2a]: Unified Mode

    juce::AudioParameterFloat* modernLpfCutoff = nullptr;
    juce::AudioParameterFloat* modernLpfReso = nullptr;
    juce::AudioParameterFloat* modernHpfCutoff = nullptr;

    juce::AudioParameterFloat* modVeloToDcw = nullptr;
    juce::AudioParameterFloat* modVeloToDca = nullptr;
    juce::AudioParameterFloat* modWheelToDcw = nullptr;
    juce::AudioParameterFloat* modWheelToLfoRate = nullptr;
    juce::AudioParameterFloat* modWheelToVibrato = nullptr;
    juce::AudioParameterFloat* modAtToDcw = nullptr;
    juce::AudioParameterFloat* modAtToVibrato = nullptr;

    juce::AudioParameterFloat* keyTrackDcw = nullptr;
    juce::AudioParameterFloat* keyTrackPitch = nullptr;
    juce::AudioParameterChoice* keyFollowDco = nullptr;
    juce::AudioParameterChoice* keyFollowDcw = nullptr;
    juce::AudioParameterChoice* keyFollowDca = nullptr;
    
    // Phase 5.1: Oversampling
    juce::AudioParameterChoice* oversamplingQuality = nullptr;

    juce::AudioParameterInt*   midiChannel = nullptr;
    juce::AudioParameterFloat* masterTune = nullptr;
    juce::AudioParameterInt*   pitchBendRange = nullptr;
    juce::AudioParameterInt*   keyTranspose = nullptr;

    // Arpeggiator Pointers
    juce::AudioParameterBool* arpEnabled = nullptr;
    juce::AudioParameterBool* arpLatch = nullptr;
    juce::AudioParameterChoice* arpRate = nullptr;
    juce::AudioParameterFloat*  arpBpm = nullptr;
    juce::AudioParameterFloat*  arpGate = nullptr;
    juce::AudioParameterFloat*  arpSwing = nullptr;
    juce::AudioParameterChoice* arpSwingMode = nullptr;
    juce::AudioParameterChoice* arpPattern = nullptr;
    juce::AudioParameterInt*   arpOctave = nullptr;

    // Macro Pointers
    juce::AudioParameterFloat* macroBrilliance = nullptr;
    juce::AudioParameterFloat* macroTone = nullptr;
    juce::AudioParameterFloat* macroSpace = nullptr;

public:
    // Arpeggiator getters
    juce::AudioParameterBool*  getArpEnabled() const { return arpEnabled; }
    juce::AudioParameterBool*  getArpLatch() const { return arpLatch; }
    juce::AudioParameterChoice* getArpRate() const { return arpRate; }
    juce::AudioParameterFloat* getArpBpm() const { return arpBpm; }
    juce::AudioParameterFloat* getArpGate() const { return arpGate; }
    juce::AudioParameterFloat* getArpSwing() const { return arpSwing; }
    juce::AudioParameterChoice* getArpSwingMode() const { return arpSwingMode; }
    juce::AudioParameterChoice* getArpPattern() const { return arpPattern; }
    juce::AudioParameterInt*   getArpOctave() const { return arpOctave; }

    // Macro Controls
    juce::AudioParameterFloat* getMacroBrilliance() const { return macroBrilliance; }
    juce::AudioParameterFloat* getMacroTone() const { return macroTone; }
    juce::AudioParameterFloat* getMacroSpace() const { return macroSpace; }
};

} // namespace State
} // namespace CZ101
