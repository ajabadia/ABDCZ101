#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "DriveEffect.h"
#include "Chorus.h"
#include "StereoDelay.h"
#include "Reverb.h"

// Forward Declaration to avoid include issues
namespace CZ101 { namespace Core { struct ParameterSnapshot; } }


namespace CZ101 {
namespace DSP {
namespace Effects {

class EffectsChain
{
public:
    EffectsChain();
    ~EffectsChain() = default;

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();
    
    /**
     * Processes the audio buffer applying effects based on the snapshot configuration.
     * @param buffer Stereo Audio Buffer
     * @param snapshot Current parameter snapshot (must be valid)
     */
    void process(juce::AudioBuffer<float>& buffer, const CZ101::Core::ParameterSnapshot& snapshot);

private:
    // Modern Filters
    juce::dsp::LadderFilter<float> modernLpf;
    juce::dsp::StateVariableTPTFilter<float> modernHpf;

    // Effects
    DriveEffect driveEffect;
    Chorus chorus;
    StereoDelay stereoDelay;
    juce::Reverb reverb;
    juce::Reverb::Parameters reverbParams;

    // Smoothed Parameters (16ms interpolation)
    juce::LinearSmoothedValue<float> smoothLpfCutoff { 20000.0f };
    juce::LinearSmoothedValue<float> smoothLpfReso { 0.0f };
    juce::LinearSmoothedValue<float> smoothHpfCutoff { 20.0f };
    juce::LinearSmoothedValue<float> smoothDriveAmount { 0.0f };
    juce::LinearSmoothedValue<float> smoothDriveColor { 0.5f };
    juce::LinearSmoothedValue<float> smoothDriveMix { 0.0f };
    juce::LinearSmoothedValue<float> smoothChorusRate { 0.0f };
    juce::LinearSmoothedValue<float> smoothChorusDepth { 0.0f };
    juce::LinearSmoothedValue<float> smoothChorusMix { 0.0f };
    juce::LinearSmoothedValue<float> smoothDelayTime { 0.25f };
    juce::LinearSmoothedValue<float> smoothDelayFb { 0.0f };
    juce::LinearSmoothedValue<float> smoothDelayMix { 0.0f };
    juce::LinearSmoothedValue<float> smoothReverbSize { 0.5f };
    juce::LinearSmoothedValue<float> smoothReverbMix { 0.0f };

    double sampleRate = 44100.0;
};

} // namespace Effects
} // namespace DSP
} // namespace CZ101
