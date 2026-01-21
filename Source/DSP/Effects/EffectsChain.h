#pragma once

#include <JuceHeader.h>
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

    double sampleRate = 44100.0;
};

} // namespace Effects
} // namespace DSP
} // namespace CZ101
