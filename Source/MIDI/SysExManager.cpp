/*
 * SysExManager.cpp - CZ-101 SysEx Parser (AUTHENTIC DUAL LINE)
 */

#include "SysExManager.h"
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

static float mapCZRateToSeconds(uint8_t rate) {
    rate = std::min(rate, static_cast<uint8_t>(99));
    float r = (99.0f - static_cast<float>(rate)) / 99.0f;
    return 0.001f + (std::pow(r, 4.0f) * 30.0f); // Consistent with MultiStageEnv
}

static float mapCZLevelToNormal(uint8_t level) {
    level = std::min(level, static_cast<uint8_t>(99));
    return static_cast<float>(level) / 99.0f;
}

static float mapCZDepth(uint8_t depthVal) {
    depthVal = std::min(depthVal, static_cast<uint8_t>(99));
    return static_cast<float>(depthVal) / 99.0f;
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
        if (checksum != msg[msgSize - 2]) {
            juce::Logger::writeToLog("⚠️ SysEx Checksum Error: Expected " + juce::String::toHexString(msg[msgSize-2]) + " got " + juce::String::toHexString(checksum));
        }

        // Device ID Check
        uint8_t devId = msg[5] & 0x0F;
        if (devId != 0) { 
             fragmentBuffer.removeSection(0, msgSize);
             continue; // Wait... should we break or skip? Skip this message.
        }

        // Parse Patches (Bulk Dump loop)
        int offset = 7;
        int patchCount = 0;
        
        auto decodeEnv = [&](CZ101::State::EnvelopeData& env, int& off, int maxSize) {
            if (off + 2 > maxSize) return;
            uint8_t endByte = decodeNibblePair(msg, off, maxSize);
            env.endPoint = endByte & 0x07;
            env.sustainPoint = -1;
            for (int i = 0; i < 8; ++i) {
                uint8_t rawRate = decodeNibblePair(msg, off, maxSize);
                uint8_t rawLevel = decodeNibblePair(msg, off, maxSize);
                if (rawLevel & 0x80) { env.sustainPoint = i; rawLevel &= 0x7F; }
                env.rates[i] = mapCZRateToSeconds(rawRate);
                env.levels[i] = mapCZLevelToNormal(rawLevel);
            }
            if (env.sustainPoint == -1) env.sustainPoint = 2;
        };

        while (offset + 256 < msgSize) // Each patch is ~256 nibbles payload? No, check sizes.
        {
            // CZ-101 Patch Nibbles: 
            // EndPoint(1) + 8*(Rate(1)+Level(1)) * 3 (DCA, DCW, Pitch) = 1 + 16*3 = 49 bytes encoded as 98 nibbles.
            // But there are two lines, so 98*2 = 196 nibbles.
            // Plus PFLAG, Detune, Vibrato... approx 128-132 bytes per patch -> 256-264 nibbles.
            // CZ-5000 Bulk dump might have multiple patches.
            
            CZ101::State::Preset preset;
            preset.name = patchName.toStdString();
            if (patchCount > 0) preset.name += " " + std::to_string(patchCount);

            // ... (rest of parsing logic)
            // I will use a size-based safety break
            int patchStartOffset = offset;
            
            uint8_t pflag = decodeNibblePair(msg, offset, msgSize);
            preset.parameters["LINE_SELECT"] = (float)(pflag & 0x03);
            
            uint8_t pds = decodeNibblePair(msg, offset, msgSize);
            uint8_t pdl = decodeNibblePair(msg, offset, msgSize);
            uint8_t pdh = decodeNibblePair(msg, offset, msgSize);
            float detune = (float)((pdl & 0x0F) + ((pdh & 0x03) * 12)) * 100.0f;
            if ((pds & 0x01)) detune = -detune;
            preset.parameters["OSC2_DETUNE"] = detune;

            uint8_t pvk = decodeNibblePair(msg, offset, msgSize);
            preset.parameters["LFO_WAVE"] = (float)(pvk & 0x03);
            decodeNibblePair(msg, offset, msgSize); 
            decodeNibblePair(msg, offset, msgSize);
            decodeNibblePair(msg, offset, msgSize);
            
            uint8_t rv1 = decodeNibblePair(msg, offset, msgSize);
            uint8_t rv2 = decodeNibblePair(msg, offset, msgSize);
            decodeNibblePair(msg, offset, msgSize);
            preset.parameters["LFO_RATE"] = mapCZRateToSeconds((rv1 & 0x0F) | ((rv2 & 0x0F) << 4)) * 10.0f;

            uint8_t dv1 = decodeNibblePair(msg, offset, msgSize);
            uint8_t dv2 = decodeNibblePair(msg, offset, msgSize);
            decodeNibblePair(msg, offset, msgSize);
            preset.parameters["LFO_DEPTH"] = mapCZDepth((dv1 & 0x0F) | ((dv2 & 0x0F) << 4));

            uint8_t mfw1 = decodeNibblePair(msg, offset, msgSize);
            uint8_t mfw1_2 = decodeNibblePair(msg, offset, msgSize);
            preset.parameters["OSC1_WAVEFORM"] = (float)(mfw1 & 0x07);
            preset.parameters["OSC1_WAVEFORM2"] = (float)(mfw1_2 & 0x07);

            decodeNibblePair(msg, offset, msgSize); decodeNibblePair(msg, offset, msgSize);
            decodeNibblePair(msg, offset, msgSize); decodeNibblePair(msg, offset, msgSize);

            decodeEnv(preset.dcaEnv, offset, msgSize);
            decodeEnv(preset.dcwEnv, offset, msgSize);
            decodeEnv(preset.pitchEnv, offset, msgSize);

            uint8_t mfw2 = decodeNibblePair(msg, offset, msgSize);
            uint8_t mfw2_2 = decodeNibblePair(msg, offset, msgSize);
            preset.parameters["OSC2_WAVEFORM"] = (float)(mfw2 & 0x07);
            preset.parameters["OSC2_WAVEFORM2"] = (float)(mfw2_2 & 0x07);

            decodeNibblePair(msg, offset, msgSize); decodeNibblePair(msg, offset, msgSize);
            decodeNibblePair(msg, offset, msgSize); decodeNibblePair(msg, offset, msgSize);

            decodeEnv(preset.dcaEnv2, offset, msgSize);
            decodeEnv(preset.dcwEnv2, offset, msgSize);
            decodeEnv(preset.pitchEnv2, offset, msgSize);

            if (onPresetParsed) onPresetParsed(preset);
            patchCount++;
            
            if (offset == patchStartOffset) break; // Infinite loop safety
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

static uint8_t mapSecondsToCZRate(float seconds) {
    // Inverse of 0.001 + (r^4 * 30.0)
    // r^4 = (seconds - 0.001) / 30.0
    if (seconds <= 0.001f) return 99;
    seconds = std::min(seconds, 30.0f);
    float r = std::pow((seconds - 0.001f) / 30.0f, 0.25f);
    int rate = 99 - (int)(r * 99.0f);
    return (uint8_t)std::clamp(rate, 0, 99);
}

static uint8_t mapNormalToCZLevel(float level) {
    return (uint8_t)(level * 99.0f);
}

juce::MemoryBlock SysExManager::createPatchDump(const CZ101::State::Preset& preset)
{
    juce::MemoryBlock data;
    data.ensureSize(264);

    // Header
    const uint8_t header[] = { 0xF0, MANUF_ID_1, MANUF_ID_2, MANUF_ID_3, DEVICE_ID_BASE, FUNC_RECV, PROG_EDIT };
    data.append(header, sizeof(header));

    // Data Body PFLAG
    uint8_t pflag = (uint8_t)preset.parameters.at("LINE_SELECT") & 0x03;
    encodeNibblePair(pflag, data);

    float detune = preset.parameters.at("OSC2_DETUNE") / 100.0f;
    uint8_t sign = (detune < 0) ? 1 : 0;
    int detuneInt = (int)std::abs(detune);
    uint8_t pdl = detuneInt % 12;
    uint8_t pdh = detuneInt / 12;
    
    encodeNibblePair(sign, data); // PDS
    encodeNibblePair(pdl, data);  // PDL
    encodeNibblePair(pdh, data);  // PDH

    // Vibrato
    uint8_t wave = (uint8_t)preset.parameters.at("LFO_WAVE");
    encodeNibblePair(wave, data); // PVK
    encodeNibblePair(0, data); // PVD (Delay)
    encodeNibblePair(0, data); 
    encodeNibblePair(0, data); 
    
    float rateSec = preset.parameters.at("LFO_RATE") / 10.0f; 
    int rateVal = mapSecondsToCZRate(rateSec);
    encodeNibblePair(rateVal & 0x0F, data); // RV1
    encodeNibblePair((rateVal >> 4) & 0x0F, data); // RV2

    encodeNibblePair(0, data); 
    
    float depth = preset.parameters.at("LFO_DEPTH");
    int depthVal = (int)(depth * 99.0f);
    encodeNibblePair(depthVal & 0x0F, data); // DV1
    encodeNibblePair((depthVal >> 4) & 0x0F, data); // DV2

    encodeNibblePair(0, data); 

    // Helper for Envelopes
    auto encodeEnv = [&](const CZ101::State::EnvelopeData& env) {
        encodeNibblePair(env.endPoint, data);
        for (int i = 0; i < 8; ++i) {
            uint8_t rate = mapSecondsToCZRate(env.rates[i]);
            encodeNibblePair(rate, data);
            uint8_t lev = mapNormalToCZLevel(env.levels[i]);
            if (i == env.sustainPoint) lev |= 0x80;
            encodeNibblePair(lev, data);
        }
    };

    // 8. Waveforms Line 1
    encodeNibblePair((uint8_t)preset.parameters.at("OSC1_WAVEFORM"), data);
    encodeNibblePair((uint8_t)preset.parameters.at("OSC1_WAVEFORM2"), data);

    // 9-10. Key Follow 
    for(int i=0; i<4; ++i) encodeNibblePair(0, data);

    // 11-16. Envelopes Line 1
    encodeEnv(preset.dcaEnv);
    encodeEnv(preset.dcwEnv);
    encodeEnv(preset.pitchEnv);
    
    // 17. Waveforms Line 2
    encodeNibblePair((uint8_t)preset.parameters.at("OSC2_WAVEFORM"), data);
    encodeNibblePair((uint8_t)preset.parameters.at("OSC2_WAVEFORM2"), data);

    // 18-19. Key Follow 2
    for(int i=0; i<4; ++i) encodeNibblePair(0, data);

    // 20-25. Envelopes Line 2
    encodeEnv(preset.dcaEnv2);
    encodeEnv(preset.dcwEnv2);
    encodeEnv(preset.pitchEnv2);

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

} // namespace MIDI
} // namespace CZ101
