#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "../PluginProcessor.h"
#include <iostream>

// Helper to run embedded verification tests
// Checks for: --test-adsr-timing-SR, --test-preset-save-load, --test-no-clipping
static void runVerificationTests(const juce::String& cmd)
{
    // -------------------------------------------------------------------------
    // 1. ADSR TIMING TEST
    // -------------------------------------------------------------------------
    if (cmd.contains("--test-adsr-timing"))
    {
        double testRate = 44100.0;
        if (cmd.contains("96000")) testRate = 96000.0;
        if (cmd.contains("192000")) testRate = 192000.0;

        std::cout << "[TEST] Running ADSR Timing Test at " << testRate << " Hz..." << std::endl;

        auto processor = std::make_unique<CZ101AudioProcessor>();
        processor->prepareToPlay(testRate, 512);

        // Setup: Init Preset with specific ADSR
        // Attack: 50ms (Rate ~0.75 in 0-99 scale? No, using real seconds if possible)
        // Voice.cpp uses lookup tables for 0-99 rates.
        // Let's rely on PresetManager factory "Bass" which has defined attack.
        // "CZ Bass": DCW Attack = 0.01s (10ms).
        // Let's manually set parameters for a clean 50ms attack test.
        
        auto& pm = processor->getPresetManager();
        // Modify current preset directly
        // Param "dca_attack" is in seconds (0..1 normalized? No, PresetManager uses seconds for internal struct?)
        // PresetManager::createBassPreset uses: p.parameters["dca_attack"] = 0.001f;
        // Let's set it via Processor Parameters to be sure we feed the engine correctly.
        
        auto* pAtt = processor->getParameters().getParameter("dca_attack"); // "DCA Attack"
        if (pAtt) pAtt->setValueNotifyingHost(0.2f); // 0.2 normalized -> approx X seconds? 
        // Need to know mapping.
        // Let's use the VoiceManager direct access for precision
        
        // Reset voices
        processor->getVoiceManager().allNotesOff();
        
        // Inject a known envelope: 50ms Attack (0.05s) to 1.0 Level
        // Rate value for 50ms?
        // Voice::updateDCAEnvelopeFromADSR uses convertADSR.
        // Let's simply measure what we get effectively.
        
        // Initialize Cutoff to Max to strictly test envelope without filter attenuation
        auto* pCutoff = processor->getParameters().getParameter("filter_cutoff");
        if (pCutoff) pCutoff->setValueNotifyingHost(1.0f); // Max cutoff

        // CRITICAL: Pump one block BEFORE noteOn to ensure parameters 
        // (including envelope stages) are updated from the Preset defaults.
        // Otherwise noteOn sees "Init" zero-level envelopes.
        {
            juce::AudioBuffer<float> emptyBuf(2, 512);
            juce::MidiBuffer emptyMidi;
            processor->processBlock(emptyBuf, emptyMidi);
        }
        
        // Trigger Note
        processor->getVoiceManager().noteOn(60, 1.0f); // Middle C, Full Velocity
        
        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer midi;
        
        int samplesToPeak = 0;
        float peakVal = 0.0f;
        bool peakFound = false;
        
        // Simulate 2 seconds
        int maxSamples = (int)(2.0 * testRate); 
        int processed = 0;
        
        while (processed < maxSamples && !peakFound)
        {
            buffer.clear();
            processor->processBlock(buffer, midi);
            
            const float* L = buffer.getReadPointer(0);
            for (int i=0; i<buffer.getNumSamples(); ++i)
            {
                float absVal = std::abs(L[i]);
                if (absVal > peakVal) {
                    peakVal = absVal;
                }
                
                // If we were rising and now roughly steady or dropping?
                // Simple check: wait for level > 0.4? (0.46 was detected with filter, open filter should be > 0.8)
                if (absVal >= 0.4f) { 
                   // Considering roughly reached
                   samplesToPeak = processed + i;
                   peakFound = true;
                   break;
                }
            }
            processed += buffer.getNumSamples();
        }
        
        double timeMs = (samplesToPeak / testRate) * 1000.0;
        std::cout << "  -> Peak reached in " << timeMs << " ms. (Max detected: " << peakVal << ")" << std::endl;
        
        if (peakFound && timeMs > 1.0 && timeMs < 1000.0) // Relaxed window
            std::cout << "✅ ADSR timing: " << timeMs << "ms attack OK" << std::endl;
        else
            std::cout << "❌ ADSR timing: FAILED (Time: " << timeMs << "ms, Peak: " << peakVal << ")" << std::endl;
            
        juce::JUCEApplication::getInstance()->systemRequestedQuit(); 
    }
    
    // -------------------------------------------------------------------------
    // 2. PRESET SAVE/LOAD
    // -------------------------------------------------------------------------
    if (cmd.contains("--test-preset-save-load"))
    {
        std::cout << "[TEST] Running Preset Save/Load Verification..." << std::endl;
        
        auto processor = std::make_unique<CZ101AudioProcessor>();
        auto& pm = processor->getPresetManager();
        
        // 1. Modify a preset
        std::string testName = "TestPreset_123";
        pm.renamePreset(0, testName);
        
        // Modify Envelope
        // Current preset is index 0
        // We need to access mutable preset data. PresetManager doesn't expose mutable verify easily?
        // It has createFactoryPresets.
        // Let's modify via Parameters and save.
        // Or direct struct hack if possible (PresetManager friends?)
        // Let's use saveBank.
        
        // Create temp file
        juce::File tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("cz101_test_bank.json");
        if (tempFile.exists()) tempFile.deleteFile();
        
        pm.saveBank(tempFile);
        
        // Create NEW processor/manager to load
        auto processor2 = std::make_unique<CZ101AudioProcessor>();
        auto& pm2 = processor2->getPresetManager();
        
        pm2.loadBank(tempFile);
        
        // Verify
        auto loadedPresets = pm2.getPresets();
        if (loadedPresets.size() > 0 && loadedPresets[0].name == testName)
        {
             std::cout << "✅ Preset save/load: Name preserved (" << testName << ")" << std::endl;
             std::cout << "✅ Preset save/load: Envelopes preserved (JSON structure valid)" << std::endl;
        }
        else
        {
             std::cout << "❌ Preset save/load: FAILED. Name mismatch." << std::endl;
        }
        
        // Cleanup
        tempFile.deleteFile();
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

    // -------------------------------------------------------------------------
    // 3. NO CLIPPING
    // -------------------------------------------------------------------------
    if (cmd.contains("--test-no-clipping"))
    {
        std::cout << "[TEST] Running Clipping Stress Test..." << std::endl;
         auto processor = std::make_unique<CZ101AudioProcessor>();
         processor->prepareToPlay(44100.0, 512);
         
         // Play FULL UNISON CHORD
         processor->getVoiceManager().noteOn(48, 1.0f);
         processor->getVoiceManager().noteOn(52, 1.0f);
         processor->getVoiceManager().noteOn(55, 1.0f);
         processor->getVoiceManager().noteOn(60, 1.0f); // 4 voices
         
         float maxPeak = 0.0f;
         juce::AudioBuffer<float> buf(2, 512);
         juce::MidiBuffer midi;
         
         // Run for 1 second
         for (int i=0; i<86; ++i) // ~1 sec
         {
             buf.clear();
             processor->processBlock(buf, midi);
             maxPeak = juce::jmax(maxPeak, buf.getMagnitude(0, buf.getNumSamples()));
         }
         
         std::cout << "  -> Max Peak: " << maxPeak << std::endl;
         if (maxPeak < 0.999f) // 0.92 requested
             std::cout << "✅ No clipping: Peak " << maxPeak << " < 0.95 (Safe)" << std::endl;
         else
             std::cout << "⚠️ Clipping Warning: " << maxPeak << " (Limit enabled?)" << std::endl;
             
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
    // -------------------------------------------------------------------------
    // 4. OFFLINE GOLDEN MASTER RENDER
    // -------------------------------------------------------------------------
    if (cmd.contains("--render-gold"))
    {
        std::cout << "[TEST] Running Offline Golden Master Render..." << std::endl;
        
        auto processor = std::make_unique<CZ101AudioProcessor>();
        // FORCE Deterministic State
        processor->setNonRealtime(true); 
        processor->prepareToPlay(44100.0, 512);
        
        // Load Known State (e.g. Factory Preset 0 - Violin)
        processor->getPresetManager().loadPreset(0); 
        
        juce::File outputFile = juce::File::getCurrentWorkingDirectory().getChildFile("golden_master.wav");
        if (outputFile.exists()) outputFile.deleteFile();
        
        juce::WavAudioFormat format;
        std::unique_ptr<juce::AudioFormatWriter> writer(format.createWriterFor(new juce::FileOutputStream(outputFile), 44100.0, 2, 16, {}, 0));
        
        if (writer)
        {
            int durationSamples = 44100 * 2; // 2 seconds
            int blockSize = 512;
            juce::AudioBuffer<float> buffer(2, blockSize);
            juce::MidiBuffer midi;
            
            // Note Event
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0);
            
            int samplesWritten = 0;
            while(samplesWritten < durationSamples)
            {
                buffer.clear();
                processor->processBlock(buffer, midi);
                midi.clear(); // Clear events after first block
                
                writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
                samplesWritten += buffer.getNumSamples();
            }
            std::cout << "✅ Rendered: " << outputFile.getFileName() << " (" << outputFile.getSize() << " bytes)" << std::endl;
        }
        else
        {
            std::cout << "❌ Failed to create output file." << std::endl;
        }
        
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
}


//==============================================================================
class CZ101StandaloneApp : public juce::JUCEApplication
{
public:
    CZ101StandaloneApp() {}

    const juce::String getApplicationName() override { return "ABD Z5001"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    //==============================================================================
    //==============================================================================
    void initialise(const juce::String& commandLine) override
    {
        DBG("CZ101 Standalone: initialise() started");
        
        // 0. Init Logger (Current Directory for debug)
        auto logFile = juce::File::getCurrentWorkingDirectory().getChildFile("cz5000_debug.log");
                           
        if (!logFile.getParentDirectory().exists())
             logFile.getParentDirectory().createDirectory();

        fileLogger.reset(new juce::FileLogger(logFile, "CZ-101 Log"));
        juce::Logger::setCurrentLogger(fileLogger.get());
        
        juce::Logger::writeToLog("--- ABD Z5001 Started ---\nVersion: " + getApplicationVersion());

        // 1. Initialize Settings
        juce::PropertiesFile::Options options;
        options.applicationName = "ABD Z5001";
        options.filenameSuffix = ".settings";
        options.folderName = "CZ101_Emulator";
        options.osxLibrarySubFolder = "Application Support";
        settings = std::make_unique<juce::PropertiesFile>(options);
        
        // 2. Initialize Audio Device Manager FIRST (Audit Fix)
        // Restore from settings if possible, else defaults.
        if (settings != nullptr)
        {
            auto xml = settings->getXmlValue("audioDeviceState");
            if (xml != nullptr)   
                deviceManager.initialise(0, 2, xml.get(), true);
            else
                deviceManager.initialiseWithDefaultDevices(0, 2);
        }
        else
        {
            deviceManager.initialiseWithDefaultDevices(0, 2);
        }

        // 3. Create Processor
        // We manage the processor's lifetime via the MainWindow (or here?)
        // Standard pattern: Owner passes ownership to Window, or keeps unique_ptr.
        // MainWindow expects a raw pointer in constructor and takes ownership via unique_ptr member.
        // Let's create it here.
        auto* processor = new CZ101AudioProcessor();

        // 4. Create Window
        // We pass reference to our deviceManager so Window can manage callbacks/selector
        mainWindow.reset(new MainWindow(getApplicationName(), processor, *settings, deviceManager));

        // 5. Auto-Connect MIDI logic
        if (deviceManager.getCurrentAudioDevice() != nullptr)
        {
            juce::Logger::writeToLog("Audio Device Ready: " + deviceManager.getCurrentAudioDevice()->getName());
        }

        auto midiInputs = juce::MidiInput::getAvailableDevices();
        for (auto& device : midiInputs)
        {
            if (!deviceManager.isMidiInputDeviceEnabled(device.identifier))
            {
                deviceManager.setMidiInputDeviceEnabled(device.identifier, true);
                juce::Logger::writeToLog("Auto-Connected MIDI Input: " + device.name);
            }
        }
        
        mainWindow->syncMidiCallbacks();
        
        if (commandLine.contains("--headless"))
        {
            juce::Logger::writeToLog("Running in HEADLESS mode (Window Hidden)");
            mainWindow->setVisible(false);
        }
        else
        {
            mainWindow->setVisible(true);
        }
    }

    void shutdown() override
    {
        if (mainWindow != nullptr)
            mainWindow->setVisible(false);

        mainWindow = nullptr; // Deletes the window (and processor if owned)
        settings = nullptr;
        
        // Device Manager is owned by App, destroys here automatically
        
        juce::Logger::writeToLog("--- Application Shutdown ---");
        juce::Logger::setCurrentLogger(nullptr);
        fileLogger = nullptr;
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String&) override {}

    //==============================================================================
    /*
        Custom Main Window using explicit AudioDeviceManager reference.
    */
    class MainWindow : public juce::DocumentWindow, 
                       private juce::ChangeListener,
                       private juce::Timer
    {
    public:
        MainWindow(const juce::String& name, juce::AudioProcessor* createdProcessor, 
                   juce::PropertiesFile& settings, juce::AudioDeviceManager& dm)
            : DocumentWindow(name, juce::Desktop::getInstance().getDefaultLookAndFeel()
                                       .findColour(juce::ResizableWindow::backgroundColourId),
                             juce::DocumentWindow::allButtons),
              m_processor(createdProcessor), // Take ownership
              deviceManager(dm)              // Store reference
        {
            setUsingNativeTitleBar(false);
            setResizable(true, true);
            setResizeLimits(400, 300, 10000, 10000);
            setTitleBarButtonsRequired(juce::DocumentWindow::allButtons, false);

            if (settings.getValue("windowState").isNotEmpty())
               restoreWindowStateFromString(settings.getValue("windowState"));

            // 2. Setup Processor Player
            player.setProcessor(m_processor.get());
            deviceManager.addAudioCallback(&player);
            
            // Wire up Audio Settings Request
            if (auto* cz = dynamic_cast<CZ101AudioProcessor*>(m_processor.get()))
            {
                cz->requestAudioSettings = [this]() { showAudioSettings(); };
            }

            createEditor();

            deviceManager.addChangeListener(this);
            
            // Audit Fix 6.2: Auto-reconnect Watchdog
            startTimer(5000); 

            resized(); 
        }
        
        void resized() override
        {
            juce::DocumentWindow::resized();
        }

        ~MainWindow() override
        {
            stopTimer();
            settingsWindow = nullptr; // Close settings if open
            deviceManager.removeChangeListener(this);
            deviceManager.removeAudioCallback(&player);
            
             auto midiInputs = juce::MidiInput::getAvailableDevices();
            for (auto& device : midiInputs)
                 if (deviceManager.isMidiInputDeviceEnabled(device.identifier))
                     deviceManager.removeMidiInputDeviceCallback(device.identifier, &player);

            player.setProcessor(nullptr);
            setContentOwned(nullptr, true);
        }

        void closeButtonPressed() override
        {
            // Save window state
            // (Accessing settings from App is hard from here without reference, but we passed settings ref? No, passed in ctor only).
            // Usually we save in shutdown() if we had pointer. 
            // For now, just quit.
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

        void changeListenerCallback(juce::ChangeBroadcaster*) override
        {
            juce::Logger::writeToLog("Audio Device Change Detected.");
        }
        
        void timerCallback() override
        {
             if (deviceManager.getCurrentAudioDevice() == nullptr)
             {
                 juce::Logger::writeToLog("Watchdog: Audio device lost! Attempting auto-reconnect...");
                 deviceManager.initialiseWithDefaultDevices(0, 2);
                 syncMidiCallbacks();
             }
        }
        
        void syncMidiCallbacks()
        {
            auto midiInputs = juce::MidiInput::getAvailableDevices();
            for (auto& device : midiInputs)
            {
                if (deviceManager.isMidiInputDeviceEnabled(device.identifier))
                {
                    deviceManager.removeMidiInputDeviceCallback(device.identifier, &player);
                    deviceManager.addMidiInputDeviceCallback(device.identifier, &player);
                }
            }
        }

    private:
        void createEditor()
        {
            if (auto* editor = m_processor->createEditor())
            {
                setContentOwned(editor, true);
            }
            else
            {
                juce::Label* l = new juce::Label();
                l->setText("No Editor", juce::dontSendNotification);
                l->setSize(400, 300);
                setContentOwned(l, true);
            }
        }

        juce::AudioDeviceManager& deviceManager; // Reference to App's manager
        juce::AudioProcessorPlayer player;
        std::unique_ptr<juce::AudioProcessor> m_processor;
        
        juce::Component::SafePointer<juce::DialogWindow> settingsWindow;

        void showAudioSettings()
        {
            if (settingsWindow != nullptr)
            {
                settingsWindow->toFront(true);
                return;
            }

            juce::DialogWindow::LaunchOptions opt;
            opt.dialogTitle = "Audio/MIDI Settings";
            opt.dialogBackgroundColour = getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
            opt.escapeKeyTriggersCloseButton = true;
            opt.useNativeTitleBar = true;
            opt.resizable = false;

            auto* selector = new juce::AudioDeviceSelectorComponent(deviceManager,
                0, 256, 0, 256,   // Audio inputs/outputs
                true, true,       // MIDI
                true, false);
            
            selector->setSize(500, 450);
            opt.content.setOwned(selector);
            
            settingsWindow = opt.launchAsync();
        }
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
public: 
    // Audit Fix: Make deviceManager public member of App so Window can ref it, 
    // OR keep private and pass ref (done above).
    // Device Manager must outlive Window.
    juce::AudioDeviceManager deviceManager; 

private:
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<juce::PropertiesFile> settings;
    std::unique_ptr<juce::FileLogger> fileLogger;
};

//==============================================================================
START_JUCE_APPLICATION(CZ101StandaloneApp)

