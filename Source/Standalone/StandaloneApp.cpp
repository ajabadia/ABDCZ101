#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "../PluginProcessor.h"

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
            setResizeLimits(400, 300, 10000, 10000);

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
        std::unique_ptr<juce::AudioProcessor> m_processor;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<juce::PropertiesFile> settings;
};

//==============================================================================
START_JUCE_APPLICATION(CZ101StandaloneApp)

