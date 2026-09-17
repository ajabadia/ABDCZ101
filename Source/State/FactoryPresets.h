#pragma once

#include <cstdint>

namespace CZ101 {
namespace State {

static constexpr int FACTORY_PRESET_COUNT = 64;
static constexpr int SYSEX_PATCH_SIZE = 264;
extern const char* FACTORY_PRESET_NAMES[FACTORY_PRESET_COUNT];

// 4 Banks of 16 presets (each patch is SYSEX_PATCH_SIZE bytes)
extern const uint8_t FACTORY_PRESET_DATA_BANK_0[SYSEX_PATCH_SIZE * 16];
extern const uint8_t FACTORY_PRESET_DATA_BANK_1[SYSEX_PATCH_SIZE * 16];
extern const uint8_t FACTORY_PRESET_DATA_BANK_2[SYSEX_PATCH_SIZE * 16];
extern const uint8_t FACTORY_PRESET_DATA_BANK_3[SYSEX_PATCH_SIZE * 16];
extern const uint8_t FACTORY_PRESET_DATA_BANK_CZ101[SYSEX_PATCH_SIZE * 16];

// Returns the 264-byte SysEx body for a factory preset (0..63), or nullptr.
inline const uint8_t* getFactoryPresetData(int index)
{
    if (index >= 0 && index < 64)
    {
        int bank = index / 16;
        int off = (index % 16) * SYSEX_PATCH_SIZE;
        switch (bank)
        {
            case 0: return FACTORY_PRESET_DATA_BANK_CZ101 + off;
            case 1: return FACTORY_PRESET_DATA_BANK_1 + off;
            case 2: return FACTORY_PRESET_DATA_BANK_2 + off;
            case 3: return FACTORY_PRESET_DATA_BANK_3 + off;
        }
    }
    return nullptr;
}

} // namespace State
} // namespace CZ101
