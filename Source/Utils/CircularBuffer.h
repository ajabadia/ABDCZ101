#pragma once

// Copyright (C) 2021 ICST - University of Applied Sciences Zurich
// Licensed under MIT

#include <vector>
#include <cmath>
#include <algorithm>
#include <juce_core/juce_core.h>

namespace CZ101 {
namespace Utils {

template <typename T>
class CircularBuffer {
public:
    CircularBuffer() = default;
    
    void setSize(int newSize) {
        buffer.resize(newSize, 0);
        writeIndex = 0;
        size = newSize;
    }
    
    void clear() {
        std::fill(buffer.begin(), buffer.end(), T(0));
        writeIndex = 0;
    }
    
    void push(T sample) {
        buffer[writeIndex] = sample;
        writeIndex++;
        if (writeIndex >= size) writeIndex = 0;
    }
    
    T get(int delaySamples) const {
        int readIndex = writeIndex - 1 - delaySamples;
        while (readIndex < 0) readIndex += size;
        while (readIndex >= size) readIndex -= size;
        return buffer[readIndex];
    }
    
    // Linear Interpolation
    T getInterpolated(float delaySamples) const {
        int i = (int)delaySamples;
        float f = delaySamples - i;
        
        T s1 = get(i);
        T s2 = get(i + 1);
        
        return s1 + (s2 - s1) * f;
    }
    
private:
    std::vector<T> buffer;
    int writeIndex = 0;
    int size = 0;
};

} // namespace Utils
} // namespace CZ101
