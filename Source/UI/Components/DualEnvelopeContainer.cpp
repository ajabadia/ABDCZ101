#include "DualEnvelopeContainer.h"

namespace CZ101 {
namespace UI {

DualEnvelopeContainer::DualEnvelopeContainer(CZ101::UI::EnvelopeEditor& e1, CZ101::UI::EnvelopeEditor& e2) 
    : l1(e1), l2(e2) 
{
    addAndMakeVisible(l1); 
    addAndMakeVisible(l2);
    addAndMakeVisible(label1); 
    addAndMakeVisible(label2);
    
    label1.setJustificationType(juce::Justification::centred);
    label2.setJustificationType(juce::Justification::centred);
    label1.setColour(juce::Label::textColourId, juce::Colours::cyan);
}

void DualEnvelopeContainer::resized() 
{
    auto area = getLocalBounds();
    auto h = area.getHeight() / 2;
    auto area1 = area.removeFromTop(h);
    
    label1.setBounds(area1.removeFromTop(20));
    l1.setBounds(area1);
    
    label2.setBounds(area.removeFromTop(20));
    l2.setBounds(area);
}

} // namespace UI
} // namespace CZ101
