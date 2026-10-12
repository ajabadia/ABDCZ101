#include "PluginEditor_ResourceProvider.h"
#include <JuceHeader.h>
#include "WebUIAssets.h"

// Helper to determine mime type based on file extension
static juce::String getMimeTypeForFilename(const juce::String& filename)
{
    if (filename.endsWithIgnoreCase(".html")) return "text/html";
    if (filename.endsWithIgnoreCase(".css"))  return "text/css";
    if (filename.endsWithIgnoreCase(".js"))   return "application/javascript";
    if (filename.endsWithIgnoreCase(".png"))  return "image/png";
    if (filename.endsWithIgnoreCase(".jpg") || filename.endsWithIgnoreCase(".jpeg")) return "image/jpeg";
    if (filename.endsWithIgnoreCase(".svg"))  return "image/svg+xml";
    if (filename.endsWithIgnoreCase(".ttf"))  return "font/ttf";
    if (filename.endsWithIgnoreCase(".woff")) return "font/woff";
    if (filename.endsWithIgnoreCase(".woff2")) return "font/woff2";
    if (filename.endsWithIgnoreCase(".json")) return "application/json";
    if (filename.endsWithIgnoreCase(".webmanifest")) return "application/manifest+json";
    return "application/octet-stream";
}

std::optional<juce::WebBrowserComponent::Resource> pluginResourceProvider(const juce::String& url)
{
    juce::String path = url;

    // Strip scheme and host if present (e.g. juce://backend/path -> /path)
    if (path.startsWith("juce://"))
    {
        int hostEndIndex = path.indexOf(7, "/");
        if (hostEndIndex != -1)
            path = path.substring(hostEndIndex);
        else
            path = "/"; // juce://backend -> /
    }
    else if (path.startsWith("https://juce.backend")) path = path.substring(20);
    else if (path.startsWith("http://localhost"))     path = path.substring(16);
    else if (path.startsWith("https://localhost"))    path = path.substring(17);

    if (path == "/" || path.isEmpty()) path = "/index.html";
    if (path.startsWith("/")) path = path.substring(1);

    // URL-decode the path (handles spaces encoded as %20, etc.)
    juce::String decodedPath = juce::URL::removeEscapeChars(path);
    // Strip cache-busting query string (?v=...)
    if (decodedPath.containsChar('?'))
        decodedPath = decodedPath.upToFirstOccurrenceOf("?", false, false);

    if (decodedPath == "juce.js" || decodedPath.endsWith("/juce.js"))
    {
        return std::nullopt; // Let JUCE WebBrowserComponent serve its built-in frontend script
    }

    // 1. PRIMARY: Match against WebUIAssets (Binary Data compiled from WebUI/dist)
    int binSize = 0;
    const char* binData = nullptr;

    juce::String normalizedDecoded = decodedPath.replace("\\", "/");
    juce::String exactDistPath = "WebUI/dist/" + normalizedDecoded;

    // Pass 1: Exact match with WebUI/dist/ prefix
    for (int i = 0; i < WebUIAssets::namedResourceListSize; ++i)
    {
        juce::String orig = juce::String::fromUTF8(WebUIAssets::originalFilenames[i]).replace("\\", "/");
        if (orig.endsWithIgnoreCase("/" + exactDistPath) || orig.equalsIgnoreCase(exactDistPath)
            || orig.endsWithIgnoreCase(exactDistPath))
        {
            binData = WebUIAssets::getNamedResource(WebUIAssets::namedResourceList[i], binSize);
            if (binData != nullptr) break;
        }
    }

    // Pass 2: Suffix fallback
    if (binData == nullptr)
    {
        for (int i = 0; i < WebUIAssets::namedResourceListSize; ++i)
        {
            juce::String orig = juce::String::fromUTF8(WebUIAssets::originalFilenames[i]).replace("\\", "/");
            if (orig.endsWithIgnoreCase("/" + normalizedDecoded) || orig.equalsIgnoreCase(normalizedDecoded))
            {
                binData = WebUIAssets::getNamedResource(WebUIAssets::namedResourceList[i], binSize);
                if (binData != nullptr) break;
            }
        }
    }

    // Pass 3: Direct flattened names fallback
    if (binData == nullptr)
    {
        juce::String resourceName = decodedPath.replace("/", "_")
                                               .replace("\\", "_")
                                               .replace(".", "_")
                                               .replace("-", "_")
                                               .replace(" ", "_");
        binData = WebUIAssets::getNamedResource(resourceName.toRawUTF8(), binSize);
    }

    // Pass 4: Basename matching for assets/ paths (e.g. assets/index.js -> index_js)
    if (binData == nullptr && decodedPath.startsWith("assets/"))
    {
        juce::String basename = decodedPath.fromLastOccurrenceOf("/", false, false);
        if (basename.isNotEmpty())
        {
            juce::String flattenedBasename = basename.replace(".", "_").replace("-", "_").replace(" ", "_");
            binData = WebUIAssets::getNamedResource(flattenedBasename.toRawUTF8(), binSize);
        }
    }

    // Pass 5: Root-level basename fallback (e.g. index.html -> index_html)
    if (binData == nullptr && decodedPath.length() > 0 && !decodedPath.containsChar('/'))
    {
        juce::String flattenedName = decodedPath.replace(".", "_").replace("-", "_").replace(" ", "_");
        if (juce::CharacterFunctions::isDigit(flattenedName[0]))
            flattenedName = "_" + flattenedName;
        binData = WebUIAssets::getNamedResource(flattenedName.toRawUTF8(), binSize);
    }

    if (binData != nullptr)
    {
        std::vector<std::byte> bytes((size_t)binSize);
        std::memcpy(bytes.data(), binData, (size_t)binSize);
        return juce::WebBrowserComponent::Resource { std::move(bytes), getMimeTypeForFilename(decodedPath).toStdString() };
    }

    // 2. Secondary Disk Fallback (Development Mode: prefer WebUI/dist/)
    juce::File thisFile(__FILE__);
    juce::File projectDir = thisFile.getParentDirectory() // Source/
                                    .getParentDirectory(); // ABDCZ101/
    juce::File distDir = projectDir.getChildFile("WebUI").getChildFile("dist");
    juce::File webUiDir = projectDir.getChildFile("WebUI");

    juce::String diskRel = decodedPath.replace("/", "\\");
    juce::File file = distDir.getChildFile(diskRel);
    if (!file.existsAsFile())
        file = webUiDir.getChildFile(diskRel);

    if (file.existsAsFile())
    {
        juce::MemoryBlock mb;
        file.loadFileAsData(mb);
        std::vector<std::byte> data(mb.getSize());
        std::memcpy(data.data(), mb.getData(), mb.getSize());
        return juce::WebBrowserComponent::Resource { std::move(data), getMimeTypeForFilename(file.getFileName()).toStdString() };
    }

    // Log missing file for debug
    juce::Logger::writeToLog("[ResourceProvider] ERROR: File not found in WebUIAssets or disk for path: " + decodedPath);
    return std::nullopt;
}
