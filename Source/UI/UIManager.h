#pragma once

#include <JuceHeader.h>

namespace CZ101 {
namespace UI {

/**
 * @brief UIManager - Central hub for UI-specific state and messaging.
 * 
 * Decouples individual components from global UI state like current tab,
 * focused parameter, and modal visibility.
 */
class UIManager : public juce::ChangeBroadcaster
{
public:
    UIManager();
    ~UIManager() override;

    // --- Tab Management ---
    void setCurrentTab(int index);
    int getCurrentTab() const { return currentTabIndex; }

    // --- Parameter Focus ---
    /**
     * @brief Broadcasts that a parameter is being interacted with.
     * Useful for temporary LCD displays or help text.
     */
    void setFocusedParameter(const juce::String& paramId);
    juce::String getFocusedParameter() const { return focusedParamId; }

    // --- Global Helpers ---
    /**
     * @brief Convenient way for components to find the UIManager 
     * by traversing up the component hierarchy.
     */
    static UIManager* findInParentHierarchy(juce::Component* c);

private:
    int currentTabIndex = 0;
    juce::String focusedParamId;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UIManager)
};

} // namespace UI
} // namespace CZ101
