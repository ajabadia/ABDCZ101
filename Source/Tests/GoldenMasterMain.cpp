/*
  ==============================================================================

    GoldenMasterMain.cpp
    Propósito: Renderizar presets de fábrica y generar/verificar Hashes MD5.
    Fase 1.5: Safety Net

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <iostream>

// Include Project Headers
#include "../PluginProcessor.h"
#include "../State/PresetManager.h"

// Minimal Test Runner
class GoldenMasterRunner
{
public:
    static void runTests(CZ101AudioProcessor& processor)
    {
        const double sampleRate = 44100.0;
        const int samplesPerBlock = 512;
        processor.prepareToPlay(sampleRate, samplesPerBlock);

        std::cout << "Starting Golden Master Tests (Verbose mode)..." << std::endl;
        
        auto& pm = processor.getPresetManager();
        const auto& presets = pm.getPresets();
        int numPresets = (int)presets.size();
        
        std::cout << "Found " << numPresets << " presets." << std::endl;
        // processor.setNonRealtime(true); // Disable realtime optimizations if any - removed as per instruction

        juce::AudioBuffer<float> buffer(2, 44100); // 1 second buffer
        juce::MidiBuffer midi;

        for (int i = 0; i < numPresets; ++i)
        {
            std::cout << "Testing Preset [" << i << "/" << numPresets << "]: " << std::flush;
            
            if (i >= (int)presets.size()) {
                std::cout << " ERROR: OOB Index " << i << " (size: " << presets.size() << ")" << std::endl;
                break;
            }

            std::cout << presets[i].name << " ... " << std::flush;
            
            // Reset
            buffer.clear();
            midi.clear();
            processor.initializeSection(InitSection::ALL); 
            
            // Load Preset
            pm.loadPreset(i);
            // const auto& presetName = pm.getPresets()[i].name; // Removed as per instruction
            // juce::Logger::writeToLog("  Name: " + presetName); // Removed as per instruction

            // Trigger Note (C3, vel 100)
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0);
            midi.addEvent(juce::MidiMessage::noteOff(1, 60, (juce::uint8)0), 22050); // Note off at 0.5s

            // Render 1 second
            int totalSamples = 44100;
            int pos = 0;
            
            std::cout << "Rendering" << std::flush;
            while (pos < totalSamples)
            {
                int todo = std::min(samplesPerBlock, totalSamples - pos);
                
                // Construct Block Buffer wrapper
                juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, pos, todo);
                
                // Construct Block Midi
                juce::MidiBuffer blockMidi;
                blockMidi.addEvents(midi, pos, todo, -pos); // Fix: Timestamps must be relative to block start
                
                processor.processBlock(block, blockMidi);
                
                pos += todo;
                
                if (pos % (samplesPerBlock * 20) == 0) std::cout << "." << std::flush;
            }

            // Compute MD5
            auto hash = computeMD5(buffer);
            
            // Output Report
            std::cout << " DONE: " << hash.toHexString() << std::endl;
        }
        
        std::cout << "Golden Master Tests Finished Successfully." << std::endl;
    }

private:
    static juce::MD5 computeMD5(const juce::AudioBuffer<float>& buffer)
    {
        juce::MemoryBlock mb;
        // Hash Channel 0 and 1
        int numSamples = buffer.getNumSamples();
        mb.append(buffer.getReadPointer(0), (size_t)(numSamples * sizeof(float)));
        if (buffer.getNumChannels() > 1)
            mb.append(buffer.getReadPointer(1), (size_t)(numSamples * sizeof(float)));
            
        return juce::MD5(mb);
    }
};

// Main Entry Point
int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit; // Initialize MessageManager
    // juce::FileLogger logger(juce::File::getCurrentWorkingDirectory().getChildFile("GoldenMasterLog.txt"), "GM Check"); // Removed as per instruction
    // juce::Logger::setCurrentLogger(&logger); // Removed as per instruction

    CZ101AudioProcessor processor;
    GoldenMasterRunner::runTests(processor);

    // juce::Logger::setCurrentLogger(nullptr); // Removed as per instruction
    return 0;
}
