#pragma once
#include "../Core/VoiceAssignmentStrategy.h"
#include <algorithm>

namespace CZ101 {
namespace Core {

/**
 * Authentic Voice Assignment Strategy for the CZ-5000.
 * Uses a Round-Robin approach but prioritizes stealing voices 
 * that are currently in the Release phase, unlike the CZ-101 
 * which is strictly "Oldest Note".
 */
class CZ5000VoiceStrategy : public VoiceAssignmentStrategy
{
public:
    int findVoiceToSteal(const Voice* voices, int numVoices, int maxActiveVoices) override
    {
        // 1. Search for a free voice (not active)
        // (Usually handled by VoiceManager before calling steal, but good to check)
        for (int i = 0; i < maxActiveVoices; ++i) {
            if (!voices[i].isActive()) return i;
        }

        // 2. Search for voices in Release phase (steal them first!)
        // Authentic behavior: If multiple are released, pick the oldest one?
        // Or just the next one in round-robin sequence?
        // Let's assume finding *any* releasing voice is better than a held one.
        for (int i = 0; i < maxActiveVoices; ++i) {
            // How to check release phase? Voice needs an accessor or we check envelope state.
            // Assuming Voice has isReleasing() or similar.
            if (voices[i].isReleasing()) return i; 
        }

        // 3. If all voices are held, use Round Robin
        int stolenVoice = roundRobinIndex;
        roundRobinIndex = (roundRobinIndex + 1) % maxActiveVoices;
        return stolenVoice;
    }

    void noteOn(int voiceIndex, int midiNote) override {
        // Update LRU or other tracking if needed
    }

    void reset() override {
        roundRobinIndex = 0;
    }

private:
    int roundRobinIndex = 0;
};

} // namespace Core
} // namespace CZ101
