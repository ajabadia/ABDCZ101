#pragma once

#include "../PresetManager.h"

namespace CZ101 {
namespace State {

// Initialize all envelope stages to a default state (shared by factory preset builders)
inline void initEnvelopes(Preset& p)
{
    // Initialize all envelope stages to a default state
    for(int i=0; i<8; ++i) {
        p.dcwEnv.rates[i] = 0.5f; p.dcwEnv.levels[i] = 1.0f; // Timbre open
        p.dcaEnv.rates[i] = 0.5f; p.dcaEnv.levels[i] = 1.0f; // Volume up
        p.pitchEnv.rates[i] = 0.5f; p.pitchEnv.levels[i] = 0.5f; // Pitch center
    }
    p.dcwEnv.sustainPoint = 2; p.dcwEnv.endPoint = 3;
    p.dcaEnv.sustainPoint = 2; p.dcaEnv.endPoint = 3;
    p.pitchEnv.sustainPoint = 2; p.pitchEnv.endPoint = 3;
}

} // namespace State
} // namespace CZ101
