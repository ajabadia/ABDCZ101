/**
 * wasm_link_stubs.cpp — Mocks and Linker Stubs for JUCE WASM build.
 */

#include <JuceHeader.h>

namespace juce
{

// Minimal inline declaration of juce::Colour to satisfy compiler since juce_graphics is absent
class Colour
{
public:
    Colour (uint32 colorARGB) noexcept;
private:
    uint32 argb;
};

// Stub for juce::Colour constructor from uint32.
Colour::Colour (uint32 colorARGB) noexcept
{
    // Write the ARGB bytes directly to the start of the Colour class memory
    *(uint32*)(this) = colorARGB;
}

// Stub for juce::File::getSpecialLocation
File File::getSpecialLocation (const SpecialLocationType)
{
    return File();
}

// Stub for MessageManager::postMessageToSystemQueue
#if JUCE_MODULE_AVAILABLE_juce_events
bool MessageManager::postMessageToSystemQueue (MessageManager::MessageBase* message)
{
    delete message;
    return true;
}
#endif

// Stubs for DirectoryIterator::NativeIterator and juce::File Posix functions
class DirectoryIterator::NativeIterator::Pimpl {};

DirectoryIterator::NativeIterator::NativeIterator (const File&, const String&) {}
DirectoryIterator::NativeIterator::~NativeIterator() {}
bool DirectoryIterator::NativeIterator::next (String&, bool*, bool*, int64*, Time*, Time*, bool*) { return false; }

bool File::isSymbolicLink() const { return false; }
String File::getNativeLinkedTarget() const { return String(); }
bool File::copyInternal (const File&) const { return false; }

} // namespace juce
