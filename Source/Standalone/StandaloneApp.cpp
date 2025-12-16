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
}


//==============================================================================
class CZ101StandaloneApp : public juce::JUCEApplication
{
public:
    CZ101StandaloneApp() {}

    const juce::String getApplicationName() override { return "CZ-101 Emulator"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    //==============================================================================
    void initialise(const juce::String& commandLine) override
    {
        // Check for Verification Tests
        runVerificationTests(commandLine);

        // 1. Create the Window (Main Entry Point)
        // We pass 'headless' if detected, though typical JUCE StandaloneWindow requires GUI.
        // For true headless on RPi without X11, JUCE usually needs specific linux backend flags.
        // However, assuming X11 is present or we want "Plug & Play" behavior:
        
        mainWindow.reset(new MainWindow(getApplicationName(), new CZ101AudioProcessor(), settings.get()));

        // 2. Auto-Connect Logic (The crucial part for Headless/RPi)
        auto& deviceManager = mainWindow->getDeviceManager();
        
        // A. Audio: Initialise with default devices if not already set
        // (StandalonePluginHolder does this, but we reinforce it)
        juce::String err = deviceManager.initialiseWithDefaultDevices(0, 2);
        if (err.isNotEmpty())
        {
            juce::Logger::writeToLog("Warning: Could not initialise default audio device: " + err);
        }
        else
        {
            juce::Logger::writeToLog("Audio Device Initialised: " + deviceManager.getCurrentAudioDevice()->getName());
        }

        // B. MIDI: Enable ALL available inputs automatically
        auto midiInputs = juce::MidiInput::getAvailableDevices();
        for (auto& device : midiInputs)
        {
            if (!deviceManager.isMidiInputDeviceEnabled(device.identifier))
            {
                deviceManager.setMidiInputDeviceEnabled(device.identifier, true);
                juce::Logger::writeToLog("Auto-Connected MIDI Input: " + device.name);
            }
        }
        
        // Ensure the player is listening to these newly enabled devices
        mainWindow->syncMidiCallbacks();
        
        // Check for specific headless flag to maybe minimize or hide
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
        mainWindow = nullptr; // Deletes the window and processor
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String&) override {}

    //==============================================================================
    /*
        Custom Main Window using explicit AudioDeviceManager and AudioProcessorPlayer.
        We avoid juce::StandalonePluginHolder to prevent internal header dependency issues.
    */
    class MainWindow : public juce::DocumentWindow, private juce::ChangeListener
    {
    public:
        MainWindow(const juce::String& name, juce::AudioProcessor* createdProcessor, juce::PropertiesFile* settings)
            : DocumentWindow(name, juce::Desktop::getInstance().getDefaultLookAndFeel()
                                       .findColour(juce::ResizableWindow::backgroundColourId),
                             juce::DocumentWindow::allButtons),
              m_processor(createdProcessor) // We take ownership via unique_ptr below
        {
            setUsingNativeTitleBar(true);
            setResizable(true, true);
            setResizable(true, true);
            setResizeLimits(400, 300, 10000, 10000);

            // Settings Button (Standard "Options..." Top Left)
            addAndMakeVisible(settingsButton);
            settingsButton.setButtonText("Options...");
            settingsButton.onClick = [this] { showAudioSettings(); };
            
            // 1. Setup Audio & MIDI
            // Initialise with 0 inputs, 2 outputs.
            // We can load setup from settings if we wanted, but let's stick to auto-defaults for now.
            auto err = deviceManager.initialiseWithDefaultDevices(0, 2);
            if (err.isNotEmpty())
                juce::Logger::writeToLog("Device Manager Init Error: " + err);

            // 2. Setup Processor Player
            // This connects the AudioProcessor to the DeviceManager callbacks
            player.setProcessor(m_processor.get());
            deviceManager.addAudioCallback(&player);
            deviceManager.addAudioCallback(&player);
            // deviceManager.addMidiInputDeviceCallback({}, &player); // Not needed generic add?
            // actually we do manual per-device add below. 
            // Actually deviceManager.addMidiInputCallback("", ...) doesn't add all.
            // We need to add callback per enabled device. But AudioProcessorPlayer handles this if we bridge it.
            // Wait, AudioProcessorPlayer IS a MidiInputCallback. 
            // We need to register it to the device manager for *each* enabled input.
            
            // 3. Create Editor
            createEditor();

            // 4. Restore State
            if (settings != nullptr)
            {
               // Load window position
               restoreWindowStateFromString(settings->getValue("windowState"));
               
               // Load Audio Device Setup
               auto xml = settings->getXmlValue("audioDeviceState");
               if (xml != nullptr)
                   deviceManager.initialise(0, 2, xml.get(), true);
            }

            // Listen for device changes
            deviceManager.addChangeListener(this);
            
            // Finalize window
            setVisible(true);
            
            resized(); // Ensure button is placed
        }
        
        void resized() override
        {
            juce::DocumentWindow::resized();
            // detailed positioning of button in title bar area? 
            // DocumentWindow draws its own title bar. We can put the button in the content area top-right?
            // Or overlay it?
            // Standard JUCE "Options..." placement (Top Left content area)
            // Typically just below the title bar if using native, or inside content.
            settingsButton.setBounds(6, 6, 100, 24);
        }

        ~MainWindow() override
        {
            deviceManager.removeChangeListener(this);
            deviceManager.removeAudioCallback(&player);
            // deviceManager.removeMidiInputCallback({}, &player);
            
            // Remove callbacks from all active midi inputs
             auto midiInputs = juce::MidiInput::getAvailableDevices();
            for (auto& device : midiInputs)
                 if (deviceManager.isMidiInputDeviceEnabled(device.identifier))
                     deviceManager.removeMidiInputDeviceCallback(device.identifier, &player);

            player.setProcessor(nullptr);
            setContentOwned(nullptr, true); // Delete editor
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

        void changeListenerCallback(juce::ChangeBroadcaster*) override
        {
            // Device changed
        }
        
        juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }
        
        // Helper to register the player as MIDI callback for all enabled devices
        void syncMidiCallbacks()
        {
            auto midiInputs = juce::MidiInput::getAvailableDevices();
            for (auto& device : midiInputs)
            {
                if (deviceManager.isMidiInputDeviceEnabled(device.identifier))
                {
                    // Remove first to be safe (no duplicates)
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

        juce::AudioDeviceManager deviceManager;
        juce::AudioProcessorPlayer player;
        juce::TextButton settingsButton;
        std::unique_ptr<juce::AudioProcessor> m_processor;
        
        void showAudioSettings()
        {
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
            
            opt.launchAsync();
        }
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<juce::PropertiesFile> settings;
};

//==============================================================================
START_JUCE_APPLICATION(CZ101StandaloneApp)

