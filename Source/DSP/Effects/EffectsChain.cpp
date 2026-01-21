#include "EffectsChain.h"
#include "../../Core/AudioThreadSnapshot.h"

namespace CZ101 {
namespace DSP {
namespace Effects {

EffectsChain::EffectsChain()
{
}

void EffectsChain::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    // Filters
    modernLpf.prepare(spec);
    modernLpf.setMode(juce::dsp::LadderFilterMode::LPF24);
    modernLpf.reset();

    modernHpf.prepare(spec);
    modernHpf.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    modernHpf.reset();

    // Effects
    driveEffect.prepare(spec);
    chorus.prepare(sampleRate);
    stereoDelay.prepare(sampleRate);
    
    reverb.reset();
    reverb.setSampleRate(sampleRate);
}

void EffectsChain::reset()
{
    modernLpf.reset();
    modernHpf.reset();
    driveEffect.reset();
    chorus.reset();
    stereoDelay.reset();
    reverb.reset();
}

void EffectsChain::process(juce::AudioBuffer<float>& buffer, const CZ101::Core::ParameterSnapshot& snapshot)
{
    const auto& eff = snapshot.effects;
    int opMode = snapshot.system.opMode;
    bool isModern = (opMode == 2);
    bool isClassic5000 = (opMode == 1);

    // 1. Modern Pre-Processing (Filters & Drive)
    if (isModern)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);

        // Update & Apply LPF
        modernLpf.setCutoffFrequencyHz(eff.lpfCutoff);
        modernLpf.setResonance(eff.lpfReso);
        modernLpf.process(context);

        // Update & Apply HPF
        modernHpf.setCutoffFrequency(eff.hpfCutoff);
        modernHpf.process(context);

        // Update & Apply Drive
        driveEffect.setAmount(eff.driveAmount);
        driveEffect.setColor(eff.driveColor);
        driveEffect.setMix(eff.driveMix);
        driveEffect.process(context);
    }

    // 2. Hardware/Legacy Effects
    
    // Chorus (Available in Modern and Classic 5000)
    // Note: In Classic 101, mix should naturally be 0 unless forced, but we logic it via opMode to be strict if needed.
    // However, if the UI hides it, params in snapshot should be default. 
    // But let's enforce availability based on mode to match previous logic.
    // The previous logic ran chorus unconditionally, relying on Mix param.
    // We'll proceed with applying parameters.
    
    chorus.setModernMode(isModern);
    chorus.setRate(eff.chorusRate);
    chorus.setDepth(eff.chorusDepth); 
    chorus.setMix(eff.chorusMix);
    
    // Process Chorus
    chorus.process(buffer.getWritePointer(0), buffer.getWritePointer(1), buffer.getNumSamples());

    // 3. Modern Post-Processing (Delay & Reverb)
    if (isModern)
    {
        // Stereo Delay
        // Params are updated every block here - slightly inefficient but robust
        stereoDelay.setParameters(eff.delayTime, eff.delayFb, eff.delayMix, false, 0.0f); 
        stereoDelay.process(buffer.getWritePointer(0), buffer.getWritePointer(1), buffer.getNumSamples());
        
        // Reverb
        reverbParams.roomSize = eff.reverbSize;
        reverbParams.wetLevel = eff.reverbMix;
        reverbParams.dryLevel = 1.0f - (eff.reverbMix * 0.5f);
        reverbParams.damping = 0.5f;
        reverbParams.width = 1.0f;
        reverb.setParameters(reverbParams);
        reverb.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), buffer.getNumSamples());
    }
}

} // namespace Effects
} // namespace DSP
} // namespace CZ101
