/*
 * SysExManager.cpp - CZ-101 SysEx Parser (AUTHENTIC DUAL LINE)
 */

#include "SysExManager.h"
#include "../State/ParameterIDs.h"
#include "../State/EnvelopeSerializer.h"
#include <juce_core/juce_core.h>
#include <cmath>
#include <array>
#include <cstdint>
#include <algorithm> // Added for std::clamp

using std::uint8_t;

namespace CZ101 {
namespace MIDI {

static uint8_t decodeNibblePair(const uint8_t* payload, int& offset, int maxSize) {
    if (offset + 2 > maxSize) {
        offset = maxSize; 
        return 0;
    }
    uint8_t lowNibble = payload[offset++] & 0x0F;
    uint8_t highNibble = payload[offset++] & 0x0F;
    return (highNibble << 4) | lowNibble;
}

// Audit Fix 10.4: Correct Mapping (0-99 -> 0.0-1.0 Normalized)
// Previously this returned Seconds, which was WRONG because PresetManager expects normalized 0-1
static float mapCZRateToNormalized(uint8_t rate) {
    rate = std::min(rate, static_cast<uint8_t>(99));
    return static_cast<float>(rate) / 99.0f; 
}

static float mapCZLevelToNormal(uint8_t level) {
    level = std::min(level, static_cast<uint8_t>(99));
    return static_cast<float>(level) / 99.0f;
}

static float mapCZDepth(uint8_t depthVal) {
    depthVal = std::min(depthVal, static_cast<uint8_t>(99));
    return static_cast<float>(depthVal) / 99.0f;
}

// Removed legacy encodeWave/decodeWave since we now read the bits directly for Kasploosh support

// ── Shared single-patch decoder ───────────────────────────────────────────
// Decodes one CZ-101/CZ-1 tone dump starting at `offset` into `preset`.
// Advances `offset` past the decoded patch (including optional CZ-1 name).
// Returns true if at least one byte was consumed (patch decoded).
// Used by both handleSysEx (bulk dump loop) and decodePatch (single-patch).
static bool decodeOnePatch(const uint8_t* msg, int& offset, int maxSize, CZ101::State::Preset& preset)
{
    const int startOffset = offset;

    // --- 1. PFLAG: Line Select & Octave ---
    uint8_t pflag = decodeNibblePair(msg, offset, maxSize);
    preset.parameters[ParameterIDs::lineSelect.toStdString()] = (float)(pflag & 0x03);
    uint8_t octaveRaw = (pflag >> 2) & 0x03;
    float octaveVal = 0.0f;
    if (octaveRaw == 1) octaveVal = 1.0f;
    else if (octaveRaw == 2) octaveVal = -1.0f;
    preset.parameters[ParameterIDs::octave.toStdString()] = octaveVal;

    // --- 2-4. Detune: PDS (sign), PDL (fine), PDH (semitones) ---
    uint8_t pds = decodeNibblePair(msg, offset, maxSize);
    uint8_t pdl = decodeNibblePair(msg, offset, maxSize);
    uint8_t pdh = decodeNibblePair(msg, offset, maxSize);

    // Casio PDL is a gapped 0-60 fine scale: bytes 0x00-0x0F, 0x11-0x1F,
    // 0x21-0x2F, 0x31-0x3F (gaps at 0x10/0x20/0x30). Invert the gaps:
    int fineUnits = pdl - (pdl / 16);
    float fineCents = ((float)fineUnits / 60.0f) * 100.0f;
    float detune = ((float)pdh * 100.0f) + fineCents;
    if ((pds & 0x01)) detune = -detune;
    preset.parameters[ParameterIDs::osc2Detune.toStdString()] = detune / 100.0f;

    // --- 5. PVK: Vibrato Wave ---
    uint8_t pvk = decodeNibblePair(msg, offset, maxSize);
    float lfoWave = 0.0f;
    if (pvk == 0x08) lfoWave = 0.0f;
    else if (pvk == 0x04) lfoWave = 1.0f;
    else if (pvk == 0x20) lfoWave = 2.0f;
    else if (pvk == 0x02) lfoWave = 3.0f;
    preset.parameters[ParameterIDs::lfoWaveform.toStdString()] = lfoWave;

    // --- 6. Vibrato Delay (3 bytes, use MSB only) ---
    uint8_t pvd1 = decodeNibblePair(msg, offset, maxSize);
    decodeNibblePair(msg, offset, maxSize);
    decodeNibblePair(msg, offset, maxSize);
    preset.parameters[ParameterIDs::lfoDelay.toStdString()] = ((float)std::min(pvd1, (uint8_t)99) / 99.0f) * 2.0f;

    // --- 7. Vibrato Rate (3 bytes, use MSB only) ---
    uint8_t rv1 = decodeNibblePair(msg, offset, maxSize);
    decodeNibblePair(msg, offset, maxSize);
    decodeNibblePair(msg, offset, maxSize);
    preset.parameters[ParameterIDs::lfoRate.toStdString()] = mapCZRateToNormalized(rv1) * 20.0f;

    // --- 8. Vibrato Depth (3 bytes, use MSB only) ---
    uint8_t dv1 = decodeNibblePair(msg, offset, maxSize);
    decodeNibblePair(msg, offset, maxSize);
    decodeNibblePair(msg, offset, maxSize);
    preset.parameters[ParameterIDs::lfoDepth.toStdString()] = mapCZDepth(dv1);

    // --- 9. MFW: DCO1 Waveforms + Window + Modulation ---
    uint8_t mfw1 = decodeNibblePair(msg, offset, maxSize);
    uint8_t mfw1_2 = decodeNibblePair(msg, offset, maxSize);
    bool wave2En = (mfw1 & 0x10) != 0;
    float wave1 = (float)((mfw1 >> 5) & 0x07);
    float wave2 = wave2En ? ((float)((mfw1 >> 1) & 0x07) + 1.0f) : 0.0f;
    float window1 = (float)(((mfw1 & 0x01) << 2) | ((mfw1_2 >> 6) & 0x03));
    uint8_t mod = (mfw1_2 >> 3) & 0x07;
    bool modSpecial = (mfw1_2 & 0x04) != 0;

    preset.parameters[ParameterIDs::osc1Waveform.toStdString()] = wave1;
    preset.parameters[ParameterIDs::osc1Window.toStdString()] = window1;
    preset.parameters[ParameterIDs::osc1Waveform2.toStdString()] = wave2;
    
    float parsedMod = 0.0f; // Off
    if (mod == 4 || mod == 5) parsedMod = 1.0f; // Ring 1
    else if (mod == 3) parsedMod = 2.0f; // Noise 1
    else if (mod == 2) parsedMod = 3.0f; // Ring 2
    else if (mod == 6) parsedMod = 4.0f; // Ring 3
    else if (mod == 7) parsedMod = 5.0f; // Noise 2
    
    preset.parameters[ParameterIDs::lineMod.toStdString()] = parsedMod;
    preset.parameters[ParameterIDs::modSpecial.toStdString()] = modSpecial ? 1.0f : 0.0f;

    // --- 10. Key Follow Line 1 ---
    // CZ-101 SysEx: Sec9 MAMD/MAMV = DCA1 KF, Sec10 MWMD/MWMV = DCW1 KF
    // (NO pitch key follow in CZ-101 hardware)
    uint8_t dcaKfMode1 = decodeNibblePair(msg, offset, maxSize);  // MAMD = DCA1 KF mode (0-9)
    decodeNibblePair(msg, offset, maxSize);                       // MAMV = DCA1 KF value (HW lookup, ignore)
    uint8_t dcwKfMode1 = decodeNibblePair(msg, offset, maxSize);  // MWMD = DCW1 KF mode (0-9)
    decodeNibblePair(msg, offset, maxSize);                       // MWMV = DCW1 KF value (HW lookup, ignore)
    preset.parameters[ParameterIDs::line1KfDca.toStdString()] = (float)dcaKfMode1;
    preset.parameters[ParameterIDs::line1KfDcw.toStdString()] = (float)dcwKfMode1;
    // LINE1_KF_PITCH: no CZ-101 HW equivalent, stays at default 0

    // --- 11-13. Envelopes DCA1, DCW1, PITCH1 ---
    float vDca1 = 0, vDcw1 = 0, vPitch1 = 0;
    State::EnvelopeSerializer::decodeFromSysEx(msg, offset, maxSize, preset.dcaEnv, State::EnvelopeSerializer::DCA, vDca1);
    State::EnvelopeSerializer::decodeFromSysEx(msg, offset, maxSize, preset.dcwEnv, State::EnvelopeSerializer::DCW, vDcw1);
    State::EnvelopeSerializer::decodeFromSysEx(msg, offset, maxSize, preset.pitchEnv, State::EnvelopeSerializer::PITCH, vPitch1);

    preset.parameters[ParameterIDs::line1VeloDca.toStdString()] = vDca1;
    preset.parameters[ParameterIDs::line1VeloDcw.toStdString()] = vDcw1;
    preset.parameters[ParameterIDs::line1VeloPitch.toStdString()] = vPitch1;

    // --- 14. SFW: DCO2 Waveforms + Window ---
    uint8_t mfw2 = decodeNibblePair(msg, offset, maxSize);
    uint8_t mfw2_2 = decodeNibblePair(msg, offset, maxSize);
    bool wave2En2 = (mfw2 & 0x10) != 0;
    float wave1_2 = (float)((mfw2 >> 5) & 0x07);
    float wave2_2 = wave2En2 ? ((float)((mfw2 >> 1) & 0x07) + 1.0f) : 0.0f;
    float window2 = (float)(((mfw2 & 0x01) << 2) | ((mfw2_2 >> 6) & 0x03));

    preset.parameters[ParameterIDs::osc2Waveform.toStdString()] = wave1_2;
    preset.parameters[ParameterIDs::osc2Window.toStdString()] = window2;
    preset.parameters[ParameterIDs::osc2Waveform2.toStdString()] = wave2_2;

    // --- 15. Key Follow Line 2 ---
    // CZ-101 SysEx: Sec18 SAMD/SAMV = DCA2 KF, Sec19 SWMD/SWMV = DCW2 KF
    uint8_t dcaKfMode2 = decodeNibblePair(msg, offset, maxSize);  // SAMD = DCA2 KF mode (0-9)
    decodeNibblePair(msg, offset, maxSize);                       // SAMV = DCA2 KF value (HW lookup, ignore)
    uint8_t dcwKfMode2 = decodeNibblePair(msg, offset, maxSize);  // SWMD = DCW2 KF mode (0-9)
    decodeNibblePair(msg, offset, maxSize);                       // SWMV = DCW2 KF value (HW lookup, ignore)
    preset.parameters[ParameterIDs::line2KfDca.toStdString()] = (float)dcaKfMode2;
    preset.parameters[ParameterIDs::line2KfDcw.toStdString()] = (float)dcwKfMode2;
    // LINE2_KF_PITCH: no CZ-101 HW equivalent, stays at default 0

    // --- 16-18. Envelopes DCA2, DCW2, PITCH2 ---
    float vDca2 = 0, vDcw2 = 0, vPitch2 = 0;
    State::EnvelopeSerializer::decodeFromSysEx(msg, offset, maxSize, preset.dcaEnv2, State::EnvelopeSerializer::DCA, vDca2);
    State::EnvelopeSerializer::decodeFromSysEx(msg, offset, maxSize, preset.dcwEnv2, State::EnvelopeSerializer::DCW, vDcw2);
    State::EnvelopeSerializer::decodeFromSysEx(msg, offset, maxSize, preset.pitchEnv2, State::EnvelopeSerializer::PITCH, vPitch2);

    preset.parameters[ParameterIDs::line2VeloDca.toStdString()] = vDca2;
    preset.parameters[ParameterIDs::line2VeloDcw.toStdString()] = vDcw2;
    preset.parameters[ParameterIDs::line2VeloPitch.toStdString()] = vPitch2;

    // --- 19. Optional CZ-1 patch name (32 nibbles = 16 ASCII chars) ---
    if (offset + 32 <= maxSize && msg[offset] != 0xF7) {
        std::string parsedName;
        for (int i = 0; i < 16; ++i) {
            uint8_t ch = decodeNibblePair(msg, offset, maxSize);
            if (ch >= 32 && ch <= 126) parsedName += (char)ch;
        }
        parsedName.erase(parsedName.find_last_not_of(" \n\r\t") + 1);
        if (!parsedName.empty()) {
            preset.name = parsedName;
        }
    }

    return offset > startOffset;
}

void SysExManager::handleSysEx(const void* data, int size, const juce::String& patchName)
{
    if (memoryProtected || !programChangeEnabled) return;

    // Audit Fix 4.3: Robust Buffering / Running Status handling
    fragmentBuffer.append(data, size);

    while (fragmentBuffer.getSize() > 0)
    {
        const uint8_t* bytes = static_cast<const uint8_t*>(fragmentBuffer.getData());
        int totalSize = (int)fragmentBuffer.getSize();

        // Search for F0 (SYSEX_START)
        int startPos = -1;
        for (int i = 0; i < totalSize; ++i) if (bytes[i] == 0xF0) { startPos = i; break; }

        if (startPos == -1) { fragmentBuffer.reset(); break; } // No start found, discard junk
        if (startPos > 0) { fragmentBuffer.removeSection(0, startPos); continue; } // Skip leading junk

        // Check for CZ Dump Request: F0 44 00 00 7x 10 [Slot] 70 31
        if (totalSize >= 9 &&
            bytes[0] == 0xF0 &&
            bytes[1] == 0x44 &&
            bytes[2] == 0x00 &&
            bytes[3] == 0x00 &&
            (bytes[4] & 0xF0) == 0x70 &&
            bytes[5] == 0x10 &&
            bytes[7] == 0x70 &&
            bytes[8] == 0x31)
        {
            uint8_t slotId = bytes[6];
            if (onDumpRequested) {
                onDumpRequested(slotId);
            }
            fragmentBuffer.removeSection(0, 9);
            continue;
        }

        // Search for F7 (SYSEX_END)
        int endPos = -1;
        for (int i = 1; i < totalSize; ++i) {
            if (bytes[i] == 0xF7) { endPos = i; break; }
            if (bytes[i] == 0xF0 && i > 0) break; // Next message start before end? 
        }

        if (endPos == -1) {
            // Partial message, wait for more. Safety: cap at 10KB
            if (totalSize > 10000) {
                 juce::Logger::writeToLog("SysEx fragment too large (>10KB), discarding buffer to prevent overflow");
                 fragmentBuffer.reset(); 
            }
            break; 
        }

        int msgSize = endPos + 1;
        const uint8_t* msg = bytes; // F0 ... F7

        // Validation (Casio ID 0x44)
        if (msgSize < 10 || msg[1] != 0x44) {
            fragmentBuffer.removeSection(0, msgSize);
            continue;
        }

        // Checksum Check (nibble payload start at byte 7)
        uint16_t sum = 0;
        for (int i = 7; i < msgSize - 2; ++i) sum += msg[i];
        uint8_t checksum = (uint8_t)((0 - sum) & 0x7F);
        #if DEBUG
        if (checksum != msg[msgSize - 2] && msg[msgSize - 2] != 0) {
            juce::Logger::writeToLog("⚠️ SysEx Checksum Error: Expected " + juce::String::toHexString(msg[msgSize-2]) + " got " + juce::String::toHexString(checksum));
        }
        #endif

        // Audit Fix 10.4: Relax Device ID Check
        uint8_t devId = msg[5] & 0x0F;

        // Parse Patches (Bulk Dump loop) — delegates to shared decodeOnePatch
        int offset = 7;
        int patchCount = 0;

        while (offset + PAYLOAD_CZ101 < msgSize)
        {
            CZ101::State::Preset preset;
            preset.name = patchName.toStdString();
            if (patchCount > 0) preset.name += " " + std::to_string(patchCount);

            if (!decodeOnePatch(msg, offset, msgSize, preset)) break;

            if (onPresetParsed) onPresetParsed(preset);
            patchCount++;
        }

        fragmentBuffer.removeSection(0, msgSize);
    }
}

// Helper to encode byte into two nibbles
static void encodeNibblePair(uint8_t value, juce::MemoryBlock& data) {
    auto low = value & 0x0F;
    auto high = (value >> 4) & 0x0F;
    data.append(&low, 1);
    data.append(&high, 1);
}

// Audit Fix 10.4: Correct Export Mapping (Normalized 0-1 -> 0-99)
static uint8_t mapNormalizedToCZRate(float norm) {
    int rate = (int)(norm * 99.0f);
    return (uint8_t)std::clamp(rate, 0, 99);
}

static uint8_t mapNormalToCZLevel(float level) {
    return (uint8_t)(level * 99.0f);
}

juce::MemoryBlock SysExManager::createPatchDump(const CZ101::State::Preset& preset, int opMode)
{
    juce::MemoryBlock data;

    // Helper to safely get parameter
    auto getParam = [&](const std::string& key, float def = 0.0f) {
        auto it = preset.parameters.find(key);
        if (it != preset.parameters.end()) return it->second;
        return def;
    };

    // Header
    const uint8_t header[] = { 0xF0, MANUF_ID_1, MANUF_ID_2, MANUF_ID_3, DEVICE_ID_BASE, FUNC_RECV, PROG_EDIT };
    data.append(header, sizeof(header));

    // Data Body PFLAG
    uint8_t pflag = (uint8_t)getParam(ParameterIDs::lineSelect.toStdString(), 2.0f) & 0x03;
    float octaveVal = getParam(ParameterIDs::octave.toStdString(), 0.0f);
    if (octaveVal > 0.0f) pflag |= (1 << 2);
    else if (octaveVal < 0.0f) pflag |= (2 << 2);
    encodeNibblePair(pflag, data);

    float detune = getParam(ParameterIDs::osc2Detune.toStdString(), 0.0f);
    uint8_t sign = (detune < 0) ? 1 : 0;
    float absDetune = std::abs(detune);
    int semitones = (int)absDetune;
    float remainderCents = std::fmod(absDetune, 1.0f) * 100.0f;
    int fineUnits = std::clamp((int)std::round((remainderCents / 100.0f) * 60.0f), 0, 60);
    
    // Casio encoding: gapped scale (00..0F, 11..1F, 21..2F, 31..3F)
    uint8_t pdl = (uint8_t)(fineUnits + (fineUnits >= 16) + (fineUnits >= 31) + (fineUnits >= 46));
    uint8_t pdh = (uint8_t)std::clamp(semitones, 0, 48);
    
    encodeNibblePair(sign, data); // PDS
    encodeNibblePair(pdl, data);  // PDL
    encodeNibblePair(pdh, data);  // PDH

    // Vibrato
    uint8_t waveRaw = (uint8_t)getParam(ParameterIDs::lfoWaveform.toStdString(), 0.0f);
    uint8_t wave = 0x08;
    if (waveRaw == 1) wave = 0x04;
    else if (waveRaw == 2) wave = 0x20;
    else if (waveRaw == 3) wave = 0x02;
    encodeNibblePair(wave, data); // PVK
    
    float delayRaw = getParam(ParameterIDs::lfoDelay.toStdString(), 0.0f);
    int delayVal = std::clamp((int)(delayRaw / 2.0f * 99.0f), 0, 99);
    encodeNibblePair(delayVal, data); // PVDLD
    encodeNibblePair(0, data); encodeNibblePair(0, data);
    
    float normRate = getParam(ParameterIDs::lfoRate.toStdString(), 1.0f) / 20.0f;
    int rateVal = mapNormalizedToCZRate(normRate);
    encodeNibblePair(rateVal, data); // RV1
    encodeNibblePair(0, data); 
    encodeNibblePair(0, data); 
    
    float depth = getParam(ParameterIDs::lfoDepth.toStdString(), 0.0f);
    int depthVal = std::clamp((int)(depth * 99.0f), 0, 99);
    encodeNibblePair(depthVal, data); // DV1
    encodeNibblePair(0, data); 
    encodeNibblePair(0, data); 

    // 8. Waveforms Line 1
    uint8_t wave1Raw = (uint8_t)getParam(ParameterIDs::osc1Waveform.toStdString(), 0.0f);
    uint8_t wave2Raw = (uint8_t)getParam(ParameterIDs::osc1Waveform2.toStdString(), 0.0f);
    uint8_t window1 = (uint8_t)getParam(ParameterIDs::osc1Window.toStdString(), 0.0f);
    
    uint8_t m1_main = wave1Raw & 0x07;
    uint8_t m2_main = wave2Raw > 0 ? ((wave2Raw - 1) & 0x07) : 0;
    bool wave2En = (wave2Raw > 0);
    
    float lineModVal = getParam(ParameterIDs::lineMod.toStdString(), 0.0f);
    uint8_t mod = 0;
    if (lineModVal == 1.0f) mod = 4;
    else if (lineModVal == 2.0f) mod = 3;
    else if (lineModVal == 3.0f) mod = 2;
    else if (lineModVal == 4.0f) mod = 6;
    else if (lineModVal == 5.0f) mod = 7;
    
    bool modSpecial = getParam(ParameterIDs::modSpecial.toStdString(), 0.0f) > 0.5f;

    uint8_t mfw1 = (m1_main << 5) | (wave2En ? 0x10 : 0x00) | (m2_main << 1) | ((window1 >> 2) & 0x01);
    uint8_t mfw1_2 = ((window1 & 0x03) << 6) | (mod << 3) | (modSpecial ? 0x04 : 0x00);

    encodeNibblePair(mfw1, data);
    encodeNibblePair(mfw1_2, data);

    // 9-10. Key Follow Line 1
    // CZ-101 SysEx: Sec9 MAMD/MAMV = DCA1 KF, Sec10 MWMD/MWMV = DCW1 KF
    // Lookup tables for MAMV (DCA1 KF value byte) and MWMV (DCW1 KF value byte)
    static constexpr uint8_t kDcaKfValue[10] = { 0x00, 0x08, 0x11, 0x1A, 0x24, 0x2F, 0x3A, 0x45, 0x52, 0x5F };
    static constexpr uint8_t kDcwKfValue[10] = { 0x00, 0x1F, 0x2C, 0x39, 0x46, 0x53, 0x60, 0x6E, 0x92, 0xFF };
    uint8_t dcaKf1 = std::min((uint8_t)getParam(ParameterIDs::line1KfDca.toStdString(), 0.0f), (uint8_t)9);
    uint8_t dcwKf1 = std::min((uint8_t)getParam(ParameterIDs::line1KfDcw.toStdString(), 0.0f), (uint8_t)9);
    encodeNibblePair(dcaKf1, data);              // MAMD = DCA1 KF mode
    encodeNibblePair(kDcaKfValue[dcaKf1], data); // MAMV = DCA1 KF value
    encodeNibblePair(dcwKf1, data);              // MWMD = DCW1 KF mode
    encodeNibblePair(kDcwKfValue[dcwKf1], data); // MWMV = DCW1 KF value

    // 11-16. Envelopes Line 1
    float vDca1 = getParam(ParameterIDs::line1VeloDca.toStdString(), 0.0f);
    float vDcw1 = getParam(ParameterIDs::line1VeloDcw.toStdString(), 0.0f);
    float vPitch1 = getParam(ParameterIDs::line1VeloPitch.toStdString(), 0.0f);
    State::EnvelopeSerializer::encodeToSysEx(preset.dcaEnv, data, State::EnvelopeSerializer::DCA, vDca1);
    State::EnvelopeSerializer::encodeToSysEx(preset.dcwEnv, data, State::EnvelopeSerializer::DCW, vDcw1);
    State::EnvelopeSerializer::encodeToSysEx(preset.pitchEnv, data, State::EnvelopeSerializer::PITCH, vPitch1);
    
    // 17. Waveforms Line 2
    uint8_t o2w1Raw = (uint8_t)getParam(ParameterIDs::osc2Waveform.toStdString(), 0.0f);
    uint8_t o2w2Raw = (uint8_t)getParam(ParameterIDs::osc2Waveform2.toStdString(), 0.0f);
    uint8_t window2 = (uint8_t)getParam(ParameterIDs::osc2Window.toStdString(), 0.0f);
    
    uint8_t o2_m1_main = o2w1Raw & 0x07;
    uint8_t o2_m2_main = o2w2Raw > 0 ? ((o2w2Raw - 1) & 0x07) : 0;
    bool o2_wave2En = (o2w2Raw > 0);
    
    uint8_t mfw2 = (o2_m1_main << 5) | (o2_wave2En ? 0x10 : 0x00) | (o2_m2_main << 1) | ((window2 >> 2) & 0x01);
    uint8_t mfw2_2 = ((window2 & 0x03) << 6); 

    encodeNibblePair(mfw2, data);
    encodeNibblePair(mfw2_2, data);

    // 18-19. Key Follow Line 2
    // CZ-101 SysEx: Sec18 SAMD/SAMV = DCA2 KF, Sec19 SWMD/SWMV = DCW2 KF
    uint8_t dcaKf2 = std::min((uint8_t)getParam(ParameterIDs::line2KfDca.toStdString(), 0.0f), (uint8_t)9);
    uint8_t dcwKf2 = std::min((uint8_t)getParam(ParameterIDs::line2KfDcw.toStdString(), 0.0f), (uint8_t)9);
    encodeNibblePair(dcaKf2, data);              // SAMD = DCA2 KF mode
    encodeNibblePair(kDcaKfValue[dcaKf2], data); // SAMV = DCA2 KF value
    encodeNibblePair(dcwKf2, data);              // SWMD = DCW2 KF mode
    encodeNibblePair(kDcwKfValue[dcwKf2], data); // SWMV = DCW2 KF value

    // 20-25. Envelopes Line 2
    float vDca2 = getParam(ParameterIDs::line2VeloDca.toStdString(), 0.0f);
    float vDcw2 = getParam(ParameterIDs::line2VeloDcw.toStdString(), 0.0f);
    float vPitch2 = getParam(ParameterIDs::line2VeloPitch.toStdString(), 0.0f);
    State::EnvelopeSerializer::encodeToSysEx(preset.dcaEnv2, data, State::EnvelopeSerializer::DCA, vDca2);
    State::EnvelopeSerializer::encodeToSysEx(preset.dcwEnv2, data, State::EnvelopeSerializer::DCW, vDcw2);
    State::EnvelopeSerializer::encodeToSysEx(preset.pitchEnv2, data, State::EnvelopeSerializer::PITCH, vPitch2);

    // CZ-1 / CZ-5000 Name Appending
    if (opMode >= 2) {
        std::string nameToEncode = preset.name;
        if (nameToEncode.length() > 16) nameToEncode = nameToEncode.substr(0, 16);
        while (nameToEncode.length() < 16) nameToEncode += ' '; // pad with spaces
        
        for (int i = 0; i < 16; ++i) {
            encodeNibblePair((uint8_t)nameToEncode[i], data);
        }
    }

    // Checksum
    uint8_t sum = 0;
    // Sum payload bytes (after header)
    for (int i = 7; i < data.getSize(); ++i) {
        sum += (uint8_t)data[i];
    }
    uint8_t checksum = (0 - sum) & 0x7F;
    uint8_t end = 0xF7;
    data.append(&checksum, 1);
    data.append(&end, 1);

    return data;
}

bool SysExManager::decodePatch(const uint8_t* msg, CZ101::State::Preset& preset) {
    int msgSize = 264;
    int offset = 7;
    return decodeOnePatch(msg, offset, msgSize, preset);
}

} // namespace MIDI
} // namespace CZ101
