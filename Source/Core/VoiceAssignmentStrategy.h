#pragma once

#include "Voice.h"
#include <vector>
#include <limits>

namespace CZ101 {
namespace Core {

/**
 * @brief Strategy interface for voice stealing.
 */
class VoiceAssignmentStrategy
{
public:
    virtual ~VoiceAssignmentStrategy() = default;

    /**
     * @brief Finds the best voice to steal based on the strategy.
     * @param voices Pointer to voice array.
     * @param numVoices Total capacity of voice array.
     * @param maxActiveVoices Current polyphony limit.
     * @return Index of the voice to steal, or -1.
     */
    virtual int findVoiceToSteal(const Voice* voices, int numVoices, int maxActiveVoices) = 0;
    
    // Optional hooks
    virtual void noteOn(int voiceIndex, int midiNote) {}
    virtual void reset() {}
};

/**
 * @brief Steals the voice that was triggered longest ago.
 */
class OldestVoiceStrategy : public VoiceAssignmentStrategy
{
public:
    int findVoiceToSteal(const Voice* voices, int numVoices, int maxActiveVoices) override
    {
        int oldestIndex = -1;
        int64_t oldestTime = std::numeric_limits<int64_t>::max();
        
        // Scan only up to maxActiveVoices (assuming they are packed 0..max-1?)
        // Wait, VoiceManager does NOT pack voices. It uses index 0..maxActiveVoices as the pool?
        // VoiceManager::findFreeVoice scans 0..maxActiveVoices.
        // So we should scan 0..maxActiveVoices.
        
        int limit = std::min(numVoices, maxActiveVoices);

        for (int i = 0; i < limit; ++i)
        {
            if (voices[i].getLastNoteOnTime() < oldestTime)
            {
                oldestTime = voices[i].getLastNoteOnTime();
                oldestIndex = i;
            }
        }

        return oldestIndex != -1 ? oldestIndex : 0; 
    }
};

} // namespace Core
} // namespace CZ101
