#include "Parameters.h"
#include "ParameterIDs.h"

namespace CZ101 {
namespace State {

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto waveChoices = juce::StringArray{"1: Saw", "2: Square", "3: Pulse", "4: Null", "5: Dbl Sine", "6: SawPulse", "7: MultiSine", "8: Pulse 2"};
    auto waveChoices2 = juce::StringArray{"0: None", "1: Saw", "2: Square", "3: Pulse", "4: Null", "5: Dbl Sine", "6: SawPulse", "7: MultiSine", "8: Pulse 2"};
    auto windowChoices = juce::StringArray{"0: None", "1: Saw", "2: Triangle", "3: Trapezoid", "4: Pulse", "5: Dbl Saw"};
    auto lfoWaveChoices = juce::StringArray{"Triangle", "Saw Up", "Saw Down", "Square"};
    auto lineSelChoices = juce::StringArray{"Line 1", "Line 2", "Line 1+1'", "Line 1+2"};
    auto keyFollowChoices = juce::StringArray{"OFF", "FIX", "VAR"};

    // 1. Oscillators
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("oscillators", "Oscillators", "|");
        group->addChild(std::make_unique<juce::AudioParameterChoice>(ParameterIDs::lineSelect, "Line Select", lineSelChoices, 2));
        group->addChild(std::make_unique<juce::AudioParameterChoice>(ParameterIDs::osc1Waveform, "Osc 1 Waveform", waveChoices, 0));
        group->addChild(std::make_unique<juce::AudioParameterChoice>(ParameterIDs::osc1Window, "Osc 1 Window", windowChoices, 0));
        group->addChild(std::make_unique<juce::AudioParameterChoice>(ParameterIDs::osc1Waveform2, "Osc 1 Second Wave", waveChoices2, 0));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::osc1Level, "Osc 1 Level", 0.0f, 1.0f, 1.0f));
        group->addChild(std::make_unique<juce::AudioParameterChoice>(ParameterIDs::osc2Waveform, "Osc 2 Waveform", waveChoices, 0));
        group->addChild(std::make_unique<juce::AudioParameterChoice>(ParameterIDs::osc2Window, "Osc 2 Window", windowChoices, 0));
        group->addChild(std::make_unique<juce::AudioParameterChoice>(ParameterIDs::osc2Waveform2, "Osc 2 Second Wave", waveChoices2, 0));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::osc2Level, "Osc 2 Level", 0.0f, 1.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::osc2Detune, "Osc 2 Detune (Legacy)", -12.0f, 12.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterInt>(ParameterIDs::octave, "Octave", -1, 1, 0));
        group->addChild(std::make_unique<juce::AudioParameterInt>(ParameterIDs::detuneOct, "Detune Octave", -3, 3, 0));
        group->addChild(std::make_unique<juce::AudioParameterInt>(ParameterIDs::detuneCoarse, "Detune Coarse", -12, 12, 0));
        group->addChild(std::make_unique<juce::AudioParameterInt>(ParameterIDs::detuneFine, "Detune Fine", -50, 50, 0));
        // Audit Fix [2.4]: Tone Mix (0.0=Line1, 0.5=Both, 1.0=Line2)
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::lineMix, "Line Mix", 0.0f, 1.0f, 0.5f));
        group->addChild(std::make_unique<juce::AudioParameterBool>(ParameterIDs::hardSync, "Hard Sync", false));
        group->addChild(std::make_unique<juce::AudioParameterChoice>(ParameterIDs::lineMod, "Line Modulation", 
            juce::StringArray{ "Off", "Ring 1", "Noise 1", "Ring 2", "Ring 3", "Noise 2" }, 0));
        group->addChild(std::make_unique<juce::AudioParameterBool>(ParameterIDs::modSpecial, "Mod Special (Line 1 Mute)", false));        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::glideTime, "Portamento Time", 0.0f, 1.0f, 0.0f));
        layout.add(std::move(group));
    }

    // 2. LFO
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("vibrato", "LFO / Vibrato", "|");
        group->addChild(std::make_unique<juce::AudioParameterChoice>(ParameterIDs::lfoWaveform, "LFO Wave", lfoWaveChoices, 0));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::lfoRate, "LFO Rate", 0.1f, 30.0f, 5.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::lfoDepth, "LFO Depth", 0.0f, 1.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::lfoDelay, "LFO Delay", 0.0f, 2.0f, 0.0f));
        layout.add(std::move(group));
    }

    // 3. Envelopes (Simplified ADSR)
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("envelopes", "ADSR Envelopes", "|");
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::dcaAttack, "DCA Attack", 0.0f, 10.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::dcaDecay, "DCA Decay", 0.0f, 10.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::dcaSustain, "DCA Sustain", 0.0f, 1.0f, 1.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::dcaRelease, "DCA Release", 0.0f, 10.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::dcwAttack, "DCW Attack", 0.0f, 10.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::dcwDecay, "DCW Decay", 0.0f, 10.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::dcwSustain, "DCW Sustain", 0.0f, 1.0f, 1.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::dcwRelease, "DCW Release", 0.0f, 10.0f, 0.0f));
        layout.add(std::move(group));
    }

    // 4. Modulation Matrix
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("modulation", "Modulation Matrix", "|");
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::modVeloDcw, "Velo -> DCW", 0.0f, 1.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::modVeloDca, "Velo -> DCA", 0.0f, 1.0f, 1.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::modWheelDcw, "Wheel -> DCW", 0.0f, 1.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::modWheelLfoRate, "Wheel -> LFO Rate", 0.0f, 1.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::modWheelVib, "Wheel -> Vibrato", 0.0f, 1.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::modAtDcw, "AT -> DCW", 0.0f, 1.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::modAtVib, "AT -> Vibrato", 0.0f, 1.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::keyTrackDcw, "Key Track DCW", 0.0f, 1.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::keyTrackPitch, "Key Track Pitch", 0.0f, 1.0f, 1.0f));
        group->addChild(std::make_unique<juce::AudioParameterChoice>(ParameterIDs::keyFollowDco, "Key Follow DCO", keyFollowChoices, 2));
        group->addChild(std::make_unique<juce::AudioParameterChoice>(ParameterIDs::keyFollowDcw, "Key Follow DCW", keyFollowChoices, 0));
        group->addChild(std::make_unique<juce::AudioParameterChoice>(ParameterIDs::keyFollowDca, "Key Follow DCA", keyFollowChoices, 0));
        
        // CZ-1 Velocity Sensitivities (Per-Line)
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::line1VeloPitch, "Line 1 Velo -> Pitch", 0.0f, 15.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::line1VeloDcw, "Line 1 Velo -> DCW", 0.0f, 15.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::line1VeloDca, "Line 1 Velo -> DCA", 0.0f, 15.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::line2VeloPitch, "Line 2 Velo -> Pitch", 0.0f, 15.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::line2VeloDcw, "Line 2 Velo -> DCW", 0.0f, 15.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::line2VeloDca, "Line 2 Velo -> DCA", 0.0f, 15.0f, 0.0f));

        // CZ-1 Key Follow per-line (0-9)
        group->addChild(std::make_unique<juce::AudioParameterInt>(ParameterIDs::line1KfPitch, "Line 1 KF Pitch", 0, 9, 0));
        group->addChild(std::make_unique<juce::AudioParameterInt>(ParameterIDs::line1KfDcw, "Line 1 KF DCW", 0, 9, 0));
        group->addChild(std::make_unique<juce::AudioParameterInt>(ParameterIDs::line1KfDca, "Line 1 KF DCA", 0, 9, 0));
        group->addChild(std::make_unique<juce::AudioParameterInt>(ParameterIDs::line2KfPitch, "Line 2 KF Pitch", 0, 9, 0));
        group->addChild(std::make_unique<juce::AudioParameterInt>(ParameterIDs::line2KfDcw, "Line 2 KF DCW", 0, 9, 0));
        group->addChild(std::make_unique<juce::AudioParameterInt>(ParameterIDs::line2KfDca, "Line 2 KF DCA", 0, 9, 0));

        layout.add(std::move(group));
    }

    // 5. Modern Filter
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("filter", "Modern Filter", "|");
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::lpfCutoff, "Modern LPF Cutoff", 
            juce::NormalisableRange<float>(20.0f, 20000.0f, 0.0f, 0.3f), 20000.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::lpfReso, "Modern LPF Resonance", 0.0f, 1.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::hpfCutoff, "Modern HPF Cutoff", 
            juce::NormalisableRange<float>(20.0f, 10000.0f, 0.0f, 0.3f), 20.0f));
        layout.add(std::move(group));
    }

    // 6. Effects
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("effects", "Effects", "|");
        // Drive (Modern Only)
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::driveAmount, "Drive Amount", 0.0f, 1.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::driveColor, "Drive Color", 0.0f, 1.0f, 0.5f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::driveMix, "Drive Mix", 0.0f, 1.0f, 0.0f));

        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::chorusRate, "Chorus Rate", 0.1f, 10.0f, 0.5f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::chorusDepth, "Chorus Depth", 0.0f, 1.0f, 0.2f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::chorusMix, "Chorus Mix", 0.0f, 1.0f, 0.3f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::delayTime, "Delay Time", 0.0f, 2.0f, 0.5f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::delayFeedback, "Delay Feedback", 0.0f, 0.95f, 0.3f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::delayMix, "Delay Mix", 0.0f, 1.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::reverbSize, "Reverb Size", 0.0f, 1.0f, 0.5f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::reverbMix, "Reverb Mix", 0.0f, 1.0f, 0.2f));
        layout.add(std::move(group));
    }

    // 7. System
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("system", "System", "|");
        group->addChild(std::make_unique<juce::AudioParameterBool>(ParameterIDs::protectSwitch, "Memory Protect", true));
        group->addChild(std::make_unique<juce::AudioParameterBool>(ParameterIDs::systemPrg, "SysEx Data Interchange", false));
        group->addChild(std::make_unique<juce::AudioParameterBool>(ParameterIDs::bypass, "Bypass", false));
        // Audit Fix [2.2a]: Unified Operation Mode
        // 0: Classic 101, 1: Classic 5000, 2: Classic CZ-1, 3: Modern
        group->addChild(std::make_unique<juce::AudioParameterChoice>(ParameterIDs::operationMode, "Operation Mode", juce::StringArray{"Classic 101", "Classic 5000", "Classic CZ-1", "Modern"}, 0));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::masterVolume, "Master Volume", 0.0f, 1.0f, 1.0f));
        
        group->addChild(std::make_unique<juce::AudioParameterInt>(ParameterIDs::midiChannel, "MIDI Channel", 1, 16, 1));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::masterTune, "Master Tune", -50.0f, 50.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterInt>(ParameterIDs::benderRange, "Pitch Bend Range", 0, 12, 2));
        group->addChild(std::make_unique<juce::AudioParameterInt>(ParameterIDs::transpose, "Key Transpose", -12, 12, 0));
        
        // Phase 5.1: Oversampling Quality
        group->addChild(std::make_unique<juce::AudioParameterChoice>(ParameterIDs::oversampling, "Oversampling", juce::StringArray{"1x (Eco)", "2x (High)", "4x (Ultra)"}, 0));
        group->addChild(std::make_unique<juce::AudioParameterBool>(ParameterIDs::hardwareNoise, "Hardware Noise", true));
        
        layout.add(std::move(group));
    }

    // 8. Arpeggiator
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("arpeggiator", "Arpeggiator", "|");
        group->addChild(std::make_unique<juce::AudioParameterBool>(ParameterIDs::arpEnabled, "Arpeggiator On/Off", false));
        group->addChild(std::make_unique<juce::AudioParameterBool>(ParameterIDs::arpLatch, "Latch Mode", false));
        group->addChild(std::make_unique<juce::AudioParameterChoice>(ParameterIDs::arpRate, "Rate", juce::StringArray{"1/4", "1/8", "1/16", "1/32"}, 2)); 
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::arpBpm, "Internal Tempo", 40.0f, 240.0f, 120.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::arpGate, "Gate Time", 0.0f, 1.0f, 0.5f)); // 0 = Staccato, 1 = Legato
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::arpSwing, "Swing Amount", 0.0f, 1.0f, 0.0f)); 
        group->addChild(std::make_unique<juce::AudioParameterChoice>(ParameterIDs::arpSwingMode, "Swing Mode", juce::StringArray{"Off", "1/8", "1/16"}, 2));
        group->addChild(std::make_unique<juce::AudioParameterChoice>(ParameterIDs::arpPattern, "Pattern", juce::StringArray{"Up", "Down", "Up/Down", "Random", "As Played"}, 0));
        group->addChild(std::make_unique<juce::AudioParameterInt>(ParameterIDs::arpOctave, "Octave Range", 1, 4, 1));
        layout.add(std::move(group));
    }

    // 9. Performance Macros
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("macros", "Performance Macros", "|");
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::macroBrilliance, "Brilliance", 0.0f, 1.0f, 0.5f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::macroTone, "Tone", 0.0f, 1.0f, 0.5f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::macroSpace, "Space", 0.0f, 1.0f, 0.0f));
        layout.add(std::move(group));
    }

    return layout;
}

Parameters::Parameters(juce::AudioProcessor& processor, juce::UndoManager* undoManager)
    : audioProcessor(processor)
{
    apvts = std::make_unique<juce::AudioProcessorValueTreeState>(processor, undoManager, "PARAMETERS", createParameterLayout());

    // Populate the helper map for easier iteration (e.g. for randomization)
    parameterMap.clear();
    for (auto* p : audioProcessor.getParameters()) {
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p)) {
            parameterMap[rp->getParameterID()] = rp;
        }
    }
}

void Parameters::createParameters() {}

juce::RangedAudioParameter* Parameters::getParameter(const juce::String& paramId) const
{
    return apvts->getParameter(paramId);
}

} // namespace State
} // namespace CZ101
