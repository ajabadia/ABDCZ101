#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../State/PresetManager.h"

namespace CZ101 {
namespace UI {

    class LCDStateManager : public juce::ChangeBroadcaster,
                            public juce::AudioProcessorValueTreeState::Listener,
                            private juce::Timer
    {
    public:
        // ... (enums) ...
        enum class Mode {
            NORMAL,   // Program Select / Tone Mix
            EDIT,     // Parameter Editing
            SYSTEM,    // System Settings (MIDI, Protect)
            COMPARE   // Comparing with original sound
        };
        
        enum class Section {
            DCO,
            DCW,
            DCA,
            LFO,
            MOD,
            SYSTEM,
            NONE
        };

        LCDStateManager(juce::AudioProcessorValueTreeState& apvts);
        ~LCDStateManager() override;

        // Listener Override
        void parameterChanged(const juce::String& parameterID, float newValue) override;

        // Navigation Inputs
        void onCursorLeft();
        void onCursorRight();
        void onValueUp();
        void onValueDown();
        void onCompareButton(); // Toggles Compare
        void onWriteButton();   // Triggers Write Mode

        // Display Data Access
        juce::String getTopLineText() const;
        juce::String getBottomLineText() const;
        
        // Mode Switching
        void setMode(Mode newMode);
        Mode getMode() const { return currentMode; }
        bool isAuthentic() const;
        bool isShowingModernParam() const { return modernParamActive; }
        
        // UI Helpers
        void showProgramMode();
        void onPageChanged(const juce::String& pageName);
        void setParameterFeedbackSuppressed(bool suppressed) { feedbackSuppressed = suppressed; }

    private:
        juce::AudioProcessorValueTreeState& apvts;
        Mode currentMode = Mode::NORMAL;
        
        // Navigation State
        int currentParameterIndex = 0;
        int currentProgramIndex = 0; // 0-31 (Internal)
        
        // Parameter List for Edit Mode
        struct ParamInfo {
            juce::String id;
            Section section;
            juce::String nameOverride;
        };
        
        std::vector<ParamInfo> parameterList;
        void buildParameterList();
        
        // Navigation state
        Section currentSection = Section::NONE;
        
        // Helpers
        void updateDisplay();
        void modifyValue(bool isUp);
        bool isAuthenticParameter(const juce::String& id) const;
        
        // Compare Logic
        CZ101::State::Preset originalPreset;
        bool isComparing = false;
        void performCompareToggle();

        // Timer implementation
        void timerCallback() override;
        
        // Internal data
        juce::String topLine;
        juce::String bottomLine;
        
        juce::uint32 lastActivityTime = 0;
        bool showingTemporaryParameter = false;
        bool modernParamActive = false;
        bool feedbackSuppressed = false;
        juce::String temporaryParamId;
        
        // Acceleration State
        juce::int64 lastValueChangeTime = 0;
        int consecutiveValueChanges = 0;

        // Blinking
        bool blinkState = true;
        juce::uint32 lastBlinkTime = 0;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LCDStateManager)
    };

} // namespace UI
} // namespace CZ101
