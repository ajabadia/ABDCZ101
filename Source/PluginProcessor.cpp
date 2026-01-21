#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "State/PresetManager.h"
#include "State/EnvelopeSerializer.h"
#include "DSP/Envelopes/ADSRtoStage.h" // [NEW] for Snapshot Builder // Required for unique_ptr destructor
#include "UI/LCDStateManager.h"

// --- CONSTRUCTOR ---
CZ101AudioProcessor::CZ101AudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      voiceManager(),
      undoManager(),
      parameters(*this, &undoManager),
      presetManager(&parameters, &voiceManager),
      midiProcessor(voiceManager, presetManager),
      sysExManager()
{
    DBG("CZ101 Processor: Constructor Start");
    // Setup File Logger
    auto logFile = juce::File::getCurrentWorkingDirectory().getChildFile("cz5000_debug.log");
                   
    fileLogger = std::make_unique<juce::FileLogger>(logFile, "CZ-5000 Emulator Log");
    if (juce::Logger::getCurrentLogger() == nullptr)
        juce::Logger::setCurrentLogger(fileLogger.get());
    juce::Logger::writeToLog("Logger initialized at: " + logFile.getFullPathName());
    
    // Bind SysEx Callback
    juce::Logger::writeToLog("Binding SysEx Callback...");
    sysExManager.onPresetParsed = [this](const CZ101::State::Preset& p) {
        // 1. Prepare POD for Audio Thread
        EnvelopeStatePOD pod;
        pod.pitchEnv = p.pitchEnv; pod.dcwEnv = p.dcwEnv; pod.dcaEnv = p.dcaEnv;
        pod.pitchEnv2 = p.pitchEnv2; pod.dcwEnv2 = p.dcwEnv2; pod.dcaEnv2 = p.dcaEnv2;
        
        int s1, sz1, s2, sz2;
        presetFifo.prepareToWrite(1, s1, sz1, s2, sz2);
        if (sz1 > 0) presetBuffer[s1] = pod;
        else if (sz2 > 0) presetBuffer[s2] = pod;
        presetFifo.finishedWrite(sz1 + sz2);
        
        // 2. Defer Parameter/Host notification to Message Thread
        {
            const juce::ScopedLock sl(sysExLock);
            pendingSysExPreset = std::make_unique<CZ101::State::Preset>(p);
            hasPendingSysEx = true;
        }
        triggerAsyncUpdate();
    };
    
    juce::Logger::writeToLog("Setting SysEx Manager...");
    midiProcessor.setSysExManager(&sysExManager);
    midiProcessor.setAPVTS(&parameters.getAPVTS());
    
    // Audit Fix 10.1: Bind Lock-free MIDI Param Callback
    midiProcessor.setParamChangeCallback([this](const char* id, float val) {
        scheduleMidiParamUpdate(id, val);
    });
    
    // Initialize LCD State Manager here to ensure it persists and avoids dangling references
    juce::Logger::writeToLog("CZ101 Processor: Initializing LCD State Manager");
    lcdStateManager = std::make_unique<CZ101::UI::LCDStateManager>(parameters.getAPVTS());

    // Audit Fix [D]: Reconnect UI and DSP
    // Register this processor as a listener for all parameters to trigger snapshot updates
    for (auto* p : juce::AudioProcessor::getParameters()) {
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p)) {
            parameters.getAPVTS().addParameterListener(rp->getParameterID(), this);
        }
    }

    juce::Logger::writeToLog("CZ101 Processor: Constructor End");
}

CZ101AudioProcessor::~CZ101AudioProcessor() 
{ 
    // Unregister listeners
    for (auto* p : juce::AudioProcessor::getParameters()) {
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p)) {
            parameters.getAPVTS().removeParameterListener(rp->getParameterID(), this);
        }
    }
    juce::Logger::setCurrentLogger(nullptr); 
}

// --- APPLY PRESET ENVELOPES (Lock-Free) ---
void CZ101AudioProcessor::applyPresetEnvelopes(const EnvelopeStatePOD& pod)
{
    // Envelopes: These update the VoiceManager directly as they are not mapped to APVTS parameters for performance reasons
    // Line 1
    for (int i = 0; i < 8; ++i) {
        voiceManager.setPitchStage(1, i, pod.pitchEnv.rates[i], pod.pitchEnv.levels[i]);
        voiceManager.setDCWStage(1, i, pod.dcwEnv.rates[i], pod.dcwEnv.levels[i]);
        voiceManager.setDCAStage(1, i, pod.dcaEnv.rates[i], pod.dcaEnv.levels[i]);
    }
    voiceManager.setPitchSustainPoint(1, pod.pitchEnv.sustainPoint);
    voiceManager.setPitchEndPoint(1, pod.pitchEnv.endPoint);
    voiceManager.setDCWSustainPoint(1, pod.dcwEnv.sustainPoint);
    voiceManager.setDCWEndPoint(1, pod.dcwEnv.endPoint);
    voiceManager.setDCASustainPoint(1, pod.dcaEnv.sustainPoint);
    voiceManager.setDCAEndPoint(1, pod.dcaEnv.endPoint);

    // Line 2
    for (int i = 0; i < 8; ++i) {
        voiceManager.setPitchStage(2, i, pod.pitchEnv2.rates[i], pod.pitchEnv2.levels[i]);
        voiceManager.setDCWStage(2, i, pod.dcwEnv2.rates[i], pod.dcwEnv2.levels[i]);
        voiceManager.setDCAStage(2, i, pod.dcaEnv2.rates[i], pod.dcaEnv2.levels[i]);
    }
    voiceManager.setPitchSustainPoint(2, pod.pitchEnv2.sustainPoint);
    voiceManager.setPitchEndPoint(2, pod.pitchEnv2.endPoint);
    voiceManager.setDCWSustainPoint(2, pod.dcwEnv2.sustainPoint);
    voiceManager.setDCWEndPoint(2, pod.dcwEnv2.endPoint);
    voiceManager.setDCASustainPoint(2, pod.dcaEnv2.sustainPoint);
    voiceManager.setDCAEndPoint(2, pod.dcaEnv2.endPoint);
}

// --- BASIC PLUGIN INFO ---
const juce::String CZ101AudioProcessor::getName() const { return JucePlugin_Name; }
bool CZ101AudioProcessor::acceptsMidi() const { return true; }
bool CZ101AudioProcessor::producesMidi() const { return false; }
bool CZ101AudioProcessor::isMidiEffect() const { return false; }
// Audit Fix 2.2: Return max reasonable tail (CZ-101 envelope max).
// Audit Fix 2.2: Return max reasonable tail (CZ-101 envelope max).
double CZ101AudioProcessor::getTailLengthSeconds() const { return 8.0; } 
int CZ101AudioProcessor::getNumPrograms() { return presetManager.getNumPresets(); }
int CZ101AudioProcessor::getCurrentProgram() { return presetManager.getCurrentPresetIndex(); }
void CZ101AudioProcessor::setCurrentProgram(int index) 
{ 
    if (index != getCurrentProgram())
        presetManager.loadPreset(index); 
}
const juce::String CZ101AudioProcessor::getProgramName(int index) 
{ 
    return presetManager.getPresetName(index);
}
void CZ101AudioProcessor::changeProgramName(int index, const juce::String& newName) 
{ 
    presetManager.renamePreset(index, newName.toStdString()); 
}

// --- EDITOR ---
bool CZ101AudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* CZ101AudioProcessor::createEditor() 
{ 
    return new CZ101AudioProcessorEditor(*this); 
}

// --- PREPARE TO PLAY ---
void CZ101AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Guard against invalid sample rate (prevents DSP module crashes)
    if (sampleRate <= 0.0 || samplesPerBlock <= 0) {
        juce::Logger::writeToLog("CZ101Processor: Invalid audio config - sampleRate=" + juce::String(sampleRate) + ", samplesPerBlock=" + juce::String(samplesPerBlock));
        return;
    }
    
    currentSampleRate.store(sampleRate);
    
    // Audit Fix 4.1: FIFO resizing
    voiceManager.setSampleRate(sampleRate);
 
    // Audit Fix: Initialize Vis Buffer (Triple Buffer is std::array, no resize needed)
    // visBuffer.setSize(1, VIS_FIFO_SIZE);
    // visBuffer.clear();
    
    juce::File presetsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("CZ101Emulator");
    if (!presetsDir.exists()) presetsDir.createDirectory();
    juce::File bankFile = presetsDir.getChildFile(CZ101::State::PresetManager::USER_BANK_FILENAME);
    
    juce::Logger::writeToLog("CZ101Processor: prepareToPlay Start");
    if (bankFile.existsAsFile()) {
        juce::Logger::writeToLog("CZ101Processor: Loading Bank File: " + bankFile.getFullPathName());
        presetManager.loadBank(bankFile);
        juce::Logger::writeToLog("CZ101Processor: Bank Loaded");
        if (presetManager.getPresets().empty()) {
            juce::Logger::writeToLog("CZ101Processor: Bank Empty, Creating Factory Presets");
            presetManager.createFactoryPresets();
        }
    } else {
        juce::Logger::writeToLog("CZ101Processor: Creating Factory Presets");
        presetManager.createFactoryPresets();
        presetManager.saveBank(bankFile);
    }
    
    juce::Logger::writeToLog("CZ101Processor: Initializing Preset 0");
    presetManager.loadPreset(0);
    // Modern Filters Setup
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 2; // Stereo
    
    effectsChain.prepare(spec);
    
    // Audit Fix 3.1: Initialize Latency
    updateParameters();
}

void CZ101AudioProcessor::releaseResources() {}

bool CZ101AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

// --- PROCESS BLOCK ---
void CZ101AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
// ... (continuation of processBlock)
    juce::ScopedNoDenormals noDenormals;

#if JUCE_ARM
    #if __ARM_FP
        uint32_t fpscr;
        __asm__ volatile ("vmrs %0, fpscr" : "=r" (fpscr));
        fpscr |= (1 << 24); // Flush-to-zero
        __asm__ volatile ("vmsr fpscr, %0" : : "r" (fpscr));
    #endif
#endif

    performanceMonitor.startMeasurement();
    
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i) 
        buffer.clear(i, 0, buffer.getNumSamples());
    
    // Audit Fix [D]: Using LOCK-FREE Snapshot System
    // 1. Process Message Queue (Envelopes) - Still separate for now, or could be in snapshot?
    // Envelopes are "Events", Snapshot is "State". Keep Events separate.
    processEnvelopeUpdates();

    // 2. Process SysEx Presets (if any pending swap) logic is handled in handleAsyncUpdate mostly, 
    // but pendingSysExPreset logic was for message thread.
    // Here we might have a simple atomic swap if we used lock-free queue for whole presets.
    // For now, handleAsyncUpdate handles the Heavy Load, and we just read the resulting Snapshot.
    
    // 3. Get LATEST Snapshot
    const auto* snapshot = audioSnapshot.get();
    
    // Optimized Bypass Path
    // Bypass is not in Snapshot yet (Juice Param). 
    // We should probably rely on the Param directly for Bypass or put it in Snapshot.
    // For safety, let's use the param directly as it's an Atomic wrapper usually?
    // Parameters access in Audio Thread is SAFE if they are Atomics (most JUCE params are).
    // But `getBypass()->get()` is safe.
    if (parameters.getBypass() && parameters.getBypass()->get())
    {
        midiProcessor.processMidiBuffer(midiMessages);
        performanceMonitor.stopMeasurement();
        return;
    }

    // 4. Apply Snapshot to Voice Manager
    if (snapshot)
    {
        voiceManager.applySnapshot(snapshot);
        voiceManager.getArpeggiator().setEnabled(snapshot->arp.enabled); // Redundant? applySnapshot does it.
    }
    
    midiProcessor.processMidiBuffer(midiMessages);
    
    auto* channelDataL = buffer.getWritePointer(0);
    auto* channelDataR = buffer.getWritePointer(1);
    
    // Render synth audio ONCE
    voiceManager.renderNextBlock(channelDataL, channelDataR, buffer.getNumSamples());

    // 5. Effects Processing
    if (snapshot)
    {
         effectsChain.process(buffer, *snapshot);
    }
    
    // Visualization logic - Triple Buffer Producer
    // 1. Write to Back Buffer (Owned by Audio Thread)
    int back = visTripleBuffer.backIndex.load(std::memory_order_relaxed);
    auto& backBuf = visTripleBuffer.buffers[back];
    
    // Copy current block (limit to buffer size)
    int numSamples = std::min(buffer.getNumSamples(), (int)visTripleBuffer.SIZE);
    juce::FloatVectorOperations::copy(backBuf.data(), buffer.getReadPointer(0), numSamples);
    // Zero the rest if needed? (Optional, if WaveformDisplay uses numSamples known? No, it uses 256 internal)
    // For now, simple copy.
    
    // 2. Publish: Swap Back with Mid
    int mid = visTripleBuffer.midIndex.exchange(back, std::memory_order_acq_rel);
    visTripleBuffer.backIndex.store(mid, std::memory_order_relaxed);
    visTripleBuffer.hasNewData.store(true, std::memory_order_release);
    
    performanceMonitor.stopMeasurement();
}

// --- UPDATE PARAMETERS (Centralized Logic) ---
void CZ101AudioProcessor::updateParameters()
{
    // Audit Fix 4.1: Refactored Monolithic Method into Helpers
    auto macros = calculateMacros();
    
    updateFilters(macros);
    updateOscillators(macros);
    updateEnvelopes(macros);
    updateLFO();
    updateModMatrix();
    updateEffects(macros);
    updateSystemGlobal();
    updateArpeggiator();
    
    // Centralized Snapshot Creation (Optimization: One allocation per update)
    auto snap = buildAudioSnapshot();
    audioSnapshot.commit(std::move(snap));
}

// Audit Fix 2.1: Implement setNonRealtime to recalculate smoothing
void CZ101AudioProcessor::setNonRealtime(bool isNonRealtime) noexcept
{
    juce::AudioProcessor::setNonRealtime(isNonRealtime);
    
    // Force recalculation of smoothing steps for all parameters
    // This ensures that offline bounces use the correct smoothing regardless of the host's block size settings
    voiceManager.setSampleRate(getSampleRate());
    updateParameters();
}

juce::AudioParameterBool* CZ101AudioProcessor::getBypassParameter() const
{
    return parameters.getBypass();
}

// Duplicate buildAudioSnapshot removed
// Phase 7: Snapshot Builder Implementation
    
// Orphaned code removed

// --- THREAD-SAFE ENVELOPE QUEUE ---
void CZ101AudioProcessor::scheduleEnvelopeUpdate(const EnvelopeUpdateCommand& cmd)
{
    int start1, size1, start2, size2;
    
    // Audit Fix 1.1: Verify free space before writing to prevent overflow/crash
    if (commandFifo.getFreeSpace() < 1) {
        // juce::Logger::writeToLog("Warning: Command FIFO full, dropping envelope update");
        return;
    }

    commandFifo.prepareToWrite(1, start1, size1, start2, size2);
    if (size1 > 0) commandBuffer[start1] = cmd;
    else if (size2 > 0) commandBuffer[start2] = cmd;
    commandFifo.finishedWrite(size1 + size2);
}

void CZ101AudioProcessor::processEnvelopeUpdates()
{
    int start1, size1, start2, size2;
    commandFifo.prepareToRead(100, start1, size1, start2, size2);
    
    // Audit Fix 3: De-duplicate logic with lambda
    auto processRange = [&](int start, int size) {
        for (int i = 0; i < size; ++i) {
            auto& cmd = commandBuffer[start + i];
            switch (cmd.type) {
                case EnvelopeUpdateCommand::DCA_STAGE: voiceManager.setDCAStage(cmd.line, cmd.index, cmd.rate, cmd.level); break;
                case EnvelopeUpdateCommand::DCA_SUSTAIN: voiceManager.setDCASustainPoint(cmd.line, cmd.index); break;
                case EnvelopeUpdateCommand::DCA_END: voiceManager.setDCAEndPoint(cmd.line, cmd.index); break;
                case EnvelopeUpdateCommand::DCW_STAGE: voiceManager.setDCWStage(cmd.line, cmd.index, cmd.rate, cmd.level); break;
                case EnvelopeUpdateCommand::DCW_SUSTAIN: voiceManager.setDCWSustainPoint(cmd.line, cmd.index); break;
                case EnvelopeUpdateCommand::DCW_END: voiceManager.setDCWEndPoint(cmd.line, cmd.index); break;
                case EnvelopeUpdateCommand::PITCH_STAGE: voiceManager.setPitchStage(cmd.line, cmd.index, cmd.rate, cmd.level); break;
                case EnvelopeUpdateCommand::PITCH_SUSTAIN: voiceManager.setPitchSustainPoint(cmd.line, cmd.index); break;
                case EnvelopeUpdateCommand::PITCH_END: voiceManager.setPitchEndPoint(cmd.line, cmd.index); break;
            }
        }
    };

    if (size1 > 0) processRange(start1, size1);
    if (size2 > 0) processRange(start2, size2);
    commandFifo.finishedRead(size1 + size2);
}

// --- PERSISTENCE ---
// --- PERSISTENCE ---
void CZ101AudioProcessor::getStateInformation(juce::MemoryBlock& destData) 
{
    // 1. Capture full envelope state from VoiceManager/UI
    presetManager.copyStateFromProcessor();
    
    // 2. Create Root XML
    juce::XmlElement root("PluginState");
    root.setAttribute("version", "1.0.0");
    
    // 3. APVTS State
    auto state = parameters.getAPVTS().copyState();
    if (auto stateXml = std::unique_ptr<juce::XmlElement>(state.createXml()))
        root.addChildElement(stateXml.release());
        
    // 4. Detailed Envelope State
    if (auto envXml = presetManager.exportEnvelopesToXml())
        root.addChildElement(envXml.release());
        
    copyXmlToBinary(root, destData);
}

void CZ101AudioProcessor::setStateInformation(const void* data, int sizeInBytes) 
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
    {
        // Check for new Container format
        if (xmlState->hasTagName("PluginState"))
        {
            // Load APVTS
            if (auto* paramsXml = xmlState->getChildElement(0)) // Assuming first child is APVTS or scan
            {
                // Better: Iterate
                for (auto* child : xmlState->getChildIterator()) {
                     if (child->hasTagName(parameters.getAPVTS().state.getType())) {
                         parameters.getAPVTS().replaceState(juce::ValueTree::fromXml(*child));
                     } else if (child->hasTagName("Envelopes")) {
                         presetManager.importEnvelopesFromXml(*child);
                     }
                }
            }
        }
        else if (xmlState->hasTagName(parameters.getAPVTS().state.getType()))
        {
            // Legacy fall-back
            parameters.getAPVTS().replaceState(juce::ValueTree::fromXml(*xmlState));
            // In legacy, we assume envelopes are default or partial.
            // But we should try to reload current preset logic if possible?
            // Legacy saves only params, so envelopes might be reset to default Init unless we handle it.
            // For now, acceptable.
        }
    }
}

// --- AUTHENTIC INITIALIZE ---
void CZ101AudioProcessor::initializeSection(InitSection section)
{
    if (section == InitSection::WAVEFORM || section == InitSection::ALL) {
        if (parameters.getOsc1Waveform()) parameters.getOsc1Waveform()->setValueNotifyingHost(0.0f);
        if (parameters.getOsc1Waveform2()) parameters.getOsc1Waveform2()->setValueNotifyingHost(0.0f);
        if (parameters.getOsc2Waveform()) parameters.getOsc2Waveform()->setValueNotifyingHost(0.0f);
        if (parameters.getOsc2Waveform2()) parameters.getOsc2Waveform2()->setValueNotifyingHost(0.0f);
    }

    if (section == InitSection::VIBRATO || section == InitSection::ALL) {
        if (parameters.getLfoRate()) parameters.getLfoRate()->setValueNotifyingHost(0.0f);
        if (parameters.getLfoDepth()) parameters.getLfoDepth()->setValueNotifyingHost(0.0f);
        if (parameters.getLfoDelay()) parameters.getLfoDelay()->setValueNotifyingHost(0.0f);
        if (parameters.getLfoWaveform()) parameters.getLfoWaveform()->setValueNotifyingHost(0.0f);
    }
    
    if (section == InitSection::DCA || section == InitSection::DCW || section == InitSection::ALL) {
        bool doDCW = (section == InitSection::DCW || section == InitSection::ALL);
        bool doDCA = (section == InitSection::DCA || section == InitSection::ALL);
        
        if (doDCW) {
             scheduleEnvelopeUpdate(EnvelopeUpdateCommand { EnvelopeUpdateCommand::DCW_STAGE, 1, 0, 99.0f, 1.0f });
             scheduleEnvelopeUpdate(EnvelopeUpdateCommand { EnvelopeUpdateCommand::DCW_STAGE, 1, 1, 99.0f, 0.0f });
             scheduleEnvelopeUpdate(EnvelopeUpdateCommand { EnvelopeUpdateCommand::DCW_SUSTAIN, 1, 0, 0.0f, 0.0f });
             scheduleEnvelopeUpdate(EnvelopeUpdateCommand { EnvelopeUpdateCommand::DCW_END, 1, 1, 0.0f, 0.0f });
        }
        
        if (doDCA) {
             scheduleEnvelopeUpdate(EnvelopeUpdateCommand { EnvelopeUpdateCommand::DCA_STAGE, 1, 0, 99.0f, 1.0f });
             scheduleEnvelopeUpdate(EnvelopeUpdateCommand { EnvelopeUpdateCommand::DCA_STAGE, 1, 1, 99.0f, 0.0f });
             scheduleEnvelopeUpdate(EnvelopeUpdateCommand { EnvelopeUpdateCommand::DCA_SUSTAIN, 1, 0, 0.0f, 0.0f });
             scheduleEnvelopeUpdate(EnvelopeUpdateCommand { EnvelopeUpdateCommand::DCA_END, 1, 1, 0.0f, 0.0f });
        }
    }
    
    if (section == InitSection::DCO || section == InitSection::ALL) {
        scheduleEnvelopeUpdate(EnvelopeUpdateCommand { EnvelopeUpdateCommand::PITCH_STAGE, 1, 0, 50.0f, 0.5f });
        scheduleEnvelopeUpdate(EnvelopeUpdateCommand { EnvelopeUpdateCommand::PITCH_STAGE, 1, 1, 50.0f, 0.5f });
        scheduleEnvelopeUpdate(EnvelopeUpdateCommand { EnvelopeUpdateCommand::PITCH_SUSTAIN, 1, 0, 0.0f, 0.0f });
        scheduleEnvelopeUpdate(EnvelopeUpdateCommand { EnvelopeUpdateCommand::PITCH_END, 1, 1, 0.0f, 0.0f });
    }
    
    if (section == InitSection::OCTAVE || section == InitSection::ALL) {
        if (parameters.getOsc2Detune()) parameters.getOsc2Detune()->setValueNotifyingHost(parameters.getOsc2Detune()->convertTo0to1(0.0f));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new CZ101AudioProcessor(); }
// Audit Fix 4.2: Handle SysEx parameter updates on Message Thread
void CZ101AudioProcessor::handleAsyncUpdate()
{
    std::unique_ptr<CZ101::State::Preset> p;
    
    // Check flag first efficiently
    if (hasPendingSysEx.load())
    {
        {
            const juce::ScopedLock sl(sysExLock);
            p = std::move(pendingSysExPreset);
            hasPendingSysEx = false;
        }

    if (p)
        {
            // Update Parameters and notify host (Message Thread)
            // We pass false to updateVoice because we already updated it in the callback
            presetManager.loadPresetFromStruct(*p, false);
        }
    }
    
    // Audit Fix 10.1: Process MIDI Parameter Queue
    int mStart1, mSize1, mStart2, mSize2;
    midiParamFifo.prepareToRead(100, mStart1, mSize1, mStart2, mSize2);
    auto processMidiParams = [&](int start, int size) {
        for (int i = 0; i < size; ++i) {
             const auto& update = midiParamBuffer[start + i];
             // Apply to APVTS via Message Thread (Safe)
             if (auto* p = parameters.getAPVTS().getParameter(update.paramID)) {
                 p->setValueNotifyingHost(update.value);
             }
        }
    };
    if (mSize1 > 0) processMidiParams(mStart1, mSize1);
    if (mSize2 > 0) processMidiParams(mStart2, mSize2);
    midiParamFifo.finishedRead(mSize1 + mSize2);

    // Audit Fix [D]: Rebuild Snapshot after any parameter change (SysEx, MIDI, or UI)
    updateParameters();
}

// Audit Fix 10.1: Lock-Free MIDI Scheduler
void CZ101AudioProcessor::scheduleMidiParamUpdate(const char* id, float value)
{
    int start1, size1, start2, size2;
    if (midiParamFifo.getFreeSpace() < 1) return; // Drop if full
    
    midiParamFifo.prepareToWrite(1, start1, size1, start2, size2);
    if (size1 > 0) {
        juce::String(id).copyToUTF8(midiParamBuffer[start1].paramID, 64);
        midiParamBuffer[start1].value = value;
    } else if (size2 > 0) {
        juce::String(id).copyToUTF8(midiParamBuffer[start2].paramID, 64);
        midiParamBuffer[start2].value = value;
    }
    midiParamFifo.finishedWrite(size1 + size2);
    triggerAsyncUpdate();
}

// APVTS Listener Implementation
void CZ101AudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    // Any parameter change in the UI or host automation triggers a snapshot rebuild
    // We use AsyncUpdater to avoid rebuilding the snapshot multiple times in a single block
    // or when multiple parameters change almost simultaneously.
    triggerAsyncUpdate();
}

// Audit Fix 10.6: Compare Logic
void CZ101AudioProcessor::toggleCompareMode(bool enable)
{
    if (isCompareEnabled == enable) return;
    isCompareEnabled = enable;
    
    // We delegate the buffer swapping logic to PresetManager
    // But since PresetManager doesn't have a formal Compare function yet (per plan), we implement it via backup/restore concept here or add it to PM.
    // Simpler: Just ask PM to toggle compare state.
    presetManager.setCompareMode(isCompareEnabled);
    
    // Refresh UI
    presetManager.sendChangeMessage(); // Notify Editor to refresh sliders
}

// --- REFACTORED UPDATERS (Audit Fix 4.1) ---
CZ101AudioProcessor::MacroValues CZ101AudioProcessor::calculateMacros()
{
    MacroValues m;
    float macroBrilliance = parameters.getMacroBrilliance() ? parameters.getMacroBrilliance()->get() : 0.5f;
    float macroTone = parameters.getMacroTone() ? parameters.getMacroTone()->get() : 0.5f;
    float macroSpace = parameters.getMacroSpace() ? parameters.getMacroSpace()->get() : 0.0f;

    // Tone Modifiers (0.5 = No change, < 0.5 = Slower, > 0.5 = Faster)
    m.toneSpeedMult = std::pow(2.0f, (macroTone - 0.5f) * 2.0f); 
    // Brilliance Modifiers (> 0.5 = Brighter, < 0.5 = Darker)
    m.brillianceOffset = (macroBrilliance - 0.5f) * 2000.0f;
    // Space Modifiers (Additive Mix)
    m.spaceMix = macroSpace * 0.5f;
    
    return m;
}

void CZ101AudioProcessor::updateFilters(const MacroValues& m)
{
    if (parameters.getModernLpfCutoff()) {
        float baseCutoff = parameters.getModernLpfCutoff()->get();
        // Voice Filters use Macro Brilliance (Output LPF handled in EffectsChain via Snapshot)
        float finalCutoff = juce::jlimit(20.0f, 20000.0f, baseCutoff + m.brillianceOffset);
        voiceManager.setFilterCutoff(finalCutoff);
    }
    if (parameters.getModernLpfReso()) {
        float res = parameters.getModernLpfReso()->get();
        voiceManager.setFilterResonance(res);
    }
    if (parameters.getModernHpfCutoff()) {
        float hpf = parameters.getModernHpfCutoff()->get();
        voiceManager.setHPF(hpf);
    }
}

void CZ101AudioProcessor::updateOscillators(const MacroValues& m)
{
    // Snapshot logic moved to updateParameters
}

void CZ101AudioProcessor::updateEnvelopes(const MacroValues& m)
{
    // Snapshot logic moved to updateParameters
}

void CZ101AudioProcessor::updateLFO()
{
    // Snapshot logic moved to updateParameters
}

void CZ101AudioProcessor::updateModMatrix()
{
    // Snapshot logic moved to updateParameters
}

void CZ101AudioProcessor::updateEffects(const MacroValues& m)
{
    // Snapshot logic moved to updateParameters
    // Latency calculation logic is now handled in updateParameters or via parameter lookup
    
    // Calculate Latency (Message Thread logic)
    // We can access parameters directly since we are on the Message Thread (or Async update)
    float cMix = parameters.getChorusMix() ? parameters.getChorusMix()->get() : 0.0f;
    float dMix = parameters.getDelayMix() ? parameters.getDelayMix()->get() : 0.0f;
    float dTime = parameters.getDelayTime() ? parameters.getDelayTime()->get() : 0.25f;
    
    int chorusDelaySamples = (cMix > 0.0f) ? (int)(0.025 * getSampleRate()) : 0;
    int delaySamples = (dMix > 0.0f) ? (int)(dTime * getSampleRate()) : 0;
    int latency = chorusDelaySamples + (delaySamples > 0 ? 1 : 0); 
    if (getLatencySamples() != latency) setLatencySamples(latency);
}

void CZ101AudioProcessor::updateSystemGlobal()
{
    // Snapshot logic moved to updateParameters
}

void CZ101AudioProcessor::updateArpeggiator()
{
    // Snapshot logic moved to updateParameters
}

// Phase 7: Snapshot Builder Implementation
std::unique_ptr<CZ101::Core::ParameterSnapshot> CZ101AudioProcessor::buildAudioSnapshot()
{
    auto snap = std::make_unique<CZ101::Core::ParameterSnapshot>();
    
    // Helper lambda for safe param access
    auto getVal = [](juce::AudioParameterFloat* p, float def = 0.0f) { return p ? p->get() : def; };
    auto getInt = [](juce::AudioParameterChoice* p, int def = 0) { return p ? p->getIndex() : def; };
    auto getIntParam = [](juce::AudioParameterInt* p, int def = 0) { return p ? p->get() : def; };
    auto getBool = [](juce::AudioParameterBool* p, bool def = false) { return p ? p->get() : def; };

    // DCO 1
    snap->dco1.wave1 = getInt(parameters.getOsc1Waveform());
    snap->dco1.wave2 = parameters.getOsc1Waveform2() ? (getInt(parameters.getOsc1Waveform2()) == 0 ? 8 : getInt(parameters.getOsc1Waveform2()) - 1) : 8;
    snap->dco1.level = getVal(parameters.getOsc1Level());

    // DCO 2
    snap->dco2.wave1 = getInt(parameters.getOsc2Waveform());
    snap->dco2.wave2 = parameters.getOsc2Waveform2() ? (getInt(parameters.getOsc2Waveform2()) == 0 ? 8 : getInt(parameters.getOsc2Waveform2()) - 1) : 8;
    snap->dco2.level = getVal(parameters.getOsc2Level());
    snap->dco2.octave = getIntParam(parameters.getDetuneOctave()); 
    snap->dco2.coarse = getIntParam(parameters.getDetuneCoarse());
    snap->dco2.fine = getIntParam(parameters.getDetuneFine());

    // 1+1 Logic (Line Select = 2)
    int lineSel = parameters.getLineSelect() ? getInt(parameters.getLineSelect()) : 2;
    if (lineSel == 2) {
         snap->dco2.wave1 = snap->dco1.wave1;
         snap->dco2.wave2 = snap->dco1.wave2;
         snap->dco2.level = snap->dco1.level;
    }
    if (lineSel == 0) snap->dco2.level = 0.0f;
    if (lineSel == 1) snap->dco1.level = 0.0f;

    // Line Mod
    snap->lineMod.ring = getBool(parameters.getRingMod());
    snap->lineMod.noise = parameters.getNoiseMod() ? getBool(parameters.getNoiseMod()) : false;

    // System
    snap->system.masterVol = getVal(parameters.getMasterVolume(), 1.0f);
    snap->system.masterTune = 0.0f; 
    snap->system.bendRange = 2.0f; 
    
    // Op Mode & Limits
    snap->system.opMode = getInt(parameters.getOperationMode());
    snap->system.voiceLimit = (snap->system.opMode == 2) ? 16 : (snap->system.opMode == 0 ? 4 : 8);
    snap->system.hardwareNoise = getBool(parameters.getHardwareNoise()); // Audit Fix: Corrected name
    snap->system.oversampling = getInt(parameters.getOversamplingQuality()); 
    snap->system.oversampling = (snap->system.oversampling == 0) ? 1 : (snap->system.oversampling == 1 ? 2 : 4);
    
    if (parameters.getMidiChannel()) snap->system.midiChannel = getIntParam(parameters.getMidiChannel());

    // Mod Matrix
    snap->mod.veloDcw = getVal(parameters.getModVeloToDcw());
    snap->mod.veloAmp = getVal(parameters.getModVeloToDca());
    snap->mod.wheelToDcw = getVal(parameters.getModWheelToDcw());
    snap->mod.wheelToLfoRate = getVal(parameters.getModWheelToLfoRate());
    snap->mod.wheelToVibrato = getVal(parameters.getModWheelToVibrato());
    snap->mod.atToDcw = getVal(parameters.getModAtToDcw());
    snap->mod.atToVibrato = getVal(parameters.getModAtToVibrato());
    
    snap->mod.keyFollowDcw = getInt(parameters.getKeyFollowDcw());
    snap->mod.keyFollowAmp = getInt(parameters.getKeyFollowDca());
    snap->mod.keyFollowDco = getInt(parameters.getKeyFollowDco()); 
    snap->mod.detune = getBool(parameters.getHardSync()) ? 1 : 0; 
    snap->mod.glideTime = getVal(parameters.getGlideTime());

    // LFO
    snap->lfo.rate = getVal(parameters.getLfoRate(), 1.0f);
    snap->lfo.waveform = getInt(parameters.getLfoWaveform());
    snap->lfo.depth = getVal(parameters.getLfoDepth());
    snap->lfo.delay = getVal(parameters.getLfoDelay());

    // Arp
    snap->arp.enabled = getBool(parameters.getArpEnabled()) && (snap->system.opMode != 0);
    snap->arp.latch = getBool(parameters.getArpLatch());
    snap->arp.rate = getInt(parameters.getArpRate());
    snap->arp.pattern = getInt(parameters.getArpPattern());
    snap->arp.octave = getIntParam(parameters.getArpOctave()) + 1; // Audit Fix: getIntParam
    snap->arp.gate = getVal(parameters.getArpGate(), 1.0f);
    snap->arp.swing = getVal(parameters.getArpSwing(), 0.0f);
    snap->arp.swingMode = getInt(parameters.getArpSwingMode(), 0);

    // Effects
    snap->effects.chorusOn = true;
    snap->effects.chorusRate = getVal(parameters.getChorusRate());
    snap->effects.chorusDepth = getVal(parameters.getChorusDepth());
    snap->effects.chorusMix = getVal(parameters.getChorusMix());

    snap->effects.delayTime = getVal(parameters.getDelayTime(), 0.25f);
    snap->effects.delayFb = getVal(parameters.getDelayFeedback());
    snap->effects.delayMix = getVal(parameters.getDelayMix());
    
    snap->effects.reverbSize = getVal(parameters.getReverbSize(), 0.5f);
    snap->effects.reverbMix = getVal(parameters.getReverbMix());

    // [NEW] Drive
    snap->effects.driveAmount = getVal(parameters.getDriveAmount());
    snap->effects.driveColor = getVal(parameters.getDriveColor(), 0.5f);
    snap->effects.driveMix = getVal(parameters.getDriveMix());

    // [NEW] Modern Filters
    // These only affect the output filter (EffectsChain). Voice filters are handled in updateFilters via setNonRealtime?
    // No, voice filters are handled by AudioThread params? 
    // Wait, updateFilters calls voiceManager.setFilterCutoff directly. 
    // VoiceManager inside processBlock uses these values.
    // So we just need to populate the snapshot for EffectsChain here.
    snap->effects.lpfCutoff = getVal(parameters.getModernLpfCutoff(), 20000.0f);
    snap->effects.lpfReso = getVal(parameters.getModernLpfReso(), 0.0f);
    snap->effects.hpfCutoff = getVal(parameters.getModernHpfCutoff(), 20.0f);

    // Envelopes (Calculated from ADSR Macros)
    double sr = currentSampleRate.load();
    auto* envs = &snap->envelopes;
    
    // Convert Macros directly to Snapshot format
    ::CZ101::State::EnvelopeSerializer::convertADSRToSnapshot(
        getVal(parameters.getDcwAttack()), getVal(parameters.getDcwDecay()),
        getVal(parameters.getDcwSustain(), 1.0f), getVal(parameters.getDcwRelease()),
        envs->dcw1, sr);
    envs->dcw2 = envs->dcw1; 

    ::CZ101::State::EnvelopeSerializer::convertADSRToSnapshot(
        getVal(parameters.getDcaAttack()), getVal(parameters.getDcaDecay()),
        getVal(parameters.getDcaSustain(), 1.0f), getVal(parameters.getDcaRelease()),
        envs->dca1, sr);
    envs->dca2 = envs->dca1; 
    
    // Pitch Envelope: Retrieve from PresetManager to preserve loaded state (Macros don't control Pitch yet)
    {
        const juce::ScopedReadLock srl(presetManager.getLock());
        const auto& currentPreset = presetManager.getCurrentPreset();
        ::CZ101::State::EnvelopeSerializer::copyToSnapshot(currentPreset.pitchEnv, snap->envelopes.pitch1);
        ::CZ101::State::EnvelopeSerializer::copyToSnapshot(currentPreset.pitchEnv2, snap->envelopes.pitch2);
    }
    
    return snap;
}
