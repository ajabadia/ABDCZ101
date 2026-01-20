#pragma once

#include <juce_core/juce_core.h>

namespace CZ101 {
namespace ParameterIDs {

    // --- Oscillators ---
    inline const juce::String lineSelect     { "LINE_SELECT" };
    inline const juce::String osc1Waveform   { "OSC1_WAVEFORM" };
    inline const juce::String osc1Waveform2  { "OSC1_WAVEFORM2" };
    inline const juce::String osc1Level      { "OSC1_LEVEL" };
    inline const juce::String osc2Waveform   { "OSC2_WAVEFORM" };
    inline const juce::String osc2Waveform2  { "OSC2_WAVEFORM2" };
    inline const juce::String osc2Level      { "OSC2_LEVEL" };
    inline const juce::String osc2Detune     { "OSC2_DETUNE" };
    inline const juce::String detuneOct      { "DETUNE_OCT" };
    inline const juce::String detuneCoarse   { "DETUNE_COARSE" };
    inline const juce::String detuneFine     { "DETUNE_FINE" };
    inline const juce::String lineMix        { "LINE_MIX" };
    inline const juce::String hardSync       { "HARD_SYNC" };
    inline const juce::String ringMod        { "RING_MOD" };
    inline const juce::String noiseMod       { "NOISE_MOD" };
    inline const juce::String glideTime      { "GLIDE" };

    // --- LFO / Vibrato ---
    inline const juce::String lfoWaveform    { "LFO_WAVE" };
    inline const juce::String lfoRate        { "LFO_RATE" };
    inline const juce::String lfoDepth       { "LFO_DEPTH" };
    inline const juce::String lfoDelay       { "LFO_DELAY" };

    // --- Envelopes (DCA/DCW) ---
    inline const juce::String dcaAttack      { "DCA_ATTACK" };
    inline const juce::String dcaDecay       { "DCA_DECAY" };
    inline const juce::String dcaSustain     { "DCA_SUSTAIN" };
    inline const juce::String dcaRelease     { "DCA_RELEASE" };
    inline const juce::String dcwAttack      { "DCW_ATTACK" };
    inline const juce::String dcwDecay       { "DCW_DECAY" };
    inline const juce::String dcwSustain     { "DCW_SUSTAIN" };
    inline const juce::String dcwRelease     { "DCW_RELEASE" };

    // --- Modulation Matrix ---
    inline const juce::String modVeloDcw      { "MOD_VELO_DCW" };
    inline const juce::String modVeloDca      { "MOD_VELO_DCA" };
    inline const juce::String modWheelDcw     { "MOD_WHEEL_DCW" };
    inline const juce::String modWheelLfoRate { "MOD_WHEEL_LFORATE" };
    inline const juce::String modWheelVib     { "MOD_WHEEL_VIB" };
    inline const juce::String modAtDcw        { "MOD_AT_DCW" };
    inline const juce::String modAtVib        { "MOD_AT_VIB" };
    inline const juce::String keyTrackDcw     { "KEY_TRACK_DCW" };
    inline const juce::String keyTrackPitch   { "KEY_TRACK_PITCH" };
    inline const juce::String keyFollowDco    { "KEY_FOLLOW_DCO" };
    inline const juce::String keyFollowDcw    { "KEY_FOLLOW_DCW" };
    inline const juce::String keyFollowDca    { "KEY_FOLLOW_DCA" };

    // --- Modern Filters ---
    inline const juce::String lpfCutoff      { "MODERN_LPF_CUTOFF" };
    inline const juce::String lpfReso        { "MODERN_LPF_RESO" };
    inline const juce::String hpfCutoff      { "MODERN_HPF_CUTOFF" };

    // --- Effects ---
    inline const juce::String driveAmount    { "DRIVE_AMOUNT" };
    inline const juce::String driveColor     { "DRIVE_COLOR" };
    inline const juce::String driveMix       { "DRIVE_MIX" };
    inline const juce::String chorusRate     { "CHORUS_RATE" };
    inline const juce::String chorusDepth    { "CHORUS_DEPTH" };
    inline const juce::String chorusMix      { "CHORUS_MIX" };
    inline const juce::String delayTime      { "DELAY_TIME" };
    inline const juce::String delayFeedback  { "DELAY_FEEDBACK" };
    inline const juce::String delayMix       { "DELAY_MIX" };
    inline const juce::String reverbSize     { "REVERB_SIZE" };
    inline const juce::String reverbMix      { "REVERB_MIX" };

    // --- System ---
    inline const juce::String protectSwitch   { "PROTECT_SWITCH" };
    inline const juce::String systemPrg       { "SYSTEM_PRG" };
    inline const juce::String bypass          { "BYPASS" };
    inline const juce::String operationMode   { "OPERATION_MODE" };
    inline const juce::String masterVolume    { "MASTER_VOLUME" };
    inline const juce::String midiChannel     { "MIDI_CH" };
    inline const juce::String masterTune      { "MASTER_TUNE" };
    inline const juce::String benderRange     { "PITCH_BEND_RANGE" };
    inline const juce::String transpose       { "KEY_TRANSPOSE" };
    inline const juce::String oversampling    { "OVERSAMPLING_QUALITY" };
    inline const juce::String hardwareNoise   { "HARDWARE_NOISE" };

    // --- Arpeggiator ---
    inline const juce::String arpEnabled     { "ARP_ENABLED" };
    inline const juce::String arpLatch       { "ARP_LATCH" };
    inline const juce::String arpRate        { "ARP_RATE" };
    inline const juce::String arpBpm         { "ARP_BPM" };
    inline const juce::String arpGate        { "ARP_GATE" };
    inline const juce::String arpSwing       { "ARP_SWING" };
    inline const juce::String arpSwingMode   { "ARP_SWING_MODE" };
    inline const juce::String arpPattern     { "ARP_PATTERN" };
    inline const juce::String arpOctave      { "ARP_OCTAVE" };

    // --- Performance Macros ---
    inline const juce::String macroBrilliance { "MACRO_BRILLIANCE" };
    inline const juce::String macroTone       { "MACRO_TONE" };
    inline const juce::String macroSpace      { "MACRO_SPACE" };

} // namespace ParameterIDs
} // namespace CZ101
