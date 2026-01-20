#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "EnvelopeEditor.h"

namespace CZ101 {
namespace UI {

class DualEnvelopeContainer : public juce::Component {
public:
    DualEnvelopeContainer(CZ101::UI::EnvelopeEditor& e1, CZ101::UI::EnvelopeEditor& e2);
    void resized() override;

private:
    CZ101::UI::EnvelopeEditor& l1;
    CZ101::UI::EnvelopeEditor& l2;
    juce::Label label1 { {}, "LINE 1" };
    juce::Label label2 { {}, "LINE 2" };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DualEnvelopeContainer)
};

} // namespace UI
} // namespace CZ101
