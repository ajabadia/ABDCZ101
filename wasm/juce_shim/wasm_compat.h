/**
 * wasm_compat.h — Global compatibility header for JUCE + Emscripten.
 */
#pragma once

#ifdef __EMSCRIPTEN__

// 1. Include emscripten and standard headers globally
#include <emscripten/emscripten.h>
#include <algorithm>
#include <iterator>
#include <cmath>
#include <map>
#include <string>
#include <atomic>
#include <memory>

// Forward decl of String to avoid full juce header dependencies inside wasm_compat.h
namespace juce
{
    class String;
    
    using uint32 = unsigned int;
    using int64 = long long;
    using uint8 = unsigned char;
    using uint16 = unsigned short;
}

// Bring standard JUCE integer types into the global namespace for compilation
using juce::uint32;
using juce::int64;
using juce::uint8;
using juce::uint16;

#endif
