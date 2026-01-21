#pragma once
#include <atomic>
#include <memory>
#include <juce_core/juce_core.h>

namespace CZ101 {
namespace Core {

/**
 * A lock-free snapshot system for passing parameter structures from the 
 * Message Thread (UI) to the Audio Thread safely.
 */
struct ParameterSnapshot {
    struct DCOParams {
         int wave1 = 0, wave2 = 0;
         float level = 1.0f;
         int octave = 0;
         int coarse = 0;
         int fine = 0;
    } dco1, dco2;

    struct LineModParams {
        bool ring = false;
        bool noise = false;
    } lineMod;

    struct SystemParams {
        float masterVol = 1.0f;
        float masterTune = 0.0f;
        float bendRange = 2.0f;
        int voiceLimit = 8;
        bool hardwareNoise = false;
        int oversampling = 1; // [NEW]
        int opMode = 0; // [NEW] 0=CZ101, 1=CZ5000
        int midiChannel = 1; // [NEW]
    } system;

    struct ModParams {
        float veloDcw = 0.0f, veloAmp = 0.0f;
        float wheelToDcw = 0.0f, wheelToLfoRate = 0.0f, wheelToVibrato = 0.0f;
        float atToDcw = 0.0f, atToVibrato = 0.0f;
        int keyFollowDcw = 0; // 0=OFF, 1=FIX, 2=VAR
        int keyFollowAmp = 0;
        int keyFollowDco = 0; // [NEW]
        int detune = 0; 
        float glideTime = 0.0f; // [NEW]
    } mod;
    
    struct ArpParams {
        bool enabled = false;
        bool latch = false;
        int rate = 1; // 1/8
        int pattern = 0; // UP
        int octave = 1;
        float gate = 1.0f;
        float swing = 0.0f;
        int swingMode = 0; // 8th or 16th
    } arp;

    struct LfoParams {
        float rate = 1.0f;
        int waveform = 0;
        float depth = 0.0f;
        float delay = 0.0f;
    } lfo;

    struct EffectsParams {
        bool chorusOn = false;
        float chorusRate = 0.0f, chorusDepth = 0.0f, chorusMix = 0.0f;
        
        // [NEW] Delay
        float delayTime = 0.25f;
        float delayFb = 0.0f;
        float delayMix = 0.0f;
        
        // [NEW] Reverb
        float reverbSize = 0.5f;
        float reverbMix = 0.0f;

        // [NEW] Drive
        float driveAmount = 0.0f;
        float driveColor = 0.5f;
        float driveMix = 0.0f;

        // [NEW] Modern Filters
        float lpfCutoff = 20000.0f;
        float lpfReso = 0.0f;
        float hpfCutoff = 20.0f;
    } effects;
    
    // We include basic envelope settings for snapshot restoration
    // Full 8-stage data is heavy but necessary for full state restoration
    struct EnvParam {
        float rates[8] = {0};
        float levels[8] = {0};
        int sustain = -1;
        int end = 0;
    };
    
    struct VoiceEnvelopes {
        EnvParam dcw1, dcw2;
        EnvParam dca1, dca2;
        EnvParam pitch1, pitch2; // Optional
    } envelopes;
};

class AudioThreadSnapshot {
public:
    AudioThreadSnapshot() : currentSnapshot(new ParameterSnapshot()) {}
    
    ~AudioThreadSnapshot() {
        if (auto* ptr = currentSnapshot.exchange(nullptr))
            delete ptr;
    }

    /**
     * Publishes a new snapshot to the audio thread.
     * Called from Message Thread.
     */
    void commit(std::unique_ptr<ParameterSnapshot> next) {
        auto* old = currentSnapshot.exchange(next.release(), std::memory_order_acq_rel);
        if (old) {
            // In a production environment, we might defer deletion or use a pool
            // to be strictly real-time safe if the Message Thread can't block.
            // But since this is called from Message Thread, standard delete is usually fine.
            delete old;
        }
    }

    /**
     * Retrieves the current snapshot.
     * Called from Audio Thread.
     */
    const ParameterSnapshot* get() const noexcept {
        return currentSnapshot.load(std::memory_order_acquire);
    }

private:
    alignas(64) std::atomic<ParameterSnapshot*> currentSnapshot{ nullptr };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioThreadSnapshot)
};

} // namespace Core
} // namespace CZ101
