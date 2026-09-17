#include "PluginEditor.h"

// Helper to create WebBrowserComponent options
static juce::WebBrowserComponent::Options createWebOptions()
{
    return juce::WebBrowserComponent::Options()
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withNativeFunction("setParameterValue", [](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion) {
            // JS calls this with: setParameterValue("paramID", value)
            if (args.size() == 2 && args[0].isString() && args[1].isVoid() == false)
            {
                // We'll handle the actual parameter change in the class to access APVTS
                // For a static helper, this is tricky. We should instead use a member method to create options.
            }
            completion("");
        });
}
