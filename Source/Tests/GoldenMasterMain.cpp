/*
  ==============================================================================

    GoldenMasterMain.cpp
    Propósito: Análisis Masivo de Audio, Diagnóstico de Bugs y SysEx Roundtrip.

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>

// Include Project Headers
#include "../PluginProcessor.h"
#include "../State/PresetManager.h"
#include "../MIDI/SysExManager.h"

struct AudioMetrics
{
    float peak = 0.0f;
    float peakDb = -100.0f;
    float rms = 0.0f;
    float rmsDb = -100.0f;
    float dcOffset = 0.0f;
    bool hasNaNOrInf = false;
    float maxReleaseStep = 0.0f;
    float chordPeak = 0.0f;
    float chordPeakDb = -100.0f;
    bool sysexMatch = true;
    std::string sysexMismatchReason;
};

class MassiveAudioAnalyzer
{
public:
    static AudioMetrics analyzePreset(CZ101AudioProcessor& processor, int presetIndex, const std::string& name)
    {
        AudioMetrics metrics;
        const double sampleRate = 44100.0;
        const int samplesPerBlock = 512;
        auto& pm = processor.getPresetManager();

        // 1. Reset and Load
        processor.initializeSection(InitSection::ALL);
        pm.loadPreset(presetIndex);

        // 2. Single Note Test (C3, vel 100, 0.5s On, 0.5s Release)
        const int totalSamples = 44100;
        const int noteOffSample = 22050;
        juce::AudioBuffer<float> buffer(2, totalSamples);
        buffer.clear();

        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0);
        midi.addEvent(juce::MidiMessage::noteOff(1, 60, (juce::uint8)0), noteOffSample);

        int pos = 0;
        while (pos < totalSamples)
        {
            int todo = std::min(samplesPerBlock, totalSamples - pos);
            juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, pos, todo);
            juce::MidiBuffer blockMidi;
            blockMidi.addEvents(midi, pos, todo, -pos);
            processor.processBlock(block, blockMidi);
            pos += todo;
        }

        // Analyze Single Note Audio
        float sumSq = 0.0f;
        float sumDC = 0.0f;
        int activeCount = 0;
        float maxAbs = 0.0f;
        const float* left = buffer.getReadPointer(0);
        const float* right = buffer.getReadPointer(1);

        for (int i = 0; i < totalSamples; ++i)
        {
            float sL = left[i];
            float sR = right[i];

            if (std::isnan(sL) || std::isinf(sL) || std::isnan(sR) || std::isinf(sR))
            {
                metrics.hasNaNOrInf = true;
                break;
            }

            float absL = std::abs(sL);
            float absR = std::abs(sR);
            maxAbs = std::max(maxAbs, std::max(absL, absR));
            sumSq += sL * sL + sR * sR;

            // Measure DC offset during steady state (samples 4000 to 20000)
            if (i >= 4000 && i < 20000)
            {
                sumDC += (sL + sR) * 0.5f;
                activeCount++;
            }

            // Check for abrupt click near noteOff boundary
            if (i > noteOffSample && i < noteOffSample + 64)
            {
                float stepL = std::abs(left[i] - left[i - 1]);
                float stepR = std::abs(right[i] - right[i - 1]);
                metrics.maxReleaseStep = std::max(metrics.maxReleaseStep, std::max(stepL, stepR));
            }
        }

        metrics.peak = maxAbs;
        metrics.peakDb = maxAbs > 0.00001f ? 20.0f * std::log10(maxAbs) : -100.0f;
        metrics.rms = std::sqrt(sumSq / (float)(totalSamples * 2));
        metrics.rmsDb = metrics.rms > 0.00001f ? 20.0f * std::log10(metrics.rms) : -100.0f;
        metrics.dcOffset = activeCount > 0 ? std::abs(sumDC / (float)activeCount) : 0.0f;

        // 3. Polyphonic 4-Note Chord Test (C3, E3, G3, B3, vel 100)
        processor.initializeSection(InitSection::ALL);
        pm.loadPreset(presetIndex);

        juce::AudioBuffer<float> chordBuffer(2, 22050);
        chordBuffer.clear();
        juce::MidiBuffer chordMidi;
        chordMidi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0);
        chordMidi.addEvent(juce::MidiMessage::noteOn(1, 64, (juce::uint8)100), 0);
        chordMidi.addEvent(juce::MidiMessage::noteOn(1, 67, (juce::uint8)100), 0);
        chordMidi.addEvent(juce::MidiMessage::noteOn(1, 71, (juce::uint8)100), 0);

        pos = 0;
        while (pos < 22050)
        {
            int todo = std::min(samplesPerBlock, 22050 - pos);
            juce::AudioBuffer<float> block(chordBuffer.getArrayOfWritePointers(), 2, pos, todo);
            juce::MidiBuffer blockMidi;
            blockMidi.addEvents(chordMidi, pos, todo, -pos);
            processor.processBlock(block, blockMidi);
            pos += todo;
        }

        float maxChordAbs = 0.0f;
        const float* cL = chordBuffer.getReadPointer(0);
        const float* cR = chordBuffer.getReadPointer(1);
        for (int i = 0; i < 22050; ++i)
        {
            if (!std::isnan(cL[i]) && !std::isinf(cL[i]))
                maxChordAbs = std::max(maxChordAbs, std::abs(cL[i]));
            if (!std::isnan(cR[i]) && !std::isinf(cR[i]))
                maxChordAbs = std::max(maxChordAbs, std::abs(cR[i]));
        }
        metrics.chordPeak = maxChordAbs;
        metrics.chordPeakDb = maxChordAbs > 0.00001f ? 20.0f * std::log10(maxChordAbs) : -100.0f;

        // 4. SysEx Round-trip Verification
        if (presetIndex < (int)pm.getPresets().size())
        {
            const auto& orig = pm.getPresets()[presetIndex];
            CZ101::MIDI::SysExManager sysEx;
            auto dump = sysEx.createPatchDump(orig);
            if (dump.getSize() >= 264 && dump.getSize() <= 296)
            {
                CZ101::State::Preset decoded;
                if (CZ101::MIDI::SysExManager::decodePatch(static_cast<const uint8_t*>(dump.getData()), decoded))
                {
                    // Compare critical parameters
                    for (const auto& [k, v] : orig.parameters)
                    {
                        auto it = decoded.parameters.find(k);
                        if (it != decoded.parameters.end())
                        {
                            float tolerance = 0.05f;
                            if (k.find("lfoRate") != std::string::npos || k.find("LFO_RATE") != std::string::npos)
                                tolerance = 0.25f; // 1 step of 0-99 across 20Hz = 0.202Hz
                            
                            if (std::abs(it->second - v) > tolerance)
                            {
                                if (k.find("detune") == std::string::npos && k.find("cutoff") == std::string::npos)
                                {
                                    metrics.sysexMatch = false;
                                    metrics.sysexMismatchReason = k + " orig=" + std::to_string(v) + " dec=" + std::to_string(it->second);
                                    break;
                                }
                            }
                        }
                    }
                }
                else
                {
                    metrics.sysexMatch = false;
                    metrics.sysexMismatchReason = "decodePatch returned false";
                }
            }
            else
            {
                metrics.sysexMatch = false;
                metrics.sysexMismatchReason = "Dump size unexpected (" + std::to_string(dump.getSize()) + ")";
            }
        }

        return metrics;
    }

    static void testSysExBanks(CZ101AudioProcessor& processor)
    {
        std::cout << "\n================================================================================" << std::endl;
        std::cout << "                  EXTERNAL SYSEX BANK COMPATIBILITY SCAN                        " << std::endl;
        std::cout << "================================================================================" << std::endl;

        std::vector<std::string> bankFiles = {
            "WebUI/public/presets/cz5000/cz1org64/CZ1ORGB1.SYX",
            "WebUI/public/presets/cz5000/cz1org64/CZ1ORGB2.SYX",
            "WebUI/public/presets/cz5000/allnetcz/STRINGS.SYX",
            "WebUI/public/presets/cz5000/allnetcz/SYNBASS.SYX",
            "WebUI/public/presets/cz5000/allnetcz/PRESETS0.SYX",
            "WebUI/public/presets/cz5000/allnetcz/SYNTH.SYX",
            "WebUI/public/presets/cz5000/allnetcz/BRASS1.SYX"
        };

        int totalBankPatches = 0;
        int passedBankPatches = 0;

        for (const auto& relPath : bankFiles)
        {
            juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile(relPath);
            if (!f.existsAsFile())
            {
                // Try parent path
                f = juce::File::getCurrentWorkingDirectory().getParentDirectory().getChildFile(relPath);
            }
            if (!f.existsAsFile())
            {
                std::cout << "[SKIP] File not found: " << relPath << std::endl;
                continue;
            }

            juce::MemoryBlock mb;
            f.loadFileAsData(mb);

            std::cout << "Testing Bank: " << f.getFileName() << " (" << mb.getSize() << " bytes) ... " << std::flush;

            std::vector<CZ101::State::Preset> bankPresets;
            CZ101::MIDI::SysExManager sysEx;
            sysEx.onPresetParsed = [&](const CZ101::State::Preset& p) {
                bankPresets.push_back(p);
            };

            sysEx.handleSysEx(mb.getData(), (int)mb.getSize(), f.getFileNameWithoutExtension());

            std::cout << "Found " << bankPresets.size() << " patches. Rendering audio ... " << std::flush;

            bool bankOk = true;
            for (const auto& p : bankPresets)
            {
                totalBankPatches++;
                processor.initializeSection(InitSection::ALL);
                processor.getPresetManager().loadPresetFromStruct(p, false, true);

                // Render 0.5s Note
                juce::AudioBuffer<float> buf(2, 22050);
                buf.clear();
                juce::MidiBuffer midi;
                midi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0);
                midi.addEvent(juce::MidiMessage::noteOff(1, 60, (juce::uint8)0), 11025);

                int pos = 0;
                while (pos < 22050)
                {
                    int todo = std::min(512, 22050 - pos);
                    juce::AudioBuffer<float> block(buf.getArrayOfWritePointers(), 2, pos, todo);
                    juce::MidiBuffer blockMidi;
                    blockMidi.addEvents(midi, pos, todo, -pos);
                    processor.processBlock(block, blockMidi);
                    pos += todo;
                }

                float peak = buf.getMagnitude(0, 22050);
                if (std::isnan(peak) || std::isinf(peak))
                {
                    bankOk = false;
                    std::cout << "\n  -> Patch '" << p.name << "' has NaN/Inf!" << std::flush;
                }
                else if (peak > 1.0f)
                {
                    bankOk = false;
                    std::cout << "\n  -> Patch '" << p.name << "' clipped! Peak=" << peak << std::flush;
                }
                else if (peak < 0.0001f)
                {
                    bankOk = false;
                    std::cout << "\n  -> Patch '" << p.name << "' is silent! Peak=" << peak << std::flush;
                }
                else
                {
                    passedBankPatches++;
                }
            }

            if (bankOk)
                std::cout << " [ALL OK]" << std::endl;
            else
                std::cout << " [ISSUES DETECTED]" << std::endl;
        }

        std::cout << "--------------------------------------------------------------------------------" << std::endl;
        std::cout << "SysEx Bank Scan Results: " << passedBankPatches << "/" << totalBankPatches << " patches rendered perfectly." << std::endl;
    }

    static void runAllTests(CZ101AudioProcessor& processor)
    {
        const double sampleRate = 44100.0;
        const int samplesPerBlock = 512;
        processor.prepareToPlay(sampleRate, samplesPerBlock);

        std::cout << "\n================================================================================" << std::endl;
        std::cout << "                 CZ-101 / CZ-5000 MASSIVE AUDIO ANALYZER                        " << std::endl;
        std::cout << "================================================================================" << std::endl;

        auto& pm = processor.getPresetManager();
        const auto& presets = pm.getPresets();
        int numPresets = (int)presets.size();

        std::cout << "Total Factory Presets in Catalog: " << numPresets << "\n" << std::endl;
        std::cout << std::left << std::setw(4)  << "ID"
                  << std::left << std::setw(24) << "Preset Name"
                  << std::right << std::setw(10) << "Peak(dB)"
                  << std::right << std::setw(10) << "RMS(dB)"
                  << std::right << std::setw(12) << "ChordPeak"
                  << std::right << std::setw(10) << "DC Offset"
                  << std::setw(14) << "Status"
                  << std::endl;
        std::cout << "--------------------------------------------------------------------------------" << std::endl;

        int totalPassed = 0;
        int totalWarnings = 0;
        int totalErrors = 0;
        std::vector<std::string> issueLog;

        for (int i = 0; i < numPresets; ++i)
        {
            const auto& pName = presets[i].name;
            auto m = analyzePreset(processor, i, pName);

            std::string status = "[OK]";
            bool isWarning = false;
            bool isError = false;

            if (m.hasNaNOrInf)
            {
                status = "[ERROR: NaN]";
                isError = true;
                issueLog.push_back(pName + ": Produced NaN/Inf in audio output!");
            }
            else if (m.chordPeakDb > 0.0f)
            {
                status = "[CLIPPING]";
                isWarning = true;
                issueLog.push_back(pName + ": Chord clipped! Peak = " + std::to_string(m.chordPeakDb) + " dBFS");
            }
            else if (m.peakDb < -50.0f)
            {
                status = "[SILENT]";
                isWarning = true;
                issueLog.push_back(pName + ": Preset is virtually silent (Peak = " + std::to_string(m.peakDb) + " dBFS)");
            }
            else if (m.maxReleaseStep > 0.4f)
            {
                status = "[REL CLICK]";
                isWarning = true;
                issueLog.push_back(pName + ": NoteOff abrupt step detected (" + std::to_string(m.maxReleaseStep) + ")");
            }
            else if (m.dcOffset > 0.05f)
            {
                status = "[DC OFFSET]";
                isWarning = true;
                issueLog.push_back(pName + ": High DC offset (" + std::to_string(m.dcOffset) + ")");
            }
            else if (!m.sysexMatch)
            {
                status = "[SYX MISMATCH]";
                isWarning = true;
                issueLog.push_back(pName + ": SysEx roundtrip mismatch: " + m.sysexMismatchReason);
            }

            if (isError) totalErrors++;
            else if (isWarning) totalWarnings++;
            else totalPassed++;

            std::cout << std::left << std::setw(4)  << i
                      << std::left << std::setw(24) << pName.substr(0, 23)
                      << std::right << std::setw(10) << std::fixed << std::setprecision(1) << m.peakDb
                      << std::right << std::setw(10) << std::fixed << std::setprecision(1) << m.rmsDb
                      << std::right << std::setw(12) << std::fixed << std::setprecision(1) << m.chordPeakDb
                      << std::right << std::setw(10) << std::fixed << std::setprecision(3) << m.dcOffset
                      << "  " << status
                      << std::endl;
        }

        std::cout << "================================================================================" << std::endl;
        std::cout << "SUMMARY: " << totalPassed << " PASSED | " << totalWarnings << " WARNINGS | " << totalErrors << " ERRORS" << std::endl;
        std::cout << "================================================================================" << std::endl;

        if (!issueLog.empty())
        {
            std::cout << "\nDetailed Diagnostic Issues:" << std::endl;
            for (const auto& issue : issueLog)
            {
                std::cout << "  - " << issue << std::endl;
            }
        }
        else
        {
            std::cout << "\nAll factory presets passed audio quality, headroom, and SysEx checks perfectly!" << std::endl;
        }

        // Run External Banks Test
        testSysExBanks(processor);
    }
};

// Main Entry Point
int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    CZ101AudioProcessor processor;
    MassiveAudioAnalyzer::runAllTests(processor);
    return 0;
}
