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

static float mapRate(uint8_t r) { return std::min(r, (uint8_t)99) / 99.0f; }
static uint8_t unmapRate(float n) { return (uint8_t)std::clamp((int)(n * 99.0f), 0, 99); }

// --- Implementation ---

void EnvelopeSerializer::copyToSnapshot(const EnvelopeData& src, ::CZ101::Core::ParameterSnapshot::EnvParam& dst) {
    for (int i = 0; i < 8; ++i) {
        dst.rates[i] = src.rates[i];
        dst.levels[i] = src.levels[i];
    }
    dst.sustain = src.sustainPoint;
    dst.end = src.endPoint;
}

void EnvelopeSerializer::decodeFromSysEx(const uint8_t* msg, int& offset, int maxSize, EnvelopeData& env) {
    if (offset + 2 > maxSize) return;
    
    uint8_t endByte = decodeNibbles(msg, offset, maxSize);
    env.endPoint = endByte & 0x07;
    env.sustainPoint = -1;
    
    for (int i = 0; i < 8; ++i) {
        uint8_t rawRate = decodeNibbles(msg, offset, maxSize);
        uint8_t rawLevel = decodeNibbles(msg, offset, maxSize);
        
        if (rawLevel & 0x80) {
            env.sustainPoint = i;
            rawLevel &= 0x7F;
        }
        
        env.rates[i] = mapRate(rawRate);
        env.levels[i] = mapRate(rawLevel);
    }
    
    // Default fallback if no sustain found in SysEx
    if (env.sustainPoint == -1) env.sustainPoint = 2;
}

void EnvelopeSerializer::encodeToSysEx(const EnvelopeData& env, juce::MemoryBlock& data) {
    encodeNibbles((uint8_t)env.endPoint, data);
    for (int i = 0; i < 8; ++i) {
        uint8_t r = unmapRate(env.rates[i]);
        uint8_t l = unmapRate(env.levels[i]);
        if (i == env.sustainPoint) l |= 0x80;
        
        encodeNibbles(r, data);
        encodeNibbles(l, data);
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
