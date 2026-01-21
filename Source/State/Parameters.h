#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "ParameterIDs.h"
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
    // Instead of raw pointers, we fetch directly from APVTS using ParameterIDs.
    juce::AudioParameterChoice* getLineSelect() const        { return getParam<juce::AudioParameterChoice>(ParameterIDs::lineSelect); }
    juce::AudioParameterChoice* getOsc1Waveform() const      { return getParam<juce::AudioParameterChoice>(ParameterIDs::osc1Waveform); }
    juce::AudioParameterChoice* getOsc1Waveform2() const     { return getParam<juce::AudioParameterChoice>(ParameterIDs::osc1Waveform2); }
    juce::AudioParameterFloat*  getOsc1Level() const          { return getParam<juce::AudioParameterFloat>(ParameterIDs::osc1Level); }
    juce::AudioParameterChoice* getOsc2Waveform() const      { return getParam<juce::AudioParameterChoice>(ParameterIDs::osc2Waveform); }
    juce::AudioParameterChoice* getOsc2Waveform2() const     { return getParam<juce::AudioParameterChoice>(ParameterIDs::osc2Waveform2); }
    juce::AudioParameterFloat*  getOsc2Level() const          { return getParam<juce::AudioParameterFloat>(ParameterIDs::osc2Level); }
    juce::AudioParameterFloat*  getOsc2Detune() const         { return getParam<juce::AudioParameterFloat>(ParameterIDs::osc2Detune); }
    juce::AudioParameterFloat*  getLineMix() const            { return getParam<juce::AudioParameterFloat>(ParameterIDs::lineMix); }
    juce::AudioParameterInt*    getDetuneOctave() const       { return getParam<juce::AudioParameterInt>(ParameterIDs::detuneOct); }
    juce::AudioParameterInt*    getDetuneCoarse() const       { return getParam<juce::AudioParameterInt>(ParameterIDs::detuneCoarse); }
    juce::AudioParameterInt*    getDetuneFine() const         { return getParam<juce::AudioParameterInt>(ParameterIDs::detuneFine); }
    juce::AudioParameterBool*   getHardSync() const           { return getParam<juce::AudioParameterBool>(ParameterIDs::hardSync); }
    juce::AudioParameterBool*   getRingMod() const            { return getParam<juce::AudioParameterBool>(ParameterIDs::ringMod); }
    juce::AudioParameterBool*   getNoiseMod() const           { return getParam<juce::AudioParameterBool>(ParameterIDs::noiseMod); }
    juce::AudioParameterFloat*  getGlideTime() const          { return getParam<juce::AudioParameterFloat>(ParameterIDs::glideTime); }

    juce::AudioParameterFloat*  getMasterVolume() const       { return getParam<juce::AudioParameterFloat>(ParameterIDs::masterVolume); }
    juce::AudioParameterBool*   getSystemPrg() const          { return getParam<juce::AudioParameterBool>(ParameterIDs::systemPrg); }
    juce::AudioParameterBool*   getBypass() const             { return getParam<juce::AudioParameterBool>(ParameterIDs::bypass); }
    juce::AudioParameterChoice* getOperationMode() const      { return getParam<juce::AudioParameterChoice>(ParameterIDs::operationMode); }
    
    juce::AudioParameterInt*    getMidiChannel() const        { return getParam<juce::AudioParameterInt>(ParameterIDs::midiChannel); }
    juce::AudioParameterFloat*  getMasterTune() const         { return getParam<juce::AudioParameterFloat>(ParameterIDs::masterTune); }
    juce::AudioParameterInt*    getPitchBendRange() const     { return getParam<juce::AudioParameterInt>(ParameterIDs::benderRange); }
    juce::AudioParameterInt*    getKeyTranspose() const       { return getParam<juce::AudioParameterInt>(ParameterIDs::transpose); }
    
    juce::AudioParameterBool*   getProtectSwitch() const      { return getParam<juce::AudioParameterBool>(ParameterIDs::protectSwitch); }
    juce::AudioParameterBool*   getHardwareNoise() const      { return getParam<juce::AudioParameterBool>(ParameterIDs::hardwareNoise); }

    // Envelopes
    juce::AudioParameterFloat*  getDcaAttack() const          { return getParam<juce::AudioParameterFloat>(ParameterIDs::dcaAttack); }
    juce::AudioParameterFloat*  getDcaDecay() const           { return getParam<juce::AudioParameterFloat>(ParameterIDs::dcaDecay); }
    juce::AudioParameterFloat*  getDcaSustain() const         { return getParam<juce::AudioParameterFloat>(ParameterIDs::dcaSustain); }
    juce::AudioParameterFloat*  getDcaRelease() const         { return getParam<juce::AudioParameterFloat>(ParameterIDs::dcaRelease); }
    juce::AudioParameterFloat*  getDcwAttack() const          { return getParam<juce::AudioParameterFloat>(ParameterIDs::dcwAttack); }
    juce::AudioParameterFloat*  getDcwDecay() const           { return getParam<juce::AudioParameterFloat>(ParameterIDs::dcwDecay); }
    juce::AudioParameterFloat*  getDcwSustain() const         { return getParam<juce::AudioParameterFloat>(ParameterIDs::dcwSustain); }
    juce::AudioParameterFloat*  getDcwRelease() const         { return getParam<juce::AudioParameterFloat>(ParameterIDs::dcwRelease); }

    // LFO / Effects
    juce::AudioParameterChoice* getLfoWaveform() const        { return getParam<juce::AudioParameterChoice>(ParameterIDs::lfoWaveform); }
    juce::AudioParameterFloat*  getLfoRate() const            { return getParam<juce::AudioParameterFloat>(ParameterIDs::lfoRate); }
    juce::AudioParameterFloat*  getLfoDepth() const           { return getParam<juce::AudioParameterFloat>(ParameterIDs::lfoDepth); }
    juce::AudioParameterFloat*  getLfoDelay() const           { return getParam<juce::AudioParameterFloat>(ParameterIDs::lfoDelay); }
    
    // Drive
    juce::AudioParameterFloat*  getDriveAmount() const        { return getParam<juce::AudioParameterFloat>(ParameterIDs::driveAmount); }
    juce::AudioParameterFloat*  getDriveColor() const         { return getParam<juce::AudioParameterFloat>(ParameterIDs::driveColor); }
    juce::AudioParameterFloat*  getDriveMix() const           { return getParam<juce::AudioParameterFloat>(ParameterIDs::driveMix); }

    juce::AudioParameterFloat*  getChorusRate() const         { return getParam<juce::AudioParameterFloat>(ParameterIDs::chorusRate); }
    juce::AudioParameterFloat*  getChorusDepth() const        { return getParam<juce::AudioParameterFloat>(ParameterIDs::chorusDepth); }
    juce::AudioParameterFloat*  getChorusMix() const          { return getParam<juce::AudioParameterFloat>(ParameterIDs::chorusMix); }
    juce::AudioParameterFloat*  getDelayTime() const          { return getParam<juce::AudioParameterFloat>(ParameterIDs::delayTime); }
    juce::AudioParameterFloat*  getDelayFeedback() const      { return getParam<juce::AudioParameterFloat>(ParameterIDs::delayFeedback); }
    juce::AudioParameterFloat*  getDelayMix() const           { return getParam<juce::AudioParameterFloat>(ParameterIDs::delayMix); }
    juce::AudioParameterFloat*  getReverbSize() const         { return getParam<juce::AudioParameterFloat>(ParameterIDs::reverbSize); }
    juce::AudioParameterFloat*  getReverbMix() const          { return getParam<juce::AudioParameterFloat>(ParameterIDs::reverbMix); }

    // Modern / Matrix
    juce::AudioParameterFloat*  getModernLpfCutoff() const    { return getParam<juce::AudioParameterFloat>(ParameterIDs::lpfCutoff); }
    juce::AudioParameterFloat*  getModernLpfReso() const      { return getParam<juce::AudioParameterFloat>(ParameterIDs::lpfReso); }
    juce::AudioParameterFloat*  getModernHpfCutoff() const    { return getParam<juce::AudioParameterFloat>(ParameterIDs::hpfCutoff); }
    juce::AudioParameterFloat*  getModVeloToDcw() const       { return getParam<juce::AudioParameterFloat>(ParameterIDs::modVeloDcw); }
    juce::AudioParameterFloat*  getModVeloToDca() const       { return getParam<juce::AudioParameterFloat>(ParameterIDs::modVeloDca); }
    juce::AudioParameterFloat*  getModWheelToDcw() const      { return getParam<juce::AudioParameterFloat>(ParameterIDs::modWheelDcw); }
    juce::AudioParameterFloat*  getModWheelToLfoRate() const  { return getParam<juce::AudioParameterFloat>(ParameterIDs::modWheelLfoRate); }
    juce::AudioParameterFloat*  getModWheelToVibrato() const  { return getParam<juce::AudioParameterFloat>(ParameterIDs::modWheelVib); }
    juce::AudioParameterFloat*  getModAtToDcw() const         { return getParam<juce::AudioParameterFloat>(ParameterIDs::modAtDcw); }
    juce::AudioParameterFloat*  getModAtToVibrato() const     { return getParam<juce::AudioParameterFloat>(ParameterIDs::modAtVib); }
    juce::AudioParameterFloat*  getKeyTrackDcw() const        { return getParam<juce::AudioParameterFloat>(ParameterIDs::keyTrackDcw); }
    juce::AudioParameterFloat*  getKeyTrackPitch() const      { return getParam<juce::AudioParameterFloat>(ParameterIDs::keyTrackPitch); }
    juce::AudioParameterChoice* getKeyFollowDco() const       { return getParam<juce::AudioParameterChoice>(ParameterIDs::keyFollowDco); }
    juce::AudioParameterChoice* getKeyFollowDcw() const       { return getParam<juce::AudioParameterChoice>(ParameterIDs::keyFollowDcw); }
    juce::AudioParameterChoice* getKeyFollowDca() const       { return getParam<juce::AudioParameterChoice>(ParameterIDs::keyFollowDca); }
    
    // Phase 5.1: Oversampling Quality
    juce::AudioParameterChoice* getOversamplingQuality() const { return getParam<juce::AudioParameterChoice>(ParameterIDs::oversampling); }

    // Arpeggiator getters
    juce::AudioParameterBool*   getArpEnabled() const         { return getParam<juce::AudioParameterBool>(ParameterIDs::arpEnabled); }
    juce::AudioParameterBool*   getArpLatch() const           { return getParam<juce::AudioParameterBool>(ParameterIDs::arpLatch); }
    juce::AudioParameterChoice* getArpRate() const            { return getParam<juce::AudioParameterChoice>(ParameterIDs::arpRate); }
    juce::AudioParameterFloat*  getArpBpm() const             { return getParam<juce::AudioParameterFloat>(ParameterIDs::arpBpm); }
    juce::AudioParameterFloat*  getArpGate() const            { return getParam<juce::AudioParameterFloat>(ParameterIDs::arpGate); }
    juce::AudioParameterFloat*  getArpSwing() const           { return getParam<juce::AudioParameterFloat>(ParameterIDs::arpSwing); }
    juce::AudioParameterChoice* getArpSwingMode() const       { return getParam<juce::AudioParameterChoice>(ParameterIDs::arpSwingMode); }
    juce::AudioParameterChoice* getArpPattern() const         { return getParam<juce::AudioParameterChoice>(ParameterIDs::arpPattern); }
    juce::AudioParameterInt*    getArpOctave() const          { return getParam<juce::AudioParameterInt>(ParameterIDs::arpOctave); }

    // Macro Controls
    juce::AudioParameterFloat*  getMacroBrilliance() const    { return getParam<juce::AudioParameterFloat>(ParameterIDs::macroBrilliance); }
    juce::AudioParameterFloat*  getMacroTone() const          { return getParam<juce::AudioParameterFloat>(ParameterIDs::macroTone); }
    juce::AudioParameterFloat*  getMacroSpace() const         { return getParam<juce::AudioParameterFloat>(ParameterIDs::macroSpace); }

    juce::RangedAudioParameter* getParameter(const juce::String& paramId) const;
    const std::map<juce::String, juce::RangedAudioParameter*>& getParameterMap() const { return parameterMap; }
    
private:
    template <typename T>
    T* getParam(const juce::String& id) const {
        return dynamic_cast<T*>(apvts->getParameter(id));
    }

    juce::AudioProcessor& audioProcessor;
    std::unique_ptr<juce::AudioProcessorValueTreeState> apvts;
    std::map<juce::String, juce::RangedAudioParameter*> parameterMap;
};

} // namespace State
} // namespace CZ101
