#pragma once

#include "PresetManager.h"
#include "../Core/AudioThreadSnapshot.h"
#include <juce_core/juce_core.h>

namespace CZ101 {
namespace State {

/**
 * EnvelopeSerializer - Centralized logic for Casio 8-stage envelope serialization.
 * Handles SysEx nibble decoding/encoding, Snapshot copying, and ADSR conversion.
 */
class EnvelopeSerializer {
public:
    // --- Snapshot Helpers ---
    
    /** Copies EnvelopeData to a ParameterSnapshot::EnvParam struct for the audio thread. */
    static void copyToSnapshot(const EnvelopeData& src, ::CZ101::Core::ParameterSnapshot::EnvParam& dst);

    // --- SysEx Helpers ---
    
    /** Decodes an 8-stage envelope from Casio SysEx nibbles. */
    static void decodeFromSysEx(const uint8_t* msg, int& offset, int maxSize, EnvelopeData& env);
    
    /** Encodes an 8-stage envelope into Casio SysEx nibbles. */
    static void encodeToSysEx(const EnvelopeData& env, juce::MemoryBlock& data);

    // --- Macro / ADSR Helpers ---
    
    /** Converts standard ADSR parameters (normalized 0-1) to an 8-stage envelope. */
    static void convertADSR(float a, float d, float s, float r, EnvelopeData& target, double sampleRate);

    /** Converts standard ADSR parameters (normalized 0-1) directly to a snapshot param. */
    static void convertADSRToSnapshot(float a, float d, float s, float r, ::CZ101::Core::ParameterSnapshot::EnvParam& target, double sampleRate);
};

} // namespace State
} // namespace CZ101
