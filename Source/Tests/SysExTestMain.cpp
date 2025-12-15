#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>

// Mocking dependencies via include path manipulation (handled in CMake or manually here for simplicity if allowed)
// Since we want to test "SysExManager.cpp" logic, we need to satisfy its include of "SysExManager.h" which includes "PresetManager.h"
// We will rely on CMake to PRIORITIZE "Source/Tests/Mocks" in include path.

#include "../MIDI/SysExManager.h"
#include <juce_core/juce_core.h>

// Minimal Mock for PresetManager (definition, since we link against it)
namespace CZ101 {
namespace State {

// Static storage for test verification
static Preset capturedPreset;
static bool presetWasLoaded = false;

PresetManager::PresetManager(Parameters*, Core::VoiceManager*) {}
void PresetManager::loadPreset(int) {}
void PresetManager::savePreset(int, const std::string&) {}
void PresetManager::createFactoryPresets() {}
void PresetManager::createBassPreset() {}
void PresetManager::createLeadPreset() {}
void PresetManager::createBrassPreset() {}
void PresetManager::createStringPreset() {}
void PresetManager::createBellsPreset() {}
void PresetManager::applyPresetToProcessor() {}
void PresetManager::applyEnvelopeToVoice(const EnvelopeData&, int) {}

// The critical method we are testing
void PresetManager::loadPresetFromStruct(const Preset& p) 
{
    capturedPreset = p;
    presetWasLoaded = true;
    std::cout << "[Test] Preset Loaded: " << p.name << std::endl;
}

PresetManager::~PresetManager() {} // Destructor now declared in header

}
}

// Stub for Parameters (if needed by linker, but PresetManager stub doesn't use it)
namespace CZ101 { namespace State { class Parameters {}; } }
namespace CZ101 { namespace Core { class VoiceManager {}; } }


int main(int argc, char* argv[])
{
    std::cout << "========================================" << std::endl;
    std::cout << "      CZ-101 SysEx Logic Test" << std::endl;
    std::cout << "========================================" << std::endl;

    if (argc < 2) {
        std::cerr << "Usage: CZ101SysExTest <path_to_syx_file>" << std::endl;
        return 1;
    }

    std::string filePath = argv[1];
    std::cout << "Loading file: " << filePath << std::endl;

    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Error: Could not open file." << std::endl;
        return 1;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        std::cerr << "Error: Could not read file." << std::endl;
        return 1;
    }

    std::cout << "Read " << size << " bytes." << std::endl;

    // Initialize mock dependencies
    CZ101::State::PresetManager mockPM(nullptr, nullptr);
    CZ101::MIDI::SysExManager sysExManager(mockPM);

    // Run Parsing
    sysExManager.handleSysEx(buffer.data(), (int)size);

    if (CZ101::State::presetWasLoaded) {
        std::cout << "SUCCESS: Preset decoded!" << std::endl;
        std::cout << "  Name: " << CZ101::State::capturedPreset.name << std::endl;
        
        // Brief Envelope Dump
        auto dumpEnv = [](const char* name, const CZ101::State::EnvelopeData& env) {
            std::cout << "  " << name << " Env:" << std::endl;
            for(int i=0; i<8; ++i) {
                if (env.levels[i] > 0.0f || env.rates[i] > 0.0f)
                    std::cout << "    Step " << i << ": R=" << env.rates[i] << " L=" << env.levels[i];
                if (i == env.sustainPoint) std::cout << " [SUS]";
                if (i == env.endPoint) std::cout << " [END]";
                if (env.levels[i] > 0.0f || env.rates[i] > 0.0f) std::cout << std::endl;
            }
        };

        dumpEnv("DCA", CZ101::State::capturedPreset.dcaEnv);
        dumpEnv("DCW", CZ101::State::capturedPreset.dcwEnv);
        dumpEnv("DCO", CZ101::State::capturedPreset.pitchEnv);
        
        return 0;
    } else {
        std::cerr << "FAILURE: handleSysEx did not trigger loadPresetFromStruct." << std::endl;
        // Maybe file wasn't recognized?
        return 1;
    }
}
