#include "EnvelopeSerializer.h"
#include "../DSP/Envelopes/ADSRtoStage.h"
#include <algorithm>

namespace CZ101 {
namespace State {

// --- Internal Helpers ---

static uint8_t decodeNibbles(const uint8_t* msg, int& offset, int maxSize) {
    if (offset + 2 > maxSize) { offset = maxSize; return 0; }
    uint8_t low = msg[offset++] & 0x0F;
    uint8_t high = msg[offset++] & 0x0F;
    return (high << 4) | low;
}

static void encodeNibbles(uint8_t value, juce::MemoryBlock& data) {
    uint8_t low = value & 0x0F;
    uint8_t high = (value >> 4) & 0x0F;
    data.append(&low, 1);
    data.append(&high, 1);
}

static float mapLevel(uint8_t rawLevel, EnvelopeSerializer::EnvType type) {
    uint8_t val = rawLevel & 0x7F;
    if (type == EnvelopeSerializer::PITCH) {
        // Bytes 0x00-0x3F (0-63) map to Level 0-63.
        // Bytes 0x44-0x67 (68-103) map to Level 64-99.
        int level99 = 0;
        if (val <= 0x3F) level99 = val;
        else if (val >= 0x44 && val <= 0x67) level99 = val - 4;
        else if (val > 0x67) level99 = 99; // Safety
        else level99 = 63; // Inside gap, default to 63
        
        return std::clamp((float)level99 / 99.0f, 0.0f, 1.0f);
    } else if (type == EnvelopeSerializer::DCW) {
        if (val == 0) return 0.0f;
        if (val >= 127) return 1.0f;
        return std::clamp((float)val / 127.0f, 0.0f, 1.0f);
    } else { // DCA
        if (val == 0) return 0.0f;
        // Hardware skips 1-27. Audible range is 28-127. 
        // 28->1 (mapped to >0.0), 127->99 (mapped to 1.0)
        float level99 = (99.0f * (float)(val - 28) / 99.0f); 
        if (level99 < 1.0f) level99 = 1.0f;
        return std::clamp(level99 / 99.0f, 0.0f, 1.0f);
    }
}

static uint8_t unmapLevel(float normLevel, EnvelopeSerializer::EnvType type) {
    if (type == EnvelopeSerializer::PITCH) {
        int level99 = (int)std::round(normLevel * 99.0f);
        if (level99 <= 63) return (uint8_t)level99;
        return (uint8_t)std::clamp(level99 + 4, 0x44, 0x67);
    } else if (type == EnvelopeSerializer::DCW) {
        return (uint8_t)std::clamp((int)std::round(normLevel * 127.0f), 0, 127);
    } else { // DCA
        if (normLevel <= 0.001f) return 0;
        // Level 1-99 maps to 28-127
        int level99 = (int)std::round(normLevel * 99.0f);
        if (level99 == 0) return 0;
        return (uint8_t)std::clamp(28 + level99, 0, 127);
    }
}

// Casio SYSEX encodes each envelope type's rate with a different formula
// (see the CZ MIDI/SYSEX spec, sections PMA/PMW/PMP):
//   DCA  (a): byte = 119*r/99          ; r = 99*byte/119 + 1
//   DCW  (w): byte = 119*level/99 + 8  ; level = 99*(byte-8)/119 + 1
//   DCO  (o): byte = 127*r/99          ; r = 99*byte/127 + 1
// (except byte 0 -> rate 0, byte 127 -> rate 99).
static float mapRate(uint8_t rawRate, EnvelopeSerializer::EnvType type) {
    uint8_t byte = rawRate & 0x7F;
    float rate = 0.0f;
    if (type == EnvelopeSerializer::DCA) {
        rate = (99.0f * (float)byte / 119.0f) + 1.0f;
    } else if (type == EnvelopeSerializer::DCW) {
        if (byte < 8) byte = 8;
        rate = (99.0f * (float)(byte - 8) / 119.0f) + 1.0f;
    } else { // PITCH (DCO)
        rate = (99.0f * (float)byte / 127.0f) + 1.0f;
    }
    return std::clamp(rate / 99.0f, 0.0f, 1.0f);
}

static uint8_t unmapRate(float normRate, EnvelopeSerializer::EnvType type) {
    float rate = normRate * 99.0f;
    int byte = 0;
    if (type == EnvelopeSerializer::DCA) {
        byte = (int)std::round((rate - 1.0f) * 119.0f / 99.0f);
    } else if (type == EnvelopeSerializer::DCW) {
        byte = (int)std::round((rate - 1.0f) * 119.0f / 99.0f) + 8;
    } else { // PITCH
        byte = (int)std::round((rate - 1.0f) * 127.0f / 99.0f);
    }
    return (uint8_t)std::clamp(byte, 0, 127);
}

// --- Implementation ---

void EnvelopeSerializer::copyToSnapshot(const EnvelopeData& src, ::CZ101::Core::ParameterSnapshot::EnvParam& dst) {
    for (int i = 0; i < 8; ++i) {
        dst.rates[i] = src.rates[i];
        dst.levels[i] = src.levels[i];
    }
    dst.sustain = src.sustainPoint;
    dst.end = src.endPoint;
}

void EnvelopeSerializer::decodeFromSysEx(const uint8_t* msg, int& offset, int maxSize, EnvelopeData& env, EnvType type, float& velocityOut) {
    if (offset + 2 > maxSize) return;
    
    uint8_t endByte = decodeNibbles(msg, offset, maxSize);
    env.endPoint = endByte & 0x07;
    env.sustainPoint = -1;
    
    // Extract CZ-1 velocity sensitivity
    uint8_t d = (endByte >> 4) & 0x0F;
    velocityOut = static_cast<float>(15 - d); // AMP = 15 - d
    
    
    for (int i = 0; i < 8; ++i) {
        uint8_t rawRate = decodeNibbles(msg, offset, maxSize);
        uint8_t rawLevel = decodeNibbles(msg, offset, maxSize);
        
        if (rawLevel & 0x80) {
            env.sustainPoint = i;
            rawLevel &= 0x7F;
        }
        
        env.rates[i] = mapRate(rawRate, type);
        env.levels[i] = mapLevel(rawLevel, type);
    }
}

void EnvelopeSerializer::encodeToSysEx(const EnvelopeData& env, juce::MemoryBlock& data, EnvType type, float velocityIn) {
    uint8_t endStep = (uint8_t)(env.endPoint & 0x07);
    uint8_t amp = (uint8_t)std::clamp((int)velocityIn, 0, 15);
    uint8_t d = 15 - amp;
    uint8_t endByte = (d << 4) | endStep;
    
    encodeNibbles(endByte, data);
    float prevLevel = 0.0f;
    for (int i = 0; i < 8; ++i) {
        uint8_t r = unmapRate(env.rates[i], type);
        uint8_t l = unmapLevel(env.levels[i], type);
        
        if (prevLevel > env.levels[i]) {
            r |= 0x80;
        }
        if (i == env.sustainPoint) l |= 0x80;
        
        encodeNibbles(r, data);
        encodeNibbles(l, data);
        
        prevLevel = env.levels[i];
    }
}

void EnvelopeSerializer::convertADSR(float a, float d, float s, float r, EnvelopeData& target, double sampleRate) {
    std::array<float, 8> rr, ll;
    int sus, end;
    float msMult = 1000.0f;
    ::CZ101::DSP::ADSRtoStageConverter::convertADSR(a * msMult, d * msMult, s, r * msMult, rr, ll, sus, end, sampleRate);
    
    for (int i = 0; i < 8; ++i) {
        target.rates[i] = rr[i];
        target.levels[i] = ll[i];
    }
    target.sustainPoint = sus;
    target.endPoint = end;
}

void EnvelopeSerializer::convertADSRToSnapshot(float a, float d, float s, float r, ::CZ101::Core::ParameterSnapshot::EnvParam& target, double sampleRate) {
    std::array<float, 8> rr, ll;
    int sus, end;
    float msMult = 1000.0f;
    ::CZ101::DSP::ADSRtoStageConverter::convertADSR(a * msMult, d * msMult, s, r * msMult, rr, ll, sus, end, sampleRate);
    
    for (int i = 0; i < 8; ++i) {
        target.rates[i] = rr[i];
        target.levels[i] = ll[i];
    }
    target.sustain = sus;
    target.end = end;
}

} // namespace State
} // namespace CZ101
