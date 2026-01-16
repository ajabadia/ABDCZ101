/*
 * SysExManager.h - CZ-101 SysEx Parser Header
 */

#pragma once

//#include <JuceHeader.h>
#include <juce_core/juce_core.h>
#include <functional>
#include <string>
#include <array>
#include "../State/PresetManager.h"  // Adjusted Include

namespace CZ101 {
namespace MIDI {

/**
 * SysExManager
 * 
 * Parses CZ-101 SysEx messages according to Casio specification.
 * 
 * CZ-101 SysEx Format:
 * F0 44 00 00 70+ch 10 program [256 bytes] F7
 * 
 * Where:
 * - F0 = System Exclusive start
 * - 44 00 00 = Casio manufacturer ID
 * - 70+ch = Device ID (ch=0-15)
 * - 10 = SEND request (CZ → Host)
 * - 20 = RECEIVE request (Host → CZ)
 * - program = 0x60 for edit buffer, 0x20-0x2F for internal, 0x40-0x4F for cartridge
 * - [256 bytes] = Tone data (in NIBBLE format - half-bytes)
 * - F7 = System Exclusive end
 * 
 * The 256 bytes are transmitted as pairs of NIBBLES (4-bit half-bytes).
 * For example, byte 0x5F is transmitted as [0x0F, 0x05] (low nibble first).
 */

class SysExManager {
public:
    SysExManager() = default;
    ~SysExManager() = default;

    /**
     * Parse and handle incoming SysEx message
     * 
     * @param data Pointer to SysEx data (including F0 and F7)
     * @param size Size of SysEx data in bytes
     * @param patchName Display name for the patch
     */
    void handleSysEx(
        const void* data,
        int size,
        const juce::String& patchName);

    /**
     * Callback when preset is successfully parsed
     * Usage: manager.onPresetParsed = [this](const auto& preset) { ... };
     */
    std::function<void(const CZ101::State::Preset&)> onPresetParsed;
    
    /**
     * Decode a single SysEx patch (264 bytes including F0/F7)
     * @param data Pointer to 264 bytes of SysEx data
     * @param preset The target preset to populate
     * @return true if decoding was successful
     */
    static bool decodePatch(const uint8_t* data, CZ101::State::Preset& preset);
    
    /**
     * Create a SysEx dump (264 bytes) from a Preset.
     * @param preset The preset to encode.
     * @return MemoryBlock containing the SysEx message.
     */
    juce::MemoryBlock createPatchDump(const CZ101::State::Preset& preset);
    
    // Protection State
    void setProtectionState(bool protectedMem, bool prgEnabled) {
        memoryProtected = protectedMem;
        programChangeEnabled = prgEnabled;
    }

private:
    bool memoryProtected = true;
    bool programChangeEnabled = false;
    
    juce::MemoryBlock fragmentBuffer; // Audit Fix 4.3: Persistent buffer for fragmented SysEx

    // Helper functions are static - see .cpp for implementation

    // Constants for SysEx header validation
    static constexpr uint8_t SYSEX_START = 0xF0;
    static constexpr uint8_t SYSEX_END = 0xF7;
    static constexpr uint8_t MANUF_ID_1 = 0x44;  // Casio
    static constexpr uint8_t MANUF_ID_2 = 0x00;
    static constexpr uint8_t MANUF_ID_3 = 0x00;
    static constexpr uint8_t DEVICE_ID_BASE = 0x70;  // +channel
    static constexpr uint8_t FUNC_SEND = 0x10;       // CZ sends data
    static constexpr uint8_t FUNC_RECV = 0x20;       // Host sends data

    // Program codes
    static constexpr uint8_t PROG_EDIT = 0x60;       // Edit buffer
    static constexpr uint8_t PROG_INTERNAL_MIN = 0x20;  // Internal memory start
    static constexpr uint8_t PROG_INTERNAL_MAX = 0x2F;  // Internal memory end
    static constexpr uint8_t PROG_CART_MIN = 0x40;      // Cartridge start
    static constexpr uint8_t PROG_CART_MAX = 0x4F;      // Cartridge end

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SysExManager)
};

}  // namespace MIDI
}  // namespace CZ101
