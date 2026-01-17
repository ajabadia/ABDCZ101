#include "Parameters.h"

namespace CZ101 {
namespace State {

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto waveChoices = juce::StringArray{"1: Saw", "2: Square", "3: Pulse", "4: Dbl Sine", "5: SawPulse", "6: Reso 1", "7: Reso 2", "8: Reso 3"};
    auto waveChoices2 = juce::StringArray{"0: None", "1: Saw", "2: Square", "3: Pulse", "4: Dbl Sine", "5: SawPulse", "6: Reso 1", "7: Reso 2", "8: Reso 3"};
    auto lfoWaveChoices = juce::StringArray{"Triangle", "Saw Up", "Saw Down", "Square"};
    auto lineSelChoices = juce::StringArray{"Line 1", "Line 2", "Line 1+1'", "Line 1+2"};
    auto keyFollowChoices = juce::StringArray{"OFF", "FIX", "VAR"};

    // 1. Oscillators
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("oscillators", "Oscillators", "|");
        group->addChild(std::make_unique<juce::AudioParameterChoice>("LINE_SELECT", "Line Select", lineSelChoices, 2));
        group->addChild(std::make_unique<juce::AudioParameterChoice>("OSC1_WAVEFORM", "Osc 1 Waveform", waveChoices, 0));
        group->addChild(std::make_unique<juce::AudioParameterChoice>("OSC1_WAVEFORM2", "Osc 1 Second Wave", waveChoices2, 0));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("OSC1_LEVEL", "Osc 1 Level", 0.0f, 1.0f, 1.0f));
        group->addChild(std::make_unique<juce::AudioParameterChoice>("OSC2_WAVEFORM", "Osc 2 Waveform", waveChoices, 0));
        group->addChild(std::make_unique<juce::AudioParameterChoice>("OSC2_WAVEFORM2", "Osc 2 Second Wave", waveChoices2, 0));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("OSC2_LEVEL", "Osc 2 Level", 0.0f, 1.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("OSC2_DETUNE", "Osc 2 Detune (Legacy)", -12.0f, 12.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterInt>("DETUNE_OCT", "Detune Octave", -3, 3, 0));
        group->addChild(std::make_unique<juce::AudioParameterInt>("DETUNE_COARSE", "Detune Coarse", -12, 12, 0));
        group->addChild(std::make_unique<juce::AudioParameterInt>("DETUNE_FINE", "Detune Fine", -50, 50, 0));
        // Audit Fix [2.4]: Tone Mix (0.0=Line1, 0.5=Both, 1.0=Line2)
        group->addChild(std::make_unique<juce::AudioParameterFloat>("LINE_MIX", "Line Mix", 0.0f, 1.0f, 0.5f));
        group->addChild(std::make_unique<juce::AudioParameterBool>("HARD_SYNC", "Hard Sync", false));
        group->addChild(std::make_unique<juce::AudioParameterBool>("RING_MOD", "Ring Mod", false));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("GLIDE", "Portamento Time", 0.0f, 1.0f, 0.0f));
        layout.add(std::move(group));
    }

    // 2. LFO
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("vibrato", "LFO / Vibrato", "|");
        group->addChild(std::make_unique<juce::AudioParameterChoice>("LFO_WAVE", "LFO Wave", lfoWaveChoices, 0));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("LFO_RATE", "LFO Rate", 0.1f, 30.0f, 5.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("LFO_DEPTH", "LFO Depth", 0.0f, 1.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("LFO_DELAY", "LFO Delay", 0.0f, 2.0f, 0.0f));
        layout.add(std::move(group));
    }

    // 3. Envelopes (Simplified ADSR)
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("envelopes", "ADSR Envelopes", "|");
        group->addChild(std::make_unique<juce::AudioParameterFloat>("DCA_ATTACK", "DCA Attack", 0.0f, 10.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("DCA_DECAY", "DCA Decay", 0.0f, 10.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("DCA_SUSTAIN", "DCA Sustain", 0.0f, 1.0f, 1.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("DCA_RELEASE", "DCA Release", 0.0f, 10.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("DCW_ATTACK", "DCW Attack", 0.0f, 10.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("DCW_DECAY", "DCW Decay", 0.0f, 10.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("DCW_SUSTAIN", "DCW Sustain", 0.0f, 1.0f, 1.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("DCW_RELEASE", "DCW Release", 0.0f, 10.0f, 0.0f));
        layout.add(std::move(group));
    }

    // 4. Modulation Matrix
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("modulation", "Modulation Matrix", "|");
        group->addChild(std::make_unique<juce::AudioParameterFloat>("MOD_VELO_DCW", "Velo -> DCW", 0.0f, 1.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("MOD_VELO_DCA", "Velo -> DCA", 0.0f, 1.0f, 1.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("MOD_WHEEL_DCW", "Wheel -> DCW", 0.0f, 1.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("MOD_WHEEL_LFORATE", "Wheel -> LFO Rate", 0.0f, 1.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("MOD_WHEEL_VIB", "Wheel -> Vibrato", 0.0f, 1.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("MOD_AT_DCW", "AT -> DCW", 0.0f, 1.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("MOD_AT_VIB", "AT -> Vibrato", 0.0f, 1.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("KEY_TRACK_DCW", "Key Track DCW", 0.0f, 1.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("KEY_TRACK_PITCH", "Key Track Pitch", 0.0f, 1.0f, 1.0f));
        group->addChild(std::make_unique<juce::AudioParameterChoice>("KEY_FOLLOW_DCO", "Key Follow DCO", keyFollowChoices, 2));
        group->addChild(std::make_unique<juce::AudioParameterChoice>("KEY_FOLLOW_DCW", "Key Follow DCW", keyFollowChoices, 0));
        group->addChild(std::make_unique<juce::AudioParameterChoice>("KEY_FOLLOW_DCA", "Key Follow DCA", keyFollowChoices, 0));
        layout.add(std::move(group));
    }

    // 5. Modern Filter
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("filter", "Modern Filter", "|");
        group->addChild(std::make_unique<juce::AudioParameterFloat>("MODERN_LPF_CUTOFF", "Modern LPF Cutoff", 
            juce::NormalisableRange<float>(20.0f, 20000.0f, 0.0f, 0.3f), 20000.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("MODERN_LPF_RESO", "Modern LPF Resonance", 0.0f, 1.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("MODERN_HPF_CUTOFF", "Modern HPF Cutoff", 
            juce::NormalisableRange<float>(20.0f, 10000.0f, 0.0f, 0.3f), 20.0f));
        layout.add(std::move(group));
    }

    // 6. Effects
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("effects", "Effects", "|");
        group->addChild(std::make_unique<juce::AudioParameterFloat>("CHORUS_RATE", "Chorus Rate", 0.1f, 10.0f, 0.5f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("CHORUS_DEPTH", "Chorus Depth", 0.0f, 1.0f, 0.2f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("CHORUS_MIX", "Chorus Mix", 0.0f, 1.0f, 0.3f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("DELAY_TIME", "Delay Time", 0.0f, 2.0f, 0.5f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("DELAY_FEEDBACK", "Delay Feedback", 0.0f, 0.95f, 0.3f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("DELAY_MIX", "Delay Mix", 0.0f, 1.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("REVERB_SIZE", "Reverb Size", 0.0f, 1.0f, 0.5f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("REVERB_MIX", "Reverb Mix", 0.0f, 1.0f, 0.2f));
        layout.add(std::move(group));
    }

    // 7. System
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("system", "System", "|");
        group->addChild(std::make_unique<juce::AudioParameterBool>("PROTECT_SWITCH", "Memory Protect", true));
        group->addChild(std::make_unique<juce::AudioParameterBool>("SYSTEM_PRG", "SysEx Data Interchange", false));
        group->addChild(std::make_unique<juce::AudioParameterBool>("BYPASS", "Bypass", false));
        group->addChild(std::make_unique<juce::AudioParameterBool>("BYPASS", "Bypass", false));
        // Audit Fix [2.2a]: Unified Operation Mode
        // 0: Classic 101, 1: Classic 5000, 2: Modern
        group->addChild(std::make_unique<juce::AudioParameterChoice>("OPERATION_MODE", "Operation Mode", juce::StringArray{"Classic 101", "Classic 5000", "Modern"}, 0));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("MASTER_VOLUME", "Master Volume", 0.0f, 1.0f, 1.0f));
        
        group->addChild(std::make_unique<juce::AudioParameterInt>("MIDI_CH", "MIDI Channel", 1, 16, 1));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("MASTER_TUNE", "Master Tune", -50.0f, 50.0f, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterInt>("PITCH_BEND_RANGE", "Pitch Bend Range", 0, 12, 2));
        group->addChild(std::make_unique<juce::AudioParameterInt>("KEY_TRANSPOSE", "Key Transpose", -12, 12, 0));
        
        layout.add(std::move(group));
    }

    // 8. Arpeggiator
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("arpeggiator", "Arpeggiator", "|");
        group->addChild(std::make_unique<juce::AudioParameterBool>("ARP_ENABLED", "Arpeggiator On/Off", false));
        group->addChild(std::make_unique<juce::AudioParameterBool>("ARP_LATCH", "Latch Mode", false));
        group->addChild(std::make_unique<juce::AudioParameterChoice>("ARP_RATE", "Rate", juce::StringArray{"1/4", "1/8", "1/16", "1/32"}, 2)); 
        group->addChild(std::make_unique<juce::AudioParameterFloat>("ARP_BPM", "Internal Tempo", 40.0f, 240.0f, 120.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("ARP_GATE", "Gate Time", 0.0f, 1.0f, 0.5f)); // 0 = Staccato, 1 = Legato
        group->addChild(std::make_unique<juce::AudioParameterFloat>("ARP_SWING", "Swing", 0.0f, 1.0f, 0.0f)); // 0 = Straight, 1.0 = Heavy Swing
        group->addChild(std::make_unique<juce::AudioParameterChoice>("ARP_PATTERN", "Pattern", juce::StringArray{"Up", "Down", "Up/Down", "Random", "As Played"}, 0));
        group->addChild(std::make_unique<juce::AudioParameterInt>("ARP_OCTAVE", "Octave Range", 1, 4, 1));
        layout.add(std::move(group));
    }

    // 9. Performance Macros
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("macros", "Performance Macros", "|");
        group->addChild(std::make_unique<juce::AudioParameterFloat>("MACRO_BRILLIANCE", "Brilliance", 0.0f, 1.0f, 0.5f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("MACRO_TONE", "Tone", 0.0f, 1.0f, 0.5f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("MACRO_SPACE", "Space", 0.0f, 1.0f, 0.0f));
        layout.add(std::move(group));
    }

    return layout;
}

Parameters::Parameters(juce::AudioProcessor& processor, juce::UndoManager* undoManager)
    : audioProcessor(processor)
{
    apvts = std::make_unique<juce::AudioProcessorValueTreeState>(processor, undoManager, "PARAMETERS", createParameterLayout());

    // Asignar punteros para acceso rápido
    lineSelect   = dynamic_cast<juce::AudioParameterChoice*>(apvts->getParameter("LINE_SELECT"));
    osc1Waveform = dynamic_cast<juce::AudioParameterChoice*>(apvts->getParameter("OSC1_WAVEFORM"));
    osc1Waveform2= dynamic_cast<juce::AudioParameterChoice*>(apvts->getParameter("OSC1_WAVEFORM2"));
    osc1Level    = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("OSC1_LEVEL"));
    osc2Waveform = dynamic_cast<juce::AudioParameterChoice*>(apvts->getParameter("OSC2_WAVEFORM"));
    osc2Waveform2= dynamic_cast<juce::AudioParameterChoice*>(apvts->getParameter("OSC2_WAVEFORM2"));
    osc2Level    = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("OSC2_LEVEL"));
    osc2Detune   = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("OSC2_DETUNE"));
    
    // Audit Fix [2.4]
    lineMix      = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("LINE_MIX"));

    detuneOct    = dynamic_cast<juce::AudioParameterInt*>(apvts->getParameter("DETUNE_OCT"));
    detuneCoarse = dynamic_cast<juce::AudioParameterInt*>(apvts->getParameter("DETUNE_COARSE"));
    detuneFine   = dynamic_cast<juce::AudioParameterInt*>(apvts->getParameter("DETUNE_FINE"));
    hardSync     = dynamic_cast<juce::AudioParameterBool*>(apvts->getParameter("HARD_SYNC"));
    ringMod      = dynamic_cast<juce::AudioParameterBool*>(apvts->getParameter("RING_MOD"));
    glideTime    = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("GLIDE"));
    
    lfoWaveform  = dynamic_cast<juce::AudioParameterChoice*>(apvts->getParameter("LFO_WAVE"));
    lfoRate      = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("LFO_RATE"));
    lfoDepth     = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("LFO_DEPTH"));
    lfoDelay     = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("LFO_DELAY"));

    chorusRate   = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("CHORUS_RATE"));
    chorusDepth  = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("CHORUS_DEPTH"));
    chorusMix    = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("CHORUS_MIX"));
    delayTime    = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("DELAY_TIME"));
    delayFeedback= dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("DELAY_FEEDBACK"));
    delayMix     = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("DELAY_MIX"));
    reverbSize   = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("REVERB_SIZE"));
    reverbMix    = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("REVERB_MIX"));
    
    dcaAttack    = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("DCA_ATTACK"));
    dcaDecay     = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("DCA_DECAY"));
    dcaSustain   = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("DCA_SUSTAIN"));
    dcaRelease   = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("DCA_RELEASE"));
    
    dcwAttack    = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("DCW_ATTACK"));
    dcwDecay     = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("DCW_DECAY"));
    dcwSustain   = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("DCW_SUSTAIN"));
    dcwRelease   = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("DCW_RELEASE"));
    
    protectSwitch = dynamic_cast<juce::AudioParameterBool*>(apvts->getParameter("PROTECT_SWITCH"));
    systemPrg     = dynamic_cast<juce::AudioParameterBool*>(apvts->getParameter("SYSTEM_PRG"));
    masterVolume   = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("MASTER_VOLUME"));
    masterVolume   = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("MASTER_VOLUME"));
    operationMode  = dynamic_cast<juce::AudioParameterChoice*>(apvts->getParameter("OPERATION_MODE"));

    modernLpfCutoff = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("MODERN_LPF_CUTOFF"));
    modernLpfReso   = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("MODERN_LPF_RESO"));
    modernHpfCutoff = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("MODERN_HPF_CUTOFF"));

    modVeloToDcw     = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("MOD_VELO_DCW"));
    modVeloToDca     = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("MOD_VELO_DCA"));
    modWheelToDcw    = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("MOD_WHEEL_DCW"));
    modWheelToLfoRate = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("MOD_WHEEL_LFORATE"));
    modWheelToVibrato = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("MOD_WHEEL_VIB"));
    modAtToDcw       = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("MOD_AT_DCW"));
    modAtToVibrato   = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("MOD_AT_VIB"));

    keyTrackDcw      = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("KEY_TRACK_DCW"));
    keyTrackPitch    = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("KEY_TRACK_PITCH"));
    keyFollowDco     = dynamic_cast<juce::AudioParameterChoice*>(apvts->getParameter("KEY_FOLLOW_DCO"));
    keyFollowDcw     = dynamic_cast<juce::AudioParameterChoice*>(apvts->getParameter("KEY_FOLLOW_DCW"));
    keyFollowDca     = dynamic_cast<juce::AudioParameterChoice*>(apvts->getParameter("KEY_FOLLOW_DCA"));

    midiChannel      = dynamic_cast<juce::AudioParameterInt*>(apvts->getParameter("MIDI_CH"));
    masterTune       = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("MASTER_TUNE"));
    pitchBendRange   = dynamic_cast<juce::AudioParameterInt*>(apvts->getParameter("PITCH_BEND_RANGE"));
    pitchBendRange   = dynamic_cast<juce::AudioParameterInt*>(apvts->getParameter("PITCH_BEND_RANGE"));
    keyTranspose     = dynamic_cast<juce::AudioParameterInt*>(apvts->getParameter("KEY_TRANSPOSE"));

    // Arpeggiator
    arpEnabled = dynamic_cast<juce::AudioParameterBool*>(apvts->getParameter("ARP_ENABLED"));
    arpLatch   = dynamic_cast<juce::AudioParameterBool*>(apvts->getParameter("ARP_LATCH"));
    arpRate    = dynamic_cast<juce::AudioParameterChoice*>(apvts->getParameter("ARP_RATE"));
    arpBpm     = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("ARP_BPM"));
    arpGate    = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("ARP_GATE"));
    arpSwing   = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("ARP_SWING"));
    arpPattern = dynamic_cast<juce::AudioParameterChoice*>(apvts->getParameter("ARP_PATTERN"));
    arpOctave  = dynamic_cast<juce::AudioParameterInt*>(apvts->getParameter("ARP_OCTAVE"));

    // Macros
    macroBrilliance = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("MACRO_BRILLIANCE"));
    macroTone       = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("MACRO_TONE"));
    macroSpace      = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter("MACRO_SPACE"));

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
