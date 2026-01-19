#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <algorithm>
#include <set>
#include <mutex>

namespace CZ101 {
namespace DSP {

class Arpeggiator
{
public:
    enum class Pattern { Up, Down, UpDown, Random, AsPlayed };
    enum class Rate { _1_4, _1_8, _1_16, _1_32 };
    enum class SwingMode { Off, _1_8, _1_16 };

    Arpeggiator() {}

    void setSampleRate(double sr) noexcept { sampleRate = sr; updatePhaseIncrement(); }
    void setTempo(double bpm) noexcept { currentBpm = bpm; updatePhaseIncrement(); }
    void setEnabled(bool shouldBeEnabled) noexcept { enabled = shouldBeEnabled; if (!enabled) allNotesOff(); }
    bool isEnabled() const noexcept { return enabled; }

    void setPattern(Pattern p) noexcept { currentPattern = p; reorderBuffer(); }
    void setRate(Rate r) noexcept { currentRate = r; updatePhaseIncrement(); }
    void setOctaveRange(int range) noexcept { octaveRange = std::clamp(range, 1, 4); }
    void setLatch(bool shouldLatch) noexcept { latch = shouldLatch; }
    void setGateTime(float g) noexcept { gateTime = std::clamp(g, 0.05f, 1.0f); }
    void setSwing(float s) noexcept { swingAmount = std::clamp(s, 0.0f, 1.0f); }
    void setJitter(float j) noexcept { jitterAmount = std::clamp(j, 0.0f, 1.0f); }
    void setSwingMode(SwingMode m) noexcept { currentSwingMode = m; }

    // Incoming Event Processing (Audio Thread)
    void noteOn(int note, float velocity)
    {
        juce::ScopedLock sl(lock);
        heldNotes.insert(note);
        lastVelocities[note] = velocity;
        
        if (heldNotes.size() == 1 && latch && activeNotes.empty()) {
            activeNotes.clear(); // Reset for new chord in latch mode?
            phase = 1.0f; // Reset phase to trigger immediately
        }
        
        rebuildActiveBuffer();
    }

    void noteOff(int note)
    {
        juce::ScopedLock sl(lock);
        heldNotes.erase(note);
        if (!latch && heldNotes.empty()) {
            activeNotes.clear();
        } else if (!latch) {
            rebuildActiveBuffer();
        }
    }

    // Called per sample block
    // Returns a generic "Event" struct or just callbacks?
    // Let's use a simple struct for generated events
    struct ArpEvent {
        int note;
        float velocity;
        bool isNoteOn;
    };

    void process(int numSamples, std::vector<ArpEvent>& outEvents)
    {
        if (!enabled || (activeNotes.empty() && heldNotes.empty())) return;

        // Tempo sync logic
        float samplesPerBeat = (60.0f / currentBpm) * sampleRate;
        float samplesPerStep = samplesPerBeat * getRateMultiplier();
        
        // Professional Swing Logic
        float swingFactor = 1.0f;
        if (currentSwingMode != SwingMode::Off)
        {
            // Determine if this is an "off-beat" step for the selected swing mode
            bool isSwingStep = false;
            
            // For 1/8 swing: steps are relative to quarter notes.
            // For 1/16 swing: steps are relative to quarter notes.
            // Rate determines the step size.
            // Professional swing usually delays the even subdivisions.
            
            // We need to know where we are in the beat.
            // currentStep is the index in the activeNotes buffer, not necessarily the timing position.
            // Let's use a timing counter. 
            
            // SIMPLIFIED PROFESSIONAL SWING:
            // If currentSwingMode is 1/16, we delay every even 1/16th note.
            // If currentSwingMode is 1/8, we delay every even 1/8th note.
            
            // We calculate the position in PPQ (4.0 = one whole note).
            // But we use sample counting here. Let's track absolute step count.
            
            int timingStep = absoluteStepCount;
            
            // Map timingStep to the grid defined by currentRate.
            float multiplier = getRateMultiplier(); // 1/4=1, 1/8=0.5, 1/16=0.25
            double stepInQuarters = absoluteStepCount * multiplier;
            
            if (currentSwingMode == SwingMode::_1_8) {
                // Swing on 0.5, 1.5, 2.5...
                double remainder = fmod(stepInQuarters, 1.0);
                if (std::abs(remainder - 0.5) < 0.01) isSwingStep = true;
            } else if (currentSwingMode == SwingMode::_1_16) {
                // Swing on 0.25, 0.75, 1.25, 1.75...
                double remainder = fmod(stepInQuarters, 0.5);
                if (std::abs(remainder - 0.25) < 0.01) isSwingStep = true;
            }
            
            if (isSwingStep) swingFactor = 1.0f + (swingAmount * 0.66f); // Max 66% delay (triplet feel)
            // Note: In a sample-based trigger system, delaying one step automatically shortens the gap to the next if the next is on time.
            // Complex compensation not needed for basic swing in this architecture.
        }

        float currentStepDuration = samplesPerStep * swingFactor;
        
        // Phase 8: Microtiming Jitter (Humanization)
        if (jitterAmount > 0.001f) {
             // Jitter up to ±20ms (approx 880 samples at 44.1k) scaled by amount
             float jitterSamples = (rng.nextFloat() * 2.0f - 1.0f) * (sampleRate * 0.02f) * jitterAmount;
             currentStepDuration += jitterSamples;
             // Ensure we don't go negative or too small
             if (currentStepDuration < 100.0f) currentStepDuration = 100.0f;
        }

        // Gate Logic (Note Off)
        // If gateTime < 1.0, we emit a Note Off at gateTime * currentStepDuration
        if (currentPlayingNote != -1) {
            stepsSinceNoteOn += numSamples;
            float gateSamples = currentStepDuration * gateTime;
            
            // Only send note off if we haven't already, and if gate is expired 
            // AND we aren't fully legato (gate >= 1.0 skips this, relies on next noteOn to stop prev)
            if (gateTime < 0.99f && stepsSinceNoteOn >= gateSamples) {
                 // But wait, we need to ensure we don't spam NoteOff. 
                 // We don't track "noteOffSent" clearly. 
                 // Simplification: We just use triggerNextStep to kill prev note if !legato
                 // But for true staccato we need intermediate NoteOff.
                 
                 // Better: Retrigger logic handles NoteOff. 
                 // Here we just handle early NoteOff for staccato.
                 // We need a flag? Or just use currentPlayingNote value check?
                 // Let's assume external voiceManager handles idempotent NoteOff.
                 outEvents.push_back({ currentPlayingNote, 0.0f, false });
                 currentPlayingNote = -1; 
            }
        }

        phase += numSamples; // Advance phase

        if (phase >= currentStepDuration)
        {
            phase -= currentStepDuration;
            stepsSinceNoteOn = 0; // Reset gate timer
            triggerNextStep(outEvents);
        }
    }
    
    // For manual sync (e.g. from DAW PPQ)
    void syncToHost(double ppqPosition) {
        // Advanced: implementation left for future
    }

private:
    double sampleRate = 44100.0;
    double currentBpm = 120.0;
    bool enabled = false;
    bool latch = false;
    
    Pattern currentPattern = Pattern::Up;
    Rate currentRate = Rate::_1_8;
    int octaveRange = 1;
    
    float phase = 0.0f; // Sample counter for timing
    float gateTime = 0.5f; // 0..1
    float swingAmount = 0.0f; // 0..1 (0.5 = normal swing)
    SwingMode currentSwingMode = SwingMode::Off;
    
    std::set<int> heldNotes; // Raw physical keys held
    std::vector<int> activeNotes; // Expanded buffer (sorted/rng + octaves)
    int currentStep = 0;
    int absoluteStepCount = 0;
    int currentPlayingNote = -1; // Currently sounding note (to send noteOff)
    int stepsSinceNoteOn = 0;
    
    // Phase 8
    float jitterAmount = 0.0f;
    juce::Random rng;
    
    std::map<int, float> lastVelocities;
    juce::CriticalSection lock;

    float getRateMultiplier() const {
        switch (currentRate) {
            case Rate::_1_4: return 1.0f;
            case Rate::_1_8: return 0.5f;
            case Rate::_1_16: return 0.25f;
            case Rate::_1_32: return 0.125f;
            default: return 0.5f;
        }
    }

    void updatePhaseIncrement() {
        // No pre-calc needed for sample counting method
    }

    void allNotesOff() {
        heldNotes.clear();
        activeNotes.clear();
        currentPlayingNote = -1;
        absoluteStepCount = 0;
    }

    void rebuildActiveBuffer()
    {
        // Don't clear immediately if using "Add" mode, but for simple ARP:
        activeNotes.clear();
        if (heldNotes.empty()) {
            absoluteStepCount = 0;
            return;
        }

        std::vector<int> baseNotes(heldNotes.begin(), heldNotes.end()); // Already sorted (set)
        
        // AsPlayed logic requires tracking insertion order, Set loses it.
        // For Up/Down/UpDn/Random, Set is fine.
        
        for (int oct = 0; oct < octaveRange; ++oct) {
            for (int note : baseNotes) {
                int mapped = note + (oct * 12);
                if (mapped < 128) activeNotes.push_back(mapped);
            }
        }
        
        reorderBuffer();
    }
    
    void reorderBuffer()
    {
        if (activeNotes.empty()) return;

        switch (currentPattern) {
            case Pattern::Up:
                std::sort(activeNotes.begin(), activeNotes.end());
                break;
            case Pattern::Down:
                std::sort(activeNotes.begin(), activeNotes.end(), std::greater<int>());
                break;
            case Pattern::UpDown:
                // Construct bounce? Or just index ping-pong?
                // Let's implement Index Ping-Pong in trigger step instead for efficiency
                std::sort(activeNotes.begin(), activeNotes.end());
                break;
            case Pattern::Random:
                // Shuffle handled at trigger time or here?
                // Real random picks randomly each step
                break;
            case Pattern::AsPlayed:
                // Not supported with std::set base implementation yet
                break;
        }
    }

    void triggerNextStep(std::vector<ArpEvent>& events)
    {
        if (activeNotes.empty()) return;

        // 1. Note Off previous
        if (currentPlayingNote != -1) {
            events.push_back({ currentPlayingNote, 0.0f, false });
        }

        // 2. Select next note
        int noteIndex = 0;
        
        if (currentPattern == Pattern::Random) {
            static juce::Random rng;
            noteIndex = rng.nextInt((int)activeNotes.size());
        } else if (currentPattern == Pattern::UpDown) {
            // Ping pong logic
            static bool goingUp = true;
            if (goingUp) {
                currentStep++;
                if (currentStep >= activeNotes.size()) {
                    currentStep = (int)activeNotes.size() - 2;
                    goingUp = false;
                }
            } else {
                currentStep--;
                if (currentStep < 0) {
                    currentStep = 1;
                    goingUp = true;
                }
            }
            // Clamp
            currentStep = std::max(0, std::min(currentStep, (int)activeNotes.size() - 1));
            noteIndex = currentStep;
        } else {
            // Up / Down (Buffers already sorted)
            currentStep = (currentStep + 1) % activeNotes.size();
            noteIndex = currentStep;
        }

        absoluteStepCount++;
        int note = activeNotes[noteIndex];
        // Find original velocity (modulo octave)
        int originalNote = note % 12 + (note / 12 - (note/12)) * 12; // Crude...
        // Better: find closest match in lastVelocities map? 
        // Or just store velocity in activeNotes pair?
        // Fallback to 0.8f for now
        float velocity = 0.8f; 
        
        // 3. Note On
        currentPlayingNote = note;
        events.push_back({ note, velocity, true });
    }
};

} // namespace DSP
} // namespace CZ101
