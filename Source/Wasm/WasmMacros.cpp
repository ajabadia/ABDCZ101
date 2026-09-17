// ─── WASM performance macros ───
// The Modern-only Brilliance / Tone / Space macros, the arpeggiator gating and
// the modulation-matrix push (with its Modern/Classic gating) extracted from
// the old monolithic WasmBridge.cpp. Each helper is re-applied whenever its
// inputs or the operation mode change.

#include "WasmState.h"

// The performance macros (Brilliance/Tone/Space) are a Modern-only feature of
// this emulator; in Classic modes they are neutralized (same as the native
// calculateMacros). Each helper is re-applied whenever its inputs or the
// operation mode change.
// Modern-only per-voice filters (LPF cutoff/reso + HPF cutoff). Outside
// Modern the per-voice filters are forced fully open so the Modern-only filter
// params cannot color the authentic Classic models (the native UI hides the
// Filter panel there and the engine must match). The post-chain LadderFilter /
// StateVariableTPT are gated separately inside EffectsChain.
void applyBrilliance() noexcept
{
    const bool modern = (gWasmSnapshot.system.opMode >= 3);
    const float offset = modern ? (gMacroBrilliance - 0.5f) * 2000.0f : 0.0f;
    const float cutoff = modern
        ? std::clamp(gWasmSnapshot.effects.lpfCutoff + offset, 20.0f, 20000.0f)
        : 20000.0f; // fully open in Classic modes
    gVoiceManager.setFilterCutoff(cutoff);
    gVoiceManager.setFilterResonance(modern ? gWasmSnapshot.effects.lpfReso : 0.0f);
    gVoiceManager.setHPF(modern ? gWasmSnapshot.effects.hpfCutoff : 20.0f); // open HPF
}

void applyTone() noexcept
{
    const bool modern = (gWasmSnapshot.system.opMode >= 3);
    // Same formula as native calculateMacros: 2x speed per +0.5 above center.
    gVoiceManager.setToneRateScale(modern ? std::pow(2.0f, (gMacroTone - 0.5f) * 2.0f) : 1.0f);
}

// Space macro: reverbMix = base REVERB_MIX + macroSpace * 0.5 (same as the
// native buildAudioSnapshot). Re-applied whenever either value changes.
void applyMacroSpace() noexcept
{
    const bool modern = (gWasmSnapshot.system.opMode >= 3);
    gWasmSnapshot.effects.reverbMix = std::clamp(gBaseReverbMix + (modern ? gMacroSpace * 0.5f : 0.0f), 0.0f, 1.0f);
}

// Arpeggiator availability mirrors the native plugin: it is only usable when
// the operation mode is not Classic 101 (snap->arp.enabled = arpEnabled && opMode != 0).
void applyArpState() noexcept
{
    gVoiceManager.getArpeggiator().setEnabled(gArpEnabled && gWasmSnapshot.system.opMode != 0);
}

// The routing-matrix additions (wheel->dcw, wheel->lfo rate, aftertouch routes)
// are a Modern-only feature of this emulator; CZ-101/CZ-5000 had no such
// matrix. The hardware-authentic sensitivities (velo->dcw/dca, wheel->vib,
// key track/follow) are pushed unconditionally by their own handlers, so this
// helper only zeros the Modern additions when not in Modern mode. Mirrors
// Voice::applySnapshot gating in the native plugin.
void pushModulationMatrix() noexcept
{
    CZ101::Core::Voice::ModulationMatrix m = gModulationMatrix;
    // Key Follow MODES are mode switches, not amounts: the authentic hardware
    // curve is applied at FULL tracking (amount = 1.0) whenever FIX or VAR is
    // selected. Mirrors Voice::applySnapshot, which forces
    // keyTrackDcw/keyTrackPitch to 1.0 (or 0.0) from the KF mode. Without this
    // the curve would be scaled by the stored KEY_TRACK_* amount (default 0)
    // and KF FIX/VAR would be silent in the WASM engine.
    m.keyTrackDcw = m.kfDcw > 0 ? 1.0f : 0.0f;
    m.keyTrackPitch = m.kfDco > 0 ? 1.0f : 0.0f;
    const bool modern = (gWasmSnapshot.system.opMode >= 3);
    if (!modern)
    {
        m.wheelToDcw = 0.0f;
        m.wheelToLfoRate = 0.0f;
        m.atToDcw = 0.0f;
        m.atToVibrato = 0.0f;
        // The free 8-slot matrix is a Modern-only feature (like the wheel/AT
        // routes); CZ-101/CZ-5000 had no such matrix.
        for (auto& slot : m.slots)
        {
            slot.source = 0;
            slot.dest = 0;
            slot.depth = 0.0f;
        }
    }
    else
    {
        // In Modern the free matrix owns the routing: the WebUI seeds a
        // Velocity→DCA slot on new presets, so the fixed hardware Velo→DCA
        // route is zeroed to avoid double velocity→amp. (Mirrors
        // Voice::applySnapshot.)
        m.veloToDca = 0.0f;
    }
    gVoiceManager.setModulationMatrix(m);
}
