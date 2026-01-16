#pragma once
#include <JuceHeader.h>
#include "DesignTokens.h"

namespace CZ101 {
namespace UI {

class ScaledComponent : public juce::Component
{
public:
    ScaledComponent() = default;
    virtual ~ScaledComponent() override = default;

    float getUiScale() const
    {
        return DesignTokens::layoutScale;
    }

    juce::Rectangle<int> scaleBounds(juce::Rectangle<int> bounds) const
    {
        return bounds * getUiScale();
    }
    
    juce::Font getScaledFont(float size) const
    {
        return juce::Font("Verdana", size * getUiScale(), 0); 
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScaledComponent)
};

} // namespace UI
} // namespace CZ101
