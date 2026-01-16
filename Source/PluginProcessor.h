#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <memory>
#include "Utils/PerformanceMonitor.h"
#include "Core/VoiceManager.h"
#include "MIDI/MIDIProcessor.h"
#include "MIDI/SysExManager.h"
#include "State/Parameters.h"
#include "State/PresetManager.h"

#include "DSP/Effects/Delay.h"
#include "DSP/Effects/Chorus.h"
#include "DSP/Effects/Reverb.h"
#include "DSP/Modulation/LFO.h"
// #include "UI/LCDStateManager.h" // Removed to prevent circular dependency
namespace CZ101 { namespace UI { class LCDStateManager; } }

// Command Pattern for Thread Safety
enum class InitSection { WAVEFORM, DCO, DCW, DCA, VIBRATO, OCTAVE, ALL, SYSTEM_ALL };

struct EnvelopeUpdateCommand
{
    enum Type { DCA_STAGE, DCW_STAGE, PITCH_STAGE, 
                DCA_SUSTAIN, DCW_SUSTAIN, PITCH_SUSTAIN,
                DCA_END, DCW_END, PITCH_END };
    Type type;
    int line; // 1 or 2
    int index;
    float rate;
    float level;
};

// Audit Fix: POD version of Envelope state for lock-free audio thread updates
struct EnvelopeStatePOD
{
    CZ101::State::EnvelopeData pitchEnv, dcwEnv, dcaEnv;
    CZ101::State::EnvelopeData pitchEnv2, dcwEnv2, dcaEnv2;
};

static_assert(std::is_trivially_copyable<EnvelopeUpdateCommand>::value, "EnvelopeUpdateCommand must be POD for thread-safe FIFO usage");
static_assert(std::is_trivially_copyable<EnvelopeStatePOD>::value, "EnvelopeStatePOD must be POD for thread-safe FIFO usage");

class CZ101AudioProcessor : public juce::AudioProcessor, 
                            public juce::AsyncUpdater
{
public:
    CZ101AudioProcessor();
    ~CZ101AudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    
    // Audit Fix 2.1: Non-realtime support
    void setNonRealtime(bool isNonRealtime) noexcept override;
    juce::AudioParameterBool* getBypassParameter() const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    
    // Audit Fix 41: Safe Sample Rate access for Editor
    // getSampleRate is non-virtual in JUCE, so we define a custom safe getter.
    double getSafeSampleRate() const noexcept { return currentSampleRate.load(); }
    
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;
    
    void saveCurrentPreset(const juce::String& name); // New Save Method
    void applyPresetEnvelopes(const EnvelopeStatePOD& pod); // Lock-free SysEx Helper
    void initializeSection(InitSection section); // Authentic Initialization

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    CZ101::State::PresetManager& getPresetManager() { return presetManager; }
    CZ101::MIDI::SysExManager& getSysExManager() { return sysExManager; }
    CZ101::MIDI::MIDIProcessor& getMidiProcessor() { return midiProcessor; }
    CZ101::State::Parameters& getParameters() { return parameters; }
    CZ101::Core::VoiceManager& getVoiceManager() { return voiceManager; }
    CZ101::Utils::PerformanceMonitor& getPerformanceMonitor() { return performanceMonitor; }

    juce::UndoManager& getUndoManager() { return undoManager; }
    
    // For Editor Visualization - Triple Buffering for Thread Safety
    struct VisTripleBuffer {
        static constexpr int SIZE = 4096;
        std::array<std::array<float, SIZE>, 3> buffers;
        std::atomic<int> backIndex { 0 }; // owned by Audio Thread
        std::atomic<int> midIndex { 1 };  // shared
        std::atomic<int> frontIndex { 2 }; // owned by UI Thread
        std::atomic<bool> hasNewData { false };
    };
    VisTripleBuffer& getVisTripleBuffer() { return visTripleBuffer; }

    // CASIO CZ COMMAND QUEUE (Thread-Safe UI -> Audio)
    void scheduleEnvelopeUpdate(const EnvelopeUpdateCommand& cmd);

    // Callbacks
    std::function<void()> requestAudioSettings; // For Standalone Audio/MIDI Settings

    // --- UI State Management ---
    std::unique_ptr<CZ101::UI::LCDStateManager> lcdStateManager;
    CZ101::UI::LCDStateManager& getLCDStateManager() { return *lcdStateManager; }

private:
    // ...
    // Visualisation
    VisTripleBuffer visTripleBuffer; // Audit Fix 1.3: Waveform Triple Buffer
    CZ101::Core::VoiceManager voiceManager;
    CZ101::MIDI::MIDIProcessor midiProcessor;
    juce::UndoManager undoManager; // Added UndoManager (Must be before Parameters)
    CZ101::State::Parameters parameters;
    CZ101::State::PresetManager presetManager;
    CZ101::MIDI::SysExManager sysExManager;
    

    CZ101::DSP::Effects::Delay delayL;
    CZ101::DSP::Effects::Delay delayR;
    
    juce::Reverb reverb;
    juce::Reverb::Parameters reverbParams;
    CZ101::DSP::Effects::Chorus chorus;
    
    // Modern Filters
    juce::dsp::LadderFilter<float> modernLpf;
    juce::dsp::StateVariableTPTFilter<float> modernHpf;
    
    // UI Update Tracking
    CZ101::Utils::PerformanceMonitor performanceMonitor;
    
    // Command Queue Data
    static constexpr int COMMAND_QUEUE_SIZE = 4096; // Audit Fix 4.1: Increased for high SR
    juce::AbstractFifo commandFifo { COMMAND_QUEUE_SIZE };
    std::array<EnvelopeUpdateCommand, COMMAND_QUEUE_SIZE> commandBuffer;
    
    void handleAsyncUpdate() override;
    
    // Audit Fix [D]: Mutex ELIMINATED. Using lock-free patterns.
    
    // Audit Fix 4.2: Pending update from SysEx
    juce::CriticalSection sysExLock; // Protects the pendingSysExPreset unique_ptr
    std::unique_ptr<CZ101::State::Preset> pendingSysExPreset;
    std::atomic<bool> hasPendingSysEx { false };    

    // Audit Fix: Lock-free POD Swap for audio thread
    static constexpr int PRESET_FIFO_SIZE = 4;
    juce::AbstractFifo presetFifo { PRESET_FIFO_SIZE };
    std::array<EnvelopeStatePOD, PRESET_FIFO_SIZE> presetBuffer;
    std::atomic<double> currentSampleRate { 44100.0 };
    
    // Audit Fix: Performance optimization
    struct ParamCache {
        int lineSelect = -1;
        float osc1Level = -1.0f;
        float osc2Level = -1.0f;
        int w1_1 = -1, w1_2 = -1, w2_1 = -1, w2_2 = -1;
        
        // Mod Matrix
        float veloToDcw = -1.0f, veloToDca = -1.0f;
        float wheelToDcw = -1.0f, wheelToLfo = -1.0f, wheelToVib = -1.0f;
        float atToDcw = -1.0f, atToVib = -1.0f;
        float ktDcw = -1.0f, ktPitch = -1.0f;
        int kfDco = -1, kfDcw = -1, kfDca = -1;
        
        // LFO
        float lfoRate = -1.0f, lfoDepth = -1.0f, lfoDelay = -1.0f;
        int lfoWave = -1;

        // Effects
        float chorusRate = -1.0f, chorusDepth = -1.0f, chorusMix = -1.0f;
        float delayTime = -1.0f, delayFb = -1.0f, delayMix = -1.0f;
        float revSize = -1.0f, revMix = -1.0f;
        
    } paramCache;

    void processEnvelopeUpdates();
    
    void updateParameters();

    std::unique_ptr<juce::FileLogger> fileLogger;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CZ101AudioProcessor)
};
