#include "UIManager.h"
#include "../PluginEditor.h"

namespace CZ101 {
namespace UI {

UIManager::UIManager()
{
}

UIManager::~UIManager()
{
}

void UIManager::setCurrentTab(int index)
{
    if (currentTabIndex != index)
    {
        currentTabIndex = index;
        sendChangeMessage();
    }
}

void UIManager::setFocusedParameter(const juce::String& paramId)
{
    if (focusedParamId != paramId)
    {
        focusedParamId = paramId;
        sendChangeMessage();
    }
}

UIManager* UIManager::findInParentHierarchy(juce::Component* c)
{
    if (c == nullptr) return nullptr;

    auto* parent = c->getParentComponent();
    while (parent != nullptr)
    {
        if (auto* editor = dynamic_cast<CZ101AudioProcessorEditor*>(parent))
        {
            // We assume the editor has a method or member getUIManager()
            // For now, we'll need to define this in PluginEditor.h
            return &editor->getUIManager();
        }
        parent = parent->getParentComponent();
    }

    return nullptr;
}

} // namespace UI
} // namespace CZ101
