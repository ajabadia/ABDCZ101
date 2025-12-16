/*
 * SysExManager.cpp - CZ-101 SysEx Parser (CORRECTO)
 * 
 * Parsea correctamente el formato SysEx del CZ-101:
 * - 256 bytes de data en formato NIBBLE (half-byte)
 * - Estructura de 25 secciones según especificación Casio
 * - Decodifica vibrato, detune, waveforms y envelopes
 */

#include "SysExManager.h"
//#include <JuceHeader.h>
#include <juce_core/juce_core.h>
#include <cmath>
#include <array>
#include <cstdint>

using std::uint8_t;

namespace CZ101 {
namespace MIDI {

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Decodifica un par de NIBBLES (half-bytes) del payload CZ-101
 * El CZ-101 envía cada byte como DOS nibbles: bajo, alto
 * Ejemplo: Byte 0x5F se transmite como [0x0F, 0x05]
 */
static uint8_t decodeNibblePair(const uint8_t* payload, int& offset, int maxSize) {
    if (offset + 1 >= maxSize) {
        juce::Logger::writeToLog("⚠️ SysEx: Offset overflow at offset=" + juce::String(offset));
        return 0;
    }
    uint8_t lowNibble = payload[offset++] & 0x0F;
    uint8_t highNibble = payload[offset++] & 0x0F;
    uint8_t result = (highNibble << 4) | lowNibble;
    return result;
}

/**
 * Mapea CZ-101 Rate (0-99) a segundos
 * Rate 0 = lento (~3 segundos)
 * Rate 99 = rápido (~1 milisegundo)
 */
static float mapCZRateToSeconds(uint8_t rate) {
    rate = std::min(rate, static_cast<uint8_t>(99));
    // Usar escala logarítmica: min=0.001s, max=3.0s
    float normalized = (99.0f - static_cast<float>(rate)) / 99.0f;
    return 0.001f * std::pow(3000.0f, normalized);
}

/**
 * Mapea CZ-101 Level (0-99) a rango 0.0-1.0
 */
static float mapCZLevelToNormal(uint8_t level) {
    level = std::min(level, static_cast<uint8_t>(99));
    return static_cast<float>(level) / 99.0f;
}

/**
 * Mapea CZ-101 Depth para vibrato (0-99 a semitones o porcentaje)
 */
static float mapCZDepth(uint8_t depthVal) {
    depthVal = std::min(depthVal, static_cast<uint8_t>(99));
    return static_cast<float>(depthVal) / 99.0f;
}

// ============================================================================
// MAIN SYSEX HANDLER
// ============================================================================

void SysExManager::handleSysEx(
    const void* data,
    int size,
    const juce::String& patchName)
{
    const auto bytes = static_cast<const uint8_t*>(data);

    // ========== VALIDACIÓN DE HEADER ==========
    if (size < 70) {
        juce::Logger::writeToLog("❌ SysEx too small: " + juce::String(size) + " bytes");
        return;
    }

    // Validar estructura mínima: F0 44 00 00 70+ch 10/20 program <data> F7
    // Típicamente: F0 44 00 00 70 10 [payload] F7
    if (bytes[0] != 0xF0) {
        juce::Logger::writeToLog("❌ Invalid SysEx start");
        return;
    }

    // Check Casio ID (44 00 00)
    if (bytes[1] != 0x44 || bytes[2] != 0x00 || bytes[3] != 0x00) {
        juce::Logger::writeToLog("❌ Not a Casio SysEx");
        return;
    }

    // Check device ID (70+channel, typically 70 for ch0)
    if ((bytes[4] & 0xF0) != 0x70) {
        juce::Logger::writeToLog("❌ Invalid device ID");
        return;
    }

    // Check function code (10=SEND, 20=RECEIVE)
    uint8_t function = bytes[5];
    if (function != 0x10 && function != 0x20) {
        juce::Logger::writeToLog("⚠️ Unknown function code: " + juce::String::toHexString(function));
        // Continue anyway - some CZs might use 0x30
    }

    // Program/bank indicator (typically bytes[6])
    uint8_t programCode = bytes[6];

    // Payload comienza en offset 7 (después de header)
    int payloadOffset = 7;
    int payloadSize = size - 8;  // Excluir F0...70,function,program y F7

    // Validación: El CZ-101 envía exactamente 256 bytes de data en NIBBLES
    // = 512 nibbles cuando se transmiten
    // Pero algunos editores pueden pre-decodificar, así que aceptamos 256-512
    if (payloadSize < 256) {
        juce::Logger::writeToLog("⚠️ SysEx payload small: " + juce::String(payloadSize) + 
                                " bytes (expected 256-512)");
    }

    juce::Logger::writeToLog("📥 Parsing CZ-101 SysEx: " + juce::String(payloadSize) + 
                            " bytes, program=" + juce::String::toHexString(programCode));

    CZ101::State::Preset preset;
    preset.name = patchName.toStdString();

    int nibbleOffset = payloadOffset;

    // ========== SECTION 1: PFLAG (Line Select + Octave Range) ==========
    uint8_t pflag = decodeNibblePair(bytes, nibbleOffset, size);
    int lineSelect = pflag & 0x03;           // Bits 0-1: 00=1, 01=2, 10=1+1, 11=1+2
    int octaveRange = (pflag >> 2) & 0x03;   // Bits 2-3: 00=0, 01=+1, 10=-1

    preset.parameters["lineselect"] = static_cast<float>(lineSelect);
    preset.parameters["octaverange"] = static_cast<float>(octaveRange);


    juce::Logger::writeToLog("  PFLAG=0x" + juce::String::toHexString(pflag) +
                            " LineSelect=" + juce::String(lineSelect) +
                            " Octave=" + juce::String(octaveRange));

    // ========== SECTION 2: PDS (Detune Sign) ==========
    uint8_t pds = decodeNibblePair(bytes, nibbleOffset, size);
    bool detunePlus = (pds & 0x01) == 0;  // 0=+, 1=-

    // ========== SECTION 3: PDL, PDH (Detune Range: FINE + OCTAVE/NOTE) ==========
    uint8_t pdFine = decodeNibblePair(bytes, nibbleOffset, size);    // Fine: 0-15
    uint8_t pdNote = decodeNibblePair(bytes, nibbleOffset, size);    // Octave (0-3) + Note (0-11)

    int detuneOctave = (pdNote >> 4) & 0x03;
    int detuneNote = pdNote & 0x0F;
    float detuneCents = (pdFine + detuneOctave * 12) * 100.0f;
    // detuneNote is not currently used in our mapping logic, but decoded for completeness
    (void)detuneNote;
    if (!detunePlus) detuneCents = -detuneCents;

    preset.parameters["osc2detune"] = detuneCents;
    juce::Logger::writeToLog("  Detune: " + juce::String(detuneCents, 1) + " cents");

    // ========== SECTION 4: PVK (Vibrato Wave) ==========
    uint8_t pvk = decodeNibblePair(bytes, nibbleOffset, size);
    int vibratoWave = pvk & 0x07;  // 0=sine, 1=tri, 2=saw, 3=square, etc.
    preset.parameters["lfowaveform"] = static_cast<float>(vibratoWave);

    // ========== SECTION 5: PVD (Vibrato DELAY) - 3 bytes encoded ==========
    uint8_t pvdld = decodeNibblePair(bytes, nibbleOffset, size);
    uint8_t pvdlv = decodeNibblePair(bytes, nibbleOffset, size);
    uint8_t pvd3  = decodeNibblePair(bytes, nibbleOffset, size); (void)pvd3;

    // Mapeo de delay: combinar bytes encoded
    // Fix: Raw value is likely in ms or small units. 1983 raw was becoming 19830ms (20s).
    // Reduced factor to 1.0f to match typical delay times (e.g. 2s).
    float delayMs = ((pvdld & 0x0F) * 256 + pvdlv) * 1.0f; 
    if (delayMs > 30000.0f) delayMs = 30000.0f;  // Cap a 30 segundos
    preset.parameters["vibratodelay"] = delayMs / 1000.0f;  // Convert to seconds

    juce::Logger::writeToLog("  Vibrato Delay: " + juce::String(delayMs, 0) + " ms");

    // ========== SECTION 6: PVS (Vibrato RATE) - 3 bytes encoded ==========
    uint8_t pvsd = decodeNibblePair(bytes, nibbleOffset, size);
    uint8_t pvsv = decodeNibblePair(bytes, nibbleOffset, size);
    uint8_t pvs3 = decodeNibblePair(bytes, nibbleOffset, size); (void)pvs3;

    uint8_t rateVal = (pvsd & 0x0F) | ((pvsv & 0x0F) << 4);
    float lfoRate = mapCZRateToSeconds(rateVal);
    preset.parameters["lforate"] = lfoRate;

    juce::Logger::writeToLog("  Vibrato Rate: " + juce::String(lfoRate, 3) + " Hz");

    // ========== SECTION 7: PVD (Vibrato DEPTH) - 3 bytes encoded ==========
    uint8_t pvdd = decodeNibblePair(bytes, nibbleOffset, size);
    uint8_t pvdv = decodeNibblePair(bytes, nibbleOffset, size);
    uint8_t pvd7_3 = decodeNibblePair(bytes, nibbleOffset, size); (void)pvd7_3;

    uint8_t depthVal = (pvdd & 0x0F) | ((pvdv & 0x0F) << 4);
    float lfoDepth = mapCZDepth(depthVal);
    preset.parameters["lfodepth"] = lfoDepth;

    juce::Logger::writeToLog("  Vibrato Depth: " + juce::String(lfoDepth, 2));

    // ========== SECTION 8: MFW (DCO1 Waveform) - 2 bytes nibbles ==========
    uint8_t mfw1 = decodeNibblePair(bytes, nibbleOffset, size);
    uint8_t mfw2 = decodeNibblePair(bytes, nibbleOffset, size);

    int osc1Wave = mfw1 & 0x07;
    int osc2Wave = mfw2 & 0x07;
    preset.parameters["osc1waveform"] = static_cast<float>(osc1Wave);
    preset.parameters["osc2waveform"] = static_cast<float>(osc2Wave);

    juce::Logger::writeToLog("  Oscillators: Wave1=" + juce::String(osc1Wave) +
                            " Wave2=" + juce::String(osc2Wave));

    // ========== SECTION 9: MAMD, MAMV (DCA1 Key Follow) ==========
    uint8_t mamd = decodeNibblePair(bytes, nibbleOffset, size); (void)mamd;
    uint8_t mamv = decodeNibblePair(bytes, nibbleOffset, size);
    // Key follow 0-9 typically, store if needed
    preset.parameters["dca1keyfollow"] = static_cast<float>(mamv & 0x0F);

    // ========== SECTION 10: MWMD, MWMV (DCW1 Key Follow) ==========
    uint8_t mwmd = decodeNibblePair(bytes, nibbleOffset, size); (void)mwmd;
    uint8_t mwmv = decodeNibblePair(bytes, nibbleOffset, size);
    preset.parameters["dcw1keyfollow"] = static_cast<float>(mwmv & 0x0F);

    // ========== SECTION 11: PMAL (DCA1 End Step) ==========
    uint8_t pmal = decodeNibblePair(bytes, nibbleOffset, size);
    int dcaEndPoint = pmal & 0x07;  // 0-7 (8 stages)

    // ========== SECTION 12: PMA (DCA1 Envelope Rates/Levels - 16 bytes = 8 stages) ==========
    std::array<float, 8> dcaRates = {};
    std::array<float, 8> dcaLevels = {};
    int dcaSustainPoint = -1;

    for (int i = 0; i < 8; i++) {
        uint8_t rawRate = decodeNibblePair(bytes, nibbleOffset, size);
        uint8_t rawLevel = decodeNibblePair(bytes, nibbleOffset, size);

        // Bit 0x80 en level = sustain point
        if (rawLevel & 0x80) {
            dcaSustainPoint = i;
            rawLevel &= 0x7F;  // Clear sustain bit
        }

        dcaRates[i] = mapCZRateToSeconds(rawRate);
        dcaLevels[i] = mapCZLevelToNormal(rawLevel);
    }

    if (dcaSustainPoint == -1) dcaSustainPoint = 2;  // Default

    // ========== SECTION 13: PMWL (DCW1 End Step) ==========
    uint8_t pmwl = decodeNibblePair(bytes, nibbleOffset, size);
    int dcwEndPoint = pmwl & 0x07;

    // ========== SECTION 14: PMW (DCW1 Envelope Rates/Levels - 16 bytes) ==========
    std::array<float, 8> dcwRates = {};
    std::array<float, 8> dcwLevels = {};
    int dcwSustainPoint = -1;

    for (int i = 0; i < 8; i++) {
        uint8_t rawRate = decodeNibblePair(bytes, nibbleOffset, size);
        uint8_t rawLevel = decodeNibblePair(bytes, nibbleOffset, size);

        if (rawLevel & 0x80) {
            dcwSustainPoint = i;
            rawLevel &= 0x7F;
        }

        dcwRates[i] = mapCZRateToSeconds(rawRate);
        dcwLevels[i] = mapCZLevelToNormal(rawLevel);
    }

    if (dcwSustainPoint == -1) dcwSustainPoint = 2;

    // ========== SECTION 15: PMPL (DCO1 End Step) ==========
    uint8_t pmpl = decodeNibblePair(bytes, nibbleOffset, size);
    int pitchEndPoint = pmpl & 0x07;

    // ========== SECTION 16: PMP (Pitch Envelope Rates/Levels - 16 bytes) ==========
    std::array<float, 8> pitchRates = {};
    std::array<float, 8> pitchLevels = {};
    int pitchSustainPoint = -1;

    for (int i = 0; i < 8; i++) {
        uint8_t rawRate = decodeNibblePair(bytes, nibbleOffset, size);
        uint8_t rawLevel = decodeNibblePair(bytes, nibbleOffset, size);

        if (rawLevel & 0x80) {
            pitchSustainPoint = i;
            rawLevel &= 0x7F;
        }

        pitchRates[i] = mapCZRateToSeconds(rawRate);
        pitchLevels[i] = mapCZLevelToNormal(rawLevel);
    }

    if (pitchSustainPoint == -1) pitchSustainPoint = 2;

    // ========== SECTIONS 17-25: DCO2, DCW2, DCA2 Envelopes (Similar) ==========
    // Decodificar pero no usar (para mantener offset correcto)
    for (int i = 0; i < 2 + 2 + 2 + 1 + 16 + 1 + 16 + 1 + 16; i++) {
        uint8_t skip = decodeNibblePair(bytes, nibbleOffset, size);
        (void)skip;
    }

    // ========== STORE ENVELOPES IN PRESET ==========

    // DCA Envelope
    preset.dcaEnv.sustainPoint = dcaSustainPoint;
    preset.dcaEnv.endPoint = dcaEndPoint;
    for (int i = 0; i < 8; i++) {
        preset.dcaEnv.rates[i] = dcaRates[i];
        preset.dcaEnv.levels[i] = dcaLevels[i];
    }

    // DCW Envelope
    preset.dcwEnv.sustainPoint = dcwSustainPoint;
    preset.dcwEnv.endPoint = dcwEndPoint;
    for (int i = 0; i < 8; i++) {
        preset.dcwEnv.rates[i] = dcwRates[i];
        preset.dcwEnv.levels[i] = dcwLevels[i];
    }

    // Pitch Envelope
    preset.pitchEnv.sustainPoint = pitchSustainPoint;
    preset.pitchEnv.endPoint = pitchEndPoint;
    for (int i = 0; i < 8; i++) {
        preset.pitchEnv.rates[i] = pitchRates[i];
        preset.pitchEnv.levels[i] = pitchLevels[i];
    }

    juce::Logger::writeToLog("✅ SysEx parsed successfully: " + patchName);

    // ========== CALLBACK ==========
    if (onPresetParsed) {
        onPresetParsed(preset);
    }
}

}  // namespace MIDI
}  // namespace CZ101
