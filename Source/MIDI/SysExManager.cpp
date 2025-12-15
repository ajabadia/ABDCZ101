#include "SysExManager.h"
#include <juce_core/juce_core.h>
#include <iostream>

namespace CZ101 {
namespace MIDI {

SysExManager::SysExManager(State::PresetManager& pm)
    : presetManager(pm)
{
}

void SysExManager::handleSysEx(const void* data, int size)
{
    const juce::uint8* bytes = static_cast<const juce::uint8*>(data);
    
    // Basic Validation
    if (size < 6 || bytes[0] != SYSEX_START || bytes[size-1] != SYSEX_END) {
        std::cout << "[SysEx] Invalid framing or size: " << size << std::endl;
        return;
    }
        
    // Check Manufacturer (Casio = 0x44)
    if (bytes[1] != MANU_ID_CASIO) {
        std::cout << "[SysEx] Wrong Manufacturer: " << (int)bytes[1] << std::endl;
        return;
    }
        
    // CZ-101 Dump Header usually: F0 44 00 00 70+ch ...
    // Let's look for the "Receive Request" pattern or "Dump" pattern.
    // Doc: "Receive request: F0 44 00 00 70+ch 20 program ... F7"
    // But actual Data Dump is the response to a "Send Request".
    // "Send request... CZ101: F0 44 00 00 70+ch 30" -> Ack.
    // The big data block usually comes after a handshake.
    // However, .syx files usually contain the FULL sequence or the DATA block.
    // A Sound Data Reference usually looks like:
    // F0 44 00 00 7n 31 ... data ... sum F7
    
    // For now, let's just look for the data payload signature if possible.
    // If bytes[5] == 0x31 (Data follows?) or we just try to parse if valid.
    
    // Let's try to parse if it looks like a parameter dump.
    // Dump length is huge (256 bytes payload meaning 512 nibbles?).
    // 25 distinct sections.
    
    // Simplified: process payload if it matches Casio format.
    // Offset 6 is usually where data starts?
    int payloadStart = 6; 
    int payloadEnd = size - 2; // exclude checksum and F7?
    
    if (payloadEnd - payloadStart > 100) // Arbitrary check for "big enough"
    {
        parseToneData(bytes + payloadStart, payloadEnd - payloadStart);
    }
}

juce::MidiMessage SysExManager::createDumpMessage(const State::Preset& preset)
{
    // TODO: Implement encoding
    juce::ignoreUnused(preset);
    return juce::MidiMessage::createSysExMessage(nullptr, 0); 
}

// ⚠️ CRITICAL: Casio uses little-endian nibble pairs
// Dato hexadecimal 5F se transmite como 0F 05 (nibbles invertidos y separados)
// Handbook: "Dato hexadecimal 5F se transmite como 0F 05"
// So: Byte 1 = Low Nibble (0F), Byte 2 = High Nibble (05)
juce::uint8 SysExManager::decodeByte(juce::uint8 lowNibble, juce::uint8 highNibble) const
{
    return (lowNibble & 0x0F) | ((highNibble & 0x0F) << 4);
}

float SysExManager::mapRangeTo01(int value, int min, int max)
{
    if (max == min) return 0.0f;
    return static_cast<float>(value - min) / static_cast<float>(max - min);
}

float SysExManager::mapRateToSeconds(int rate)
{
    // CZ Rate 0-99. 99 is fast (~1ms), 0 is slow (~3s?).
    // Invert: 99 -> 0.0, 0 -> 1.0 logic?
    // Our map: 0..1 to Seconds.
    return 1.0f - (rate / 99.0f);
}

void SysExManager::parseToneData(const juce::uint8* payload, int payloadSize)
{
    // Need approx 256 bytes payload (256 nibbles = 128 bytes internal data)
    // File is 264 bytes total -> ~256 data.
    std::cout << "[SysEx] Payload Size: " << payloadSize << std::endl;
    if (payloadSize < 240) {
        std::cout << "[SysEx] Error: Payload too small (" << payloadSize << " < 240)" << std::endl;
        return;
    } 
    
    State::Preset p = presetManager.getCurrentPreset(); 
    p.name = "SysEx Import";
    
    int currentByte = 0;
    auto getNextByte = [&]() -> juce::uint8 {
        if (currentByte + 1 >= payloadSize) return 0;
        juce::uint8 v = decodeByte(payload[currentByte], payload[currentByte+1]);
        currentByte += 2;
        return v;
    };
    
    // --- 1. PFLAG (1 byte) ---
    juce::uint8 pflag = getNextByte();
    int octave = (pflag >> 1) & 0x03; // Bits 1-2
    int lineSel = pflag & 0x01;       // Bit 0? Handbook says "Line Select: bits 0-1"? 
    juce::ignoreUnused(octave, lineSel);
    
    // --- 2. PDS (1 byte) - Detune Sign ---
    juce::uint8 pds = getNextByte();
    juce::ignoreUnused(pds);
    
    // --- 3. PDL, PDH (2 bytes) - Detune Range ---
    juce::uint8 pdl = getNextByte();
    juce::uint8 pdh = getNextByte();
    int detuneVal = pdl | (pdh << 8);
    juce::ignoreUnused(detuneVal);
    // TODO: Map detune
    
    // --- 4. PVK (1 byte) - Vibrato Wave ---
    juce::uint8 pvk = getNextByte();
    juce::ignoreUnused(pvk);
    // 0x08=Sine, 0x04=Tri, 0x20=Saw, 0x02=Square
    // We only have Sine implemented fully, currently?
    
    // --- 5. PVDL (3 bytes) - Vibrato Delay ---
    getNextByte(); getNextByte(); getNextByte(); 
    
    // --- 6. PVS (3 bytes) - Vibrato Rate ---
    getNextByte(); getNextByte(); getNextByte();
    
    // --- 7. PVD (3 bytes) - Vibrato Depth ---
    juce::uint8 pvd1 = getNextByte();
    juce::uint8 pvd2 = getNextByte(); 
    juce::uint8 pvd3 = getNextByte();
    juce::ignoreUnused(pvd1, pvd2, pvd3);
    // Usually only one byte matters for basic emulation (depth).
    // mapRangeTo01(pvd3, 0, 99) -> Vibrato Depth?
    
    // ... Skipping to Section 12 (DCA1 Env) ...
    // Sections 8-11: MFW, MAMD, MWMD, PMAL
    getNextByte(); getNextByte(); // MFW (Wave)
    getNextByte(); getNextByte(); // MAMD
    getNextByte(); getNextByte(); // MWMD
    juce::uint8 pmal = getNextByte(); // DCA1 End Step
    
    auto parseEnvelope = [&](State::EnvelopeData& env, int endStep) {
        env.endPoint = (endStep > 7) ? 7 : endStep;
        
        for (int i=0; i<8; ++i) {
            juce::uint8 rawRate = getNextByte();
            juce::uint8 rawLevel = getNextByte();
            
            // Handbook: Parameters are 0-99.
            // Mapping 0-127 was incorrect and caused slow rates (99 -> 0.78).
            // We must map 0-99 to 0.0-1.0.
            
            int rateVal = (rawRate > 99) ? 99 : rawRate;
            int levelVal = (rawLevel & 0x7F);
            if (levelVal > 99) levelVal = 99;
            
            env.rates[i] = mapRangeTo01(rateVal, 0, 99); 
            env.levels[i] = mapRangeTo01(levelVal, 0, 99);
            
            if (rawLevel & 0x80) env.sustainPoint = i;
        }
    };
    
    // 12: DCA1 Env
    State::EnvelopeData dca1; 
    parseEnvelope(dca1, pmal);
    p.dcaEnv = dca1; // Only mapping to Voice 1 mapping for now
    
    // 13: PMWL (DCW1 End)
    juce::uint8 pmwl = getNextByte();
    
    // 14: DCW1 Env
    State::EnvelopeData dcw1;
    parseEnvelope(dcw1, pmwl);
    p.dcwEnv = dcw1;
    
    // 15: PMPL (DCO1 End)
    juce::uint8 pmpl = getNextByte();
    
    // 16: DCO1 Env
    State::EnvelopeData dco1;
    parseEnvelope(dco1, pmpl);
    p.pitchEnv = dco1;
    
    // Skip Line 2 for now to keep it simple (monotimbral preset structure assumption)
    // Or we could map it if Preset struct supported dual lines.
    
    // Apply
    presetManager.loadPresetFromStruct(p);
}

} // namespace MIDI
} // namespace CZ101
