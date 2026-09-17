// ─── WASM parameter dispatch ───
// wasm_set_param / wasm_get_param and the DSP sync helper (updateWasmDSP),
// extracted from the old monolithic WasmBridge.cpp.
//
// The original wasm_set_param was an 80+ branch if-else chain; it is now a
// static std::unordered_map<string, Handler> dispatch table (built once on
// first use), so a param lookup is O(1) instead of O(n) string compares and
// each handler is a small self-contained function. The variable-length
// MOD_SLOT_<n>_<FIELD> family (which cannot be enumerated statically) is
// handled separately after the exact-match table misses.

#include "WasmState.h"
#include <unordered_map>
#include <functional>

void updateWasmDSP() {
    float actualLevel1 = gOsc1Level;
    float actualLevel2 = gOsc2Level;
    int actualOsc2Wave = gOsc2Wave;
    int actualOsc2Wave2 = gOsc2Wave2;
    int actualOsc1Window = gOsc1Window;
    int actualOsc2Window = gOsc2Window;

    if (gLineSelect == 0) { // Line 1 only
        actualLevel2 = 0.0f;
    } else if (gLineSelect == 1) { // Line 2 only
        actualLevel1 = 0.0f;
    } else if (gLineSelect == 2) { // Line 1+1' (Line 2 copies Line 1)
        actualLevel2 = actualLevel1;
        actualOsc2Wave = gOsc1Wave;
        actualOsc2Wave2 = gOsc1Wave2;
        actualOsc2Window = actualOsc1Window;
    }
    // lineSelect == 3: Line 1+2 (both lines, own waveforms) → fall through

    // LINE_MIX crossfade: only applies when both lines are active (lineSelect >= 2)
    // and the user has moved the slider away from center (0.5 = no crossfade).
    // For single-line modes (0/1), the line select already silences the other line.
    if (gLineSelect >= 2 && gLineMix != 0.5f) {
        // Map mix from [0,1] centered at 0.5: 0=Line1 only, 0.5=equal, 1=Line2 only
        actualLevel1 *= (1.0f - gLineMix) * 2.0f; // ×2 to keep unity at center
        actualLevel2 *= gLineMix * 2.0f;
    }

    gVoiceManager.setOsc1Waveforms(gOsc1Wave, gOsc1Wave2, actualOsc1Window);
    gVoiceManager.setOsc1Level(actualLevel1);
    gVoiceManager.setOsc2Waveforms(actualOsc2Wave, actualOsc2Wave2, actualOsc2Window);
    gVoiceManager.setOsc2Level(actualLevel2);
}

namespace {

// One handler per parameter id: receives the RAW value (registry units, after
// skew/normalisation). Returns nothing; the shared gWasmParams cache is
// written by wasm_set_param before dispatch.
using ParamHandler = void (*)(float rawVal);

void hLineSelect(float rawVal)         { gLineSelect = (int)std::lround(rawVal); updateWasmDSP(); }
void hOsc1Waveform(float rawVal)       { gOsc1Wave = (int)std::lround(rawVal); updateWasmDSP(); }
void hOsc1Waveform2(float rawVal)      { int idx = (int)std::lround(rawVal); gOsc1Wave2 = (idx == 0) ? 8 : idx - 1; updateWasmDSP(); }
void hOsc1Window(float rawVal)         { gOsc1Window = (int)std::lround(rawVal); updateWasmDSP(); }
void hOsc1Level(float rawVal)          { gOsc1Level = rawVal; updateWasmDSP(); }
void hOsc2Waveform(float rawVal)       { gOsc2Wave = (int)std::lround(rawVal); updateWasmDSP(); }
void hOsc2Waveform2(float rawVal)      { int idx = (int)std::lround(rawVal); gOsc2Wave2 = (idx == 0) ? 8 : idx - 1; updateWasmDSP(); }
void hOsc2Window(float rawVal)         { gOsc2Window = (int)std::lround(rawVal); updateWasmDSP(); }
void hOsc2Level(float rawVal)          { gOsc2Level = rawVal; updateWasmDSP(); }
void hLineMix(float rawVal)            { gLineMix = rawVal; updateWasmDSP(); }
void hOsc2Detune(float rawVal)         { gVoiceManager.setOsc2Detune(rawVal); }
void hLFOWave(float rawVal)            { gVoiceManager.setLFOWaveform(static_cast<CZ101::DSP::LFO::Waveform>((int)std::lround(rawVal))); }
void hDetuneOct(float rawVal)          { gWasmSnapshot.dco2.octave = (int)std::lround(rawVal); gVoiceManager.setOsc2DetuneHardware(gWasmSnapshot.dco2.octave, gWasmSnapshot.dco2.coarse, gWasmSnapshot.dco2.fine); }
void hDetuneCoarse(float rawVal)       { gWasmSnapshot.dco2.coarse = (int)std::lround(rawVal); gVoiceManager.setOsc2DetuneHardware(gWasmSnapshot.dco2.octave, gWasmSnapshot.dco2.coarse, gWasmSnapshot.dco2.fine); }
void hDetuneFine(float rawVal)         { gWasmSnapshot.dco2.fine = (int)std::lround(rawVal); gVoiceManager.setOsc2DetuneHardware(gWasmSnapshot.dco2.octave, gWasmSnapshot.dco2.coarse, gWasmSnapshot.dco2.fine); }
void hHardSync(float rawVal)           { gVoiceManager.setHardSync(rawVal > 0.5f); }
void hLineModulation(float rawVal)     { gVoiceManager.setLineModulation((int)std::lround(rawVal)); }
void hGlide(float rawVal)              { gVoiceManager.setGlideTime(rawVal); }
void hLFORate(float rawVal)            { gVoiceManager.setLFOFrequency(rawVal); }
void hLFODepth(float rawVal)           { gVoiceManager.setVibratoDepth(rawVal * 4.0f); } // ×4: CZ-101 depth 0-99 ≈ 0-4 semitones
void hLFODelay(float rawVal)           { gVoiceManager.setLFODelay(rawVal); }
// Legacy ADSR parameters are preserved in registry for DAW compatibility,
// but CZ hardware strictly uses authentic 8-stage envelopes.
void hDCAAttack(float /*rawVal*/)          { /* 8-stage envelopes take precedence */ }
void hDCADecay(float /*rawVal*/)           { /* 8-stage envelopes take precedence */ }
void hDCASustain(float /*rawVal*/)         { /* 8-stage envelopes take precedence */ }
void hDCARelease(float /*rawVal*/)         { /* 8-stage envelopes take precedence */ }
void hDCWAttack(float /*rawVal*/)          { /* 8-stage envelopes take precedence */ }
void hDCWDecay(float /*rawVal*/)           { /* 8-stage envelopes take precedence */ }
void hDCWSustain(float /*rawVal*/)         { /* 8-stage envelopes take precedence */ }
void hDCWRelease(float /*rawVal*/)         { /* 8-stage envelopes take precedence */ }
void hModernLpfCutoff(float rawVal)    { gWasmSnapshot.effects.lpfCutoff = rawVal; applyBrilliance(); }
void hModernLpfReso(float rawVal)      { gWasmSnapshot.effects.lpfReso = rawVal; applyBrilliance(); }
void hModernHpfCutoff(float rawVal)    { gWasmSnapshot.effects.hpfCutoff = rawVal; applyBrilliance(); }
void hDriveMix(float rawVal)           { gWasmSnapshot.effects.driveMix = rawVal; }
void hDriveAmount(float rawVal)        { gWasmSnapshot.effects.driveAmount = rawVal; }
void hDriveColor(float rawVal)         { gWasmSnapshot.effects.driveColor = rawVal; }
void hChorusMix(float rawVal)          { gWasmSnapshot.effects.chorusMix = rawVal; }
void hChorusRate(float rawVal)         { gWasmSnapshot.effects.chorusRate = rawVal; }
void hChorusDepth(float rawVal)        { gWasmSnapshot.effects.chorusDepth = rawVal; }
void hDelayMix(float rawVal)           { gWasmSnapshot.effects.delayMix = rawVal; }
void hDelayTime(float rawVal)          { gWasmSnapshot.effects.delayTime = rawVal; }
void hDelayFeedback(float rawVal)      { gWasmSnapshot.effects.delayFb = rawVal; }
void hReverbMix(float rawVal)          { gBaseReverbMix = rawVal; applyMacroSpace(); }
void hReverbSize(float rawVal)         { gWasmSnapshot.effects.reverbSize = rawVal; }
void hMasterTune(float rawVal)         { gVoiceManager.setMasterTune(rawVal / 100.0f); } // cents -> semitones
void hMasterVolume(float rawVal)       { gVoiceManager.setMasterVolume(rawVal); }

// All fixed-route handlers push through pushModulationMatrix so the
// Modern/Classic gating stays consistent (e.g. veloToDca is zeroed in
// Modern — the free matrix owns that route).
void hModVeloDcw(float rawVal)         { gModulationMatrix.veloToDcw = rawVal; pushModulationMatrix(); }
void hModVeloDca(float rawVal)         { gModulationMatrix.veloToDca = rawVal; pushModulationMatrix(); }
void hModWheelVib(float rawVal)        { gModulationMatrix.wheelToVibrato = rawVal; pushModulationMatrix(); }
void hModWheelDcw(float rawVal)        { gModulationMatrix.wheelToDcw = rawVal; pushModulationMatrix(); }
void hModWheelLfoRate(float rawVal)    { gModulationMatrix.wheelToLfoRate = rawVal; pushModulationMatrix(); }
void hModAtDcw(float rawVal)           { gModulationMatrix.atToDcw = rawVal; pushModulationMatrix(); }
void hModAtVib(float rawVal)           { gModulationMatrix.atToVibrato = rawVal; pushModulationMatrix(); }
void hKeyTrackPitch(float rawVal)      { gModulationMatrix.keyTrackPitch = rawVal; pushModulationMatrix(); }
void hKeyTrackDcw(float rawVal)        { gModulationMatrix.keyTrackDcw = rawVal; pushModulationMatrix(); }
void hKeyFollowDco(float rawVal)       { gModulationMatrix.kfDco = (int)std::lround(rawVal); pushModulationMatrix(); }
void hKeyFollowDcw(float rawVal)       { gModulationMatrix.kfDcw = (int)std::lround(rawVal); pushModulationMatrix(); }
void hKeyFollowDca(float rawVal)       { gModulationMatrix.kfDca = (int)std::lround(rawVal); pushModulationMatrix(); }

void hLine1VeloPitch(float rawVal)     { gModulationMatrix.line1VeloPitch = rawVal; pushModulationMatrix(); }
void hLine1VeloDcw(float rawVal)       { gModulationMatrix.line1VeloDcw = rawVal; pushModulationMatrix(); }
void hLine1VeloDca(float rawVal)       { gModulationMatrix.line1VeloDca = rawVal; pushModulationMatrix(); }
void hLine2VeloPitch(float rawVal)     { gModulationMatrix.line2VeloPitch = rawVal; pushModulationMatrix(); }
void hLine2VeloDcw(float rawVal)       { gModulationMatrix.line2VeloDcw = rawVal; pushModulationMatrix(); }
void hLine2VeloDca(float rawVal)       { gModulationMatrix.line2VeloDca = rawVal; pushModulationMatrix(); }

void hLine1KfPitch(float rawVal)       { gModulationMatrix.line1KfPitch = rawVal; pushModulationMatrix(); }
void hLine1KfDcw(float rawVal)         { gModulationMatrix.line1KfDcw = rawVal; pushModulationMatrix(); }
void hLine1KfDca(float rawVal)         { gModulationMatrix.line1KfDca = rawVal; pushModulationMatrix(); }
void hLine2KfPitch(float rawVal)       { gModulationMatrix.line2KfPitch = rawVal; pushModulationMatrix(); }
void hLine2KfDcw(float rawVal)         { gModulationMatrix.line2KfDcw = rawVal; pushModulationMatrix(); }
void hLine2KfDca(float rawVal)         { gModulationMatrix.line2KfDca = rawVal; pushModulationMatrix(); }

void hModSpecial(float rawVal)         { gVoiceManager.setModSpecial(rawVal > 0.5f); }
void hMacroBrilliance(float rawVal)    { gMacroBrilliance = rawVal; applyBrilliance(); }
void hMacroTone(float rawVal)          { gMacroTone = rawVal; applyTone(); }
void hMacroSpace(float rawVal)         { gMacroSpace = rawVal; applyMacroSpace(); }
void hArpEnabled(float rawVal)         { gArpEnabled = rawVal > 0.5f; applyArpState(); }
void hArpLatch(float rawVal)           { gVoiceManager.getArpeggiator().setLatch(rawVal > 0.5f); }
void hArpRate(float rawVal)            { gVoiceManager.getArpeggiator().setRate(static_cast<CZ101::DSP::Arpeggiator::Rate>((int)std::lround(rawVal))); }
void hArpBpm(float rawVal)             { gVoiceManager.getArpeggiator().setTempo(rawVal); }
void hArpGate(float rawVal)            { gVoiceManager.getArpeggiator().setGateTime(rawVal); }
void hArpSwing(float rawVal)           { gVoiceManager.getArpeggiator().setSwing(rawVal); }
void hArpSwingMode(float rawVal)       { gVoiceManager.getArpeggiator().setSwingMode(static_cast<CZ101::DSP::Arpeggiator::SwingMode>((int)std::lround(rawVal))); }
void hArpPattern(float rawVal)         { gVoiceManager.getArpeggiator().setPattern(static_cast<CZ101::DSP::Arpeggiator::Pattern>((int)std::lround(rawVal))); }
void hArpOctave(float rawVal)          { gVoiceManager.getArpeggiator().setOctaveRange((int)std::lround(rawVal)); }

void hOperationMode(float rawVal) {
    int mode = (int)std::lround(rawVal);
    gWasmSnapshot.system.opMode = mode;
    // Shared mapping with the native plugin: CZ101 (4v), CZ5000 (8v), Modern (16v).
    gVoiceManager.setOperationMode(mode);
    applyArpState();        // Classic 101 disables the arpeggiator
    pushModulationMatrix(); // routing matrix is Modern-only
    applyTone();            // macros (Brilliance/Tone/Space) are Modern-only
    applyBrilliance();
    applyMacroSpace();
}
void hOversampling(float rawVal) {
    // Choice: 0 = 1x (Eco), 1 = 2x, 2 = 4x
    gVoiceManager.setOversamplingFactor(1 << (int)std::lround(rawVal));
}
void hHardwareNoise(float rawVal) {
    gVoiceManager.setHardwareNoiseEnabled(rawVal > 0.5f);
    gWasmSnapshot.system.hardwareNoise = rawVal > 0.5f;
}
void hMidiCh(float rawVal) {
    // Registry stores 1..16 (matching the APVTS). 0 would be OMNI but the
    // WebUI exposes only 1..16, same as the native plugin UI.
    getMidiProcessor().setMidiChannel((int)std::lround(rawVal));
}
void hPitchBendRange(float rawVal) {
    // Registry stores 0..12 semitones (APVTS default 2).
    getMidiProcessor().setPitchBendRange((int)std::lround(rawVal));
}
void hKeyTranspose(float rawVal) {
    // Registry stores -12..+12 semitones. Applies to note-on/off from the
    // web keyboard AND incoming MIDI (shifts before VoiceManager).
    gKeyTranspose = (int)std::lround(rawVal);
    getMidiProcessor().setKeyTranspose(gKeyTranspose);
}

// PROTECT_SWITCH: persisted in gWasmParams (presets/banks). Memory protection
// has no audio component; it takes effect through the native JUCE gateway.

// Build the exact-match dispatch table once (first call is cheap; the map is
// immutable afterwards). Note: do NOT register PROTECT_SWITCH — it is a no-op
// for the DSP (kept in gWasmParams by the common path).
const std::unordered_map<std::string, ParamHandler>& paramHandlers() {
    static const std::unordered_map<std::string, ParamHandler> handlers = {
        { "LINE_SELECT", hLineSelect },
        { "OSC1_WAVEFORM", hOsc1Waveform },
        { "OSC1_WAVEFORM2", hOsc1Waveform2 },
        { "OSC1_WINDOW", hOsc1Window },
        { "OSC1_LEVEL", hOsc1Level },
        { "OSC2_WAVEFORM", hOsc2Waveform },
        { "OSC2_WAVEFORM2", hOsc2Waveform2 },
        { "OSC2_WINDOW", hOsc2Window },
        { "OSC2_LEVEL", hOsc2Level },
        { "LINE_MIX", hLineMix },
        { "OSC2_DETUNE", hOsc2Detune },
        { "LFO_WAVE", hLFOWave },
        { "DETUNE_OCT", hDetuneOct },
        { "DETUNE_COARSE", hDetuneCoarse },
        { "DETUNE_FINE", hDetuneFine },
        { "HARD_SYNC", hHardSync },
        { "LINE_MODULATION", hLineModulation },
        { "GLIDE", hGlide },
        { "LFO_RATE", hLFORate },
        { "LFO_DEPTH", hLFODepth },
        { "LFO_DELAY", hLFODelay },
        { "DCA_ATTACK", hDCAAttack },
        { "DCA_DECAY", hDCADecay },
        { "DCA_SUSTAIN", hDCASustain },
        { "DCA_RELEASE", hDCARelease },
        { "DCW_ATTACK", hDCWAttack },
        { "DCW_DECAY", hDCWDecay },
        { "DCW_SUSTAIN", hDCWSustain },
        { "DCW_RELEASE", hDCWRelease },
        { "MODERN_LPF_CUTOFF", hModernLpfCutoff },
        { "MODERN_LPF_RESO", hModernLpfReso },
        { "MODERN_HPF_CUTOFF", hModernHpfCutoff },
        { "DRIVE_MIX", hDriveMix },
        { "DRIVE_AMOUNT", hDriveAmount },
        { "DRIVE_COLOR", hDriveColor },
        { "CHORUS_MIX", hChorusMix },
        { "CHORUS_RATE", hChorusRate },
        { "CHORUS_DEPTH", hChorusDepth },
        { "DELAY_MIX", hDelayMix },
        { "DELAY_TIME", hDelayTime },
        { "DELAY_FEEDBACK", hDelayFeedback },
        { "REVERB_MIX", hReverbMix },
        { "REVERB_SIZE", hReverbSize },
        { "MASTER_TUNE", hMasterTune },
        { "MASTER_VOLUME", hMasterVolume },
        { "MOD_VELO_DCW", hModVeloDcw },
        { "MOD_VELO_DCA", hModVeloDca },
        { "MOD_WHEEL_VIB", hModWheelVib },
        { "MOD_WHEEL_DCW", hModWheelDcw },
        { "MOD_WHEEL_LFORATE", hModWheelLfoRate },
        { "MOD_AT_DCW", hModAtDcw },
        { "MOD_AT_VIB", hModAtVib },
        { "KEY_TRACK_PITCH", hKeyTrackPitch },
        { "KEY_TRACK_DCW", hKeyTrackDcw },
        { "KEY_FOLLOW_DCO", hKeyFollowDco },
        { "KEY_FOLLOW_DCW", hKeyFollowDcw },
        { "KEY_FOLLOW_DCA", hKeyFollowDca },
        { "LINE1_VELO_PITCH", hLine1VeloPitch },
        { "LINE1_VELO_DCW", hLine1VeloDcw },
        { "LINE1_VELO_DCA", hLine1VeloDca },
        { "LINE2_VELO_PITCH", hLine2VeloPitch },
        { "LINE2_VELO_DCW", hLine2VeloDcw },
        { "LINE2_VELO_DCA", hLine2VeloDca },
        { "LINE1_KF_PITCH", hLine1KfPitch },
        { "LINE1_KF_DCW", hLine1KfDcw },
        { "LINE1_KF_DCA", hLine1KfDca },
        { "LINE2_KF_PITCH", hLine2KfPitch },
        { "LINE2_KF_DCW", hLine2KfDcw },
        { "LINE2_KF_DCA", hLine2KfDca },
        { "MOD_SPECIAL", hModSpecial },
        { "MACRO_BRILLIANCE", hMacroBrilliance },
        { "MACRO_TONE", hMacroTone },
        { "MACRO_SPACE", hMacroSpace },
        { "ARP_ENABLED", hArpEnabled },
        { "ARP_LATCH", hArpLatch },
        { "ARP_RATE", hArpRate },
        { "ARP_BPM", hArpBpm },
        { "ARP_GATE", hArpGate },
        { "ARP_SWING", hArpSwing },
        { "ARP_SWING_MODE", hArpSwingMode },
        { "ARP_PATTERN", hArpPattern },
        { "ARP_OCTAVE", hArpOctave },
        { "OPERATION_MODE", hOperationMode },
        { "OVERSAMPLING_QUALITY", hOversampling },
        { "HARDWARE_NOISE", hHardwareNoise },
        { "MIDI_CH", hMidiCh },
        { "PITCH_BEND_RANGE", hPitchBendRange },
        { "KEY_TRANSPOSE", hKeyTranspose },
        // OCTAVE: shifts all notes by ±1 octave (±12 semitones)
        { "OCTAVE", [](float rawVal) { gOctave = (int)std::lround(rawVal); getMidiProcessor().setOctaveShift(gOctave); } },
        { "BYPASS", [](float) {} },
        { "SYSTEM_PRG", [](float) {} }
    };
    return handlers;
}

// Free matrix (ABDEEP-style): MOD_SLOT_<n>_SRC / _DEST / _DEPTH. Slot ids are
// 1-based in the param id; array index is 0-based. Cannot live in the
// exact-match table, so it is dispatched separately.
void handleModSlot(const juce::String& paramId, float rawVal) {
    const juce::String id = paramId.fromFirstOccurrenceOf("MOD_SLOT_", false, false);
    const int sep = id.indexOfChar('_');
    if (sep > 0)
    {
        const int slotIdx = id.substring(0, sep).getIntValue() - 1;
        const juce::String field = id.substring(sep + 1);
        if (slotIdx >= 0 && slotIdx < CZ101::Core::Voice::ModulationMatrix::kNumModSlots)
        {
            auto& slot = gModulationMatrix.slots[slotIdx];
            if (field == "SRC")      slot.source = (int)std::lround(rawVal);
            else if (field == "DEST") slot.dest  = (int)std::lround(rawVal);
            else if (field == "DEPTH") slot.depth = rawVal;
            pushModulationMatrix();
        }
    }
}

} // namespace

EMSCRIPTEN_KEEPALIVE void wasm_set_param(const char* paramIdStr, float normalizedVal) {
    if (!paramIdStr) return;
    float norm = std::clamp(normalizedVal, 0.0f, 1.0f);
    gWasmParams[paramIdStr] = norm;

    juce::String paramId(paramIdStr);
    const auto* spec = CZ101::ParameterRegistry::findSpec(paramId);

    float rawVal = rawFromNormalized(spec, norm);

    const auto& handlers = paramHandlers();
    const auto it = handlers.find(paramIdStr);
    if (it != handlers.end())
    {
        it->second(rawVal);
        return;
    }
    if (paramId.startsWith("MOD_SLOT_"))
        handleModSlot(paramId, rawVal);
}

EMSCRIPTEN_KEEPALIVE float wasm_get_param(const char* paramIdStr) {
    if (!paramIdStr) return 0.0f;
    auto it = gWasmParams.find(paramIdStr);
    if (it != gWasmParams.end()) return it->second;

    // Fallback to spec default value if not cached yet
    const auto* spec = CZ101::ParameterRegistry::findSpec(paramIdStr);
    if (spec) {
        if (spec->maxValue > spec->minValue) {
            return normalizedFromRaw(spec, spec->defaultValue);
        }
        return spec->defaultValue;
    }
    return 0.0f;
}
