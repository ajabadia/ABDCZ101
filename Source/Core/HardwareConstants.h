#pragma once

#include <cmath>
#include <algorithm>
#include <cstdlib>

namespace CZ101 {
namespace Core {
namespace HardwareConstants {

    // --- CZ-101 Hardware Specifics ---
    
    // DCW Key Follow scaling factor (Heuristic based on handbook p.47)
    constexpr float KEYTRACK_DCW_FACTOR = 0.015f; 
    
    // DCA Key Follow Offset (Simulates slightly shorter decay on high notes)
    constexpr float KEYTRACK_DCA_OFFSET = -0.002f;

    // Output Soft Clip Curve (Derived from scope measurement)
    constexpr float SOFT_CLIP_DRIVE = 0.68f;

    // Master Headroom Scaling (To prevent digital clipping after mix)
    constexpr float MASTER_HEADROOM_GAIN = 0.9f;

    // --- DSP Thresholds ---
    
    // Minimum value to treat as effectively zero for optimization
    constexpr float EPSILON_AUDIBLE = 0.001f;
    
    // Denormal protection threshold for Tanh
    constexpr float TANH_DENORMAL_THRESHOLD = 1e-9f;
    
    // Tanh denominator protection
    constexpr float TANH_DENOM_PROTECT = 1e-20f;
    
    // Control Rate Divider (Process control every 8 samples)
    constexpr int CONTROL_RATE_DIVIDER = 8;
    constexpr int CONTROL_RATE_MASK = CONTROL_RATE_DIVIDER - 1;

    // Audit Fix 10.3: Non-Linear Line Mixing (Simulate summing amp saturation)
    // Uses tanh approximation for warmth and safety.
    inline float mixLines(float l1, float l2) {
        float sum = l1 + l2;
        // Soft saturation: std::tanh(sum)
        // We allow a bit of "hotness"
        return std::tanh(sum); 
    }

} // namespace HardwareConstants
} // namespace Core
} // namespace CZ101
