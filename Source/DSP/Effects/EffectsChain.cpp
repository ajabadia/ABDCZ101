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
    
    // Initialize smoothing
    smoothLpfCutoff.reset(sampleRate, 0.016);
    smoothLpfReso.reset(sampleRate, 0.016);
    smoothHpfCutoff.reset(sampleRate, 0.016);
    smoothDriveAmount.reset(sampleRate, 0.016);
    smoothDriveColor.reset(sampleRate, 0.016);
    smoothDriveMix.reset(sampleRate, 0.016);
    smoothChorusRate.reset(sampleRate, 0.016);
    smoothChorusDepth.reset(sampleRate, 0.016);
    smoothChorusMix.reset(sampleRate, 0.016);
    smoothDelayTime.reset(sampleRate, 0.016);
    smoothDelayFb.reset(sampleRate, 0.016);
    smoothDelayMix.reset(sampleRate, 0.016);
    smoothReverbSize.reset(sampleRate, 0.016);
    smoothReverbMix.reset(sampleRate, 0.016);
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
    bool isModern = (opMode == 3);
    bool isClassic5000 = (opMode == 1 || opMode == 2);

    // Set targets for smoothing
    smoothLpfCutoff.setTargetValue(eff.lpfCutoff);
    smoothLpfReso.setTargetValue(eff.lpfReso);
    smoothHpfCutoff.setTargetValue(eff.hpfCutoff);
    smoothDriveAmount.setTargetValue(eff.driveAmount);
    smoothDriveColor.setTargetValue(eff.driveColor);
    smoothDriveMix.setTargetValue(eff.driveMix);
    
    const float rawChorusMix = (opMode == 0) ? 0.0f : eff.chorusMix;
    smoothChorusRate.setTargetValue(eff.chorusRate);
    smoothChorusDepth.setTargetValue(eff.chorusDepth);
    smoothChorusMix.setTargetValue(rawChorusMix);
    
    smoothDelayTime.setTargetValue(eff.delayTime);
    smoothDelayFb.setTargetValue(eff.delayFb);
    smoothDelayMix.setTargetValue(eff.delayMix);
    
    smoothReverbSize.setTargetValue(eff.reverbSize);
    smoothReverbMix.setTargetValue(eff.reverbMix);

    int numSamples = buffer.getNumSamples();

    // 1. Modern Pre-Processing (Filters & Drive)
    if (isModern)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);

        // Update & Apply LPF
        modernLpf.setCutoffFrequencyHz(smoothLpfCutoff.getNextValue());
        modernLpf.setResonance(smoothLpfReso.getNextValue());
        modernLpf.process(context);

        // Update & Apply HPF
        modernHpf.setCutoffFrequency(smoothHpfCutoff.getNextValue());
        modernHpf.process(context);

        // Update & Apply Drive
        driveEffect.setAmount(smoothDriveAmount.getNextValue());
        driveEffect.setColor(smoothDriveColor.getNextValue());
        driveEffect.setMix(smoothDriveMix.getNextValue());
        driveEffect.process(context);
        
        // Skip smoothing steps consumed by block processing
        smoothLpfCutoff.skip(numSamples - 1);
        smoothLpfReso.skip(numSamples - 1);
        smoothHpfCutoff.skip(numSamples - 1);
        smoothDriveAmount.skip(numSamples - 1);
        smoothDriveColor.skip(numSamples - 1);
        smoothDriveMix.skip(numSamples - 1);
    }
    else {
        // Skip smoothing if modules are inactive
        smoothLpfCutoff.skip(numSamples);
        smoothLpfReso.skip(numSamples);
        smoothHpfCutoff.skip(numSamples);
        smoothDriveAmount.skip(numSamples);
        smoothDriveColor.skip(numSamples);
        smoothDriveMix.skip(numSamples);
    }

    // 2. Hardware/Legacy Effects
    chorus.setModernMode(isModern);
    chorus.setRate(smoothChorusRate.getNextValue());
    chorus.setDepth(smoothChorusDepth.getNextValue());
    chorus.setMix(smoothChorusMix.getNextValue());
    chorus.process(buffer.getWritePointer(0), buffer.getWritePointer(1), numSamples);
    
    smoothChorusRate.skip(numSamples - 1);
    smoothChorusDepth.skip(numSamples - 1);
    smoothChorusMix.skip(numSamples - 1);

    // 3. Modern Post-Processing (Delay & Reverb)
    if (isModern)
    {
        stereoDelay.setParameters(smoothDelayTime.getNextValue(), smoothDelayFb.getNextValue(), smoothDelayMix.getNextValue(), false, 0.0f); 
        stereoDelay.process(buffer.getWritePointer(0), buffer.getWritePointer(1), numSamples);
        
        reverbParams.roomSize = smoothReverbSize.getNextValue();
        reverbParams.wetLevel = smoothReverbMix.getNextValue();
        reverbParams.dryLevel = 1.0f - (reverbParams.wetLevel * 0.5f);
        reverbParams.damping = 0.5f;
        reverbParams.width = 1.0f;
        reverb.setParameters(reverbParams);
        reverb.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), numSamples);
        
        smoothDelayTime.skip(numSamples - 1);
        smoothDelayFb.skip(numSamples - 1);
        smoothDelayMix.skip(numSamples - 1);
        smoothReverbSize.skip(numSamples - 1);
        smoothReverbMix.skip(numSamples - 1);
    }
    else {
        smoothDelayTime.skip(numSamples);
        smoothDelayFb.skip(numSamples);
        smoothDelayMix.skip(numSamples);
        smoothReverbSize.skip(numSamples);
        smoothReverbMix.skip(numSamples);
    }
}

} // namespace Effects
} // namespace DSP
} // namespace CZ101
