#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "UI/LCDStateManager.h" // Required for unique_ptr destructor

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
    
    // Initialize LCD State Manager here to ensure it persists and avoids dangling references
    juce::Logger::writeToLog("CZ101 Processor: Initializing LCD State Manager");
    lcdStateManager = std::make_unique<CZ101::UI::LCDStateManager>(parameters.getAPVTS());
    juce::Logger::writeToLog("CZ101 Processor: Constructor End");
}

CZ101AudioProcessor::~CZ101AudioProcessor() 
{ 
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
double CZ101AudioProcessor::getTailLengthSeconds() const { return 8.0; } 
int CZ101AudioProcessor::getNumPrograms() { return static_cast<int>(presetManager.getPresets().size()); }
int CZ101AudioProcessor::getCurrentProgram() { return presetManager.getCurrentPresetIndex(); }
void CZ101AudioProcessor::setCurrentProgram(int index) 
{ 
    if (index != getCurrentProgram())
        presetManager.loadPreset(index); 
}
const juce::String CZ101AudioProcessor::getProgramName(int index) 
{ 
    if (index >= 0 && index < getNumPrograms())
        return presetManager.getPresets()[index].name;
    return {};
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
    // juce::Logger::writeToLog("CZ101Processor: prepareToPlay Start");
    currentSampleRate.store(sampleRate);
    
    // Audit Fix 4.1: FIFO resizing
    voiceManager.setSampleRate(sampleRate);
    delayL.setSampleRate(sampleRate);
    delayR.setSampleRate(sampleRate);
    reverb.setSampleRate(sampleRate);
    chorus.prepare(sampleRate);
    
    // Audit Fix: Initialize Vis Buffer (Triple Buffer is std::array, no resize needed)
    // visBuffer.setSize(1, VIS_FIFO_SIZE);
    // visBuffer.clear();
    
    juce::File presetsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("CZ101Emulator");
    if (!presetsDir.exists()) presetsDir.createDirectory();
    juce::File bankFile = presetsDir.getChildFile(CZ101::State::PresetManager::USER_BANK_FILENAME);
    
    // juce::Logger::writeToLog("CZ101Processor: prepareToPlay Start");
    if (bankFile.existsAsFile()) {
        juce::Logger::writeToLog("CZ101Processor: Loading Bank File: " + bankFile.getFullPathName());
        presetManager.loadBank(bankFile);
        if (presetManager.getPresets().empty()) presetManager.createFactoryPresets();
    } else {
        juce::Logger::writeToLog("CZ101Processor: Creating Factory Presets");
        presetManager.createFactoryPresets();
        presetManager.saveBank(bankFile);
    }
    
    presetManager.loadPreset(0);
    // juce::Logger::writeToLog("CZ101Processor: prepareToPlay End");
    
    // Modern Filters Setup
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 2; // Stereo
    
    modernLpf.prepare(spec);
    modernLpf.setMode(juce::dsp::LadderFilterMode::LPF24);
    
    modernHpf.prepare(spec);
    modernHpf.setType(juce::dsp::StateVariableTPTFilterType::highpass);

    // Audit Fix 1.2: Reset Effects RNG/Phase
    chorus.prepare(sampleRate);
    delayL.reset();
    delayR.reset();
    modernLpf.reset();
    modernHpf.reset();
    reverb.reset(); // Added missing reverb reset
    
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
    
    // Audit Fix [D]: LOCK-FREE. Process pending preset updates from SysEx
    int s1, sz1, s2, sz2;
    presetFifo.prepareToRead(1, s1, sz1, s2, sz2);
    if (sz1 > 0) applyPresetEnvelopes(presetBuffer[s1]);
    else if (sz2 > 0) applyPresetEnvelopes(presetBuffer[s2]);
    presetFifo.finishedRead(sz1 + sz2);

    // Optimized Bypass Path
    if (parameters.getBypass() && parameters.getBypass()->get())
    {
        midiProcessor.processMidiBuffer(midiMessages);
        performanceMonitor.stopMeasurement();
        return;
    }

    processEnvelopeUpdates();
    updateParameters();
    
    midiProcessor.processMidiBuffer(midiMessages);
    
    auto* channelDataL = buffer.getWritePointer(0);
    auto* channelDataR = buffer.getWritePointer(1);
    
    voiceManager.renderNextBlock(channelDataL, channelDataR, buffer.getNumSamples());
    
    // Audit Fix: Consolidated with start of block update

    // Effects Path (Gated by Authentic Mode)
    // Effects Path (Included in Modern Mode)
    // 0=101, 1=5000, 2=Modern
    int opMode = parameters.getOperationMode() ? parameters.getOperationMode()->getIndex() : 0;
    bool isModern = (opMode == 2);
    
    // Note: Previously this checked Authentic Mode (so isAuthentic -> !isModern)
    // Here we want to know if we should process effects. Effects are enabled in Modern Mode.
    if (isModern)
    {
        // Modern Filters (Before effects)
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        modernLpf.process(context);
        modernHpf.process(context);

        chorus.process(channelDataL, channelDataR, buffer.getNumSamples());
        
        for (int i=0; i<buffer.getNumSamples(); ++i) {
            channelDataL[i] = delayL.processSample(channelDataL[i]);
            if (channelDataR != channelDataL) channelDataR[i] = delayR.processSample(channelDataR[i]);
        }
        
        reverb.processStereo(channelDataL, channelDataR, buffer.getNumSamples());
    }
    else
    {
        // Audit Fix: In Authentic mode, we still allow Chorus? 
        // The real CZ-101 has a hardware chorus (analog BBD), so we should keep it.
        // But the Delay/Reverb are definitely modern additions.
        chorus.process(channelDataL, channelDataR, buffer.getNumSamples());
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
        // Modern LPF (Output) uses base value
        modernLpf.setCutoffFrequencyHz(baseCutoff); 
        
        // Voice Filters use Macro Brilliance
        float finalCutoff = juce::jlimit(20.0f, 20000.0f, baseCutoff + m.brillianceOffset);
        voiceManager.setFilterCutoff(finalCutoff);
    }
    if (parameters.getModernLpfReso()) {
        float res = parameters.getModernLpfReso()->get();
        voiceManager.setFilterResonance(res);
        modernLpf.setResonance(res);
    }
    if (parameters.getModernHpfCutoff()) {
        float hpf = parameters.getModernHpfCutoff()->get();
        voiceManager.setHPF(hpf);
        modernHpf.setCutoffFrequency(hpf);
    }
}

void CZ101AudioProcessor::updateOscillators(const MacroValues& m)
{
    int lineSel = parameters.getLineSelect() ? parameters.getLineSelect()->getIndex() : 2; 
    float l1 = parameters.getOsc1Level()->get();
    float l2 = parameters.getOsc2Level()->get();
    int w1_1 = parameters.getOsc1Waveform()->getIndex();
    int w1_2 = parameters.getOsc1Waveform2() ? (parameters.getOsc1Waveform2()->getIndex() == 0 ? 8 : parameters.getOsc1Waveform2()->getIndex() - 1) : 8;
    int w2_1 = parameters.getOsc2Waveform()->getIndex();
    int w2_2 = parameters.getOsc2Waveform2() ? (parameters.getOsc2Waveform2()->getIndex() == 0 ? 8 : parameters.getOsc2Waveform2()->getIndex() - 1) : 8;

    // 1. Line Select Muting Logic
    if (lineSel == 0) l2 = 0.0f; // Line 1 Only
    if (lineSel == 1) l1 = 0.0f; // Line 2 Only

    // Tone Mix Logic
    float mix = parameters.getLineMix() ? parameters.getLineMix()->get() : 0.5f;
    if (lineSel >= 2) 
    {
        if (mix < 0.5f) {
            l2 *= (mix * 2.0f);
        } else {
            l1 *= ((1.0f - mix) * 2.0f);
        }
    }

    // 2. Line Select 1+1' Logic (Copy Line 1 to Line 2)
    bool lineChanged = (lineSel != paramCache.lineSelect);
    bool levelsChanged = (l1 != paramCache.osc1Level || l2 != paramCache.osc2Level);
    bool wavesChanged = (w1_1 != paramCache.w1_1 || w1_2 != paramCache.w1_2 || w2_1 != paramCache.w2_1 || w2_2 != paramCache.w2_2);
    
    if (lineSel == 2 && (lineChanged || levelsChanged || wavesChanged)) 
    { 
        w2_1 = w1_1; w2_2 = w1_2; l2 = l1; 
        for (int i=0; i<8; ++i) {
                float r, lv;
                voiceManager.getPitchStage(1, i, r, lv); voiceManager.setPitchStage(2, i, r, lv);
                voiceManager.getDCWStage(1, i, r, lv); voiceManager.setDCWStage(2, i, r, lv);
                voiceManager.getDCAStage(1, i, r, lv); voiceManager.setDCAStage(2, i, r, lv);
        }
        voiceManager.setPitchSustainPoint(2, voiceManager.getPitchSustainPoint(1));
        voiceManager.setPitchEndPoint(2, voiceManager.getPitchEndPoint(1));
        voiceManager.setDCWSustainPoint(2, voiceManager.getDCWSustainPoint(1));
        voiceManager.setDCWEndPoint(2, voiceManager.getDCWEndPoint(1));
        voiceManager.setDCASustainPoint(2, voiceManager.getDCASustainPoint(1));
        voiceManager.setDCAEndPoint(2, voiceManager.getDCAEndPoint(1));
    }

    // Update Cache
    paramCache.lineSelect = lineSel;
    paramCache.osc1Level = l1; paramCache.osc2Level = l2;
    paramCache.w1_1 = w1_1; paramCache.w1_2 = w1_2;
    paramCache.w2_1 = w2_1; paramCache.w2_2 = w2_2;

    // 3. Update VoiceManager engine
    voiceManager.setOsc1Level(l1);
    voiceManager.setOsc2Level(l2);
    voiceManager.setOsc1Waveforms(w1_1, w1_2);
    voiceManager.setOsc2Waveforms(w2_1, w2_2);
    
    voiceManager.setOsc2DetuneHardware(
        parameters.getDetuneOctave() ? parameters.getDetuneOctave()->get() : 0,
        parameters.getDetuneCoarse() ? parameters.getDetuneCoarse()->get() : 0,
        parameters.getDetuneFine() ? parameters.getDetuneFine()->get() : 0
    );
    
    if (parameters.getHardSync()) voiceManager.setHardSync(parameters.getHardSync()->get());
    if (parameters.getRingMod()) voiceManager.setRingMod(parameters.getRingMod()->get());
    if (parameters.getGlideTime()) voiceManager.setGlideTime(parameters.getGlideTime()->get());
    if (parameters.getMasterVolume()) voiceManager.setMasterVolume(parameters.getMasterVolume()->get());
}

void CZ101AudioProcessor::updateEnvelopes(const MacroValues& m)
{
    auto applyTone = [&](float val) { return juce::jlimit(0.0f, 10.0f, val / m.toneSpeedMult); };

    if (parameters.getDcwAttack()) voiceManager.setDCWAttack(applyTone(parameters.getDcwAttack()->get()));
    if (parameters.getDcwDecay()) voiceManager.setDCWDecay(applyTone(parameters.getDcwDecay()->get()));
    if (parameters.getDcwSustain()) voiceManager.setDCWSustain(parameters.getDcwSustain()->get());
    if (parameters.getDcwRelease()) voiceManager.setDCARelease(applyTone(parameters.getDcwRelease()->get()));

    if (parameters.getDcaAttack()) voiceManager.setDCAAttack(applyTone(parameters.getDcaAttack()->get()));
    if (parameters.getDcaDecay()) voiceManager.setDCADecay(applyTone(parameters.getDcaDecay()->get()));
    if (parameters.getDcaSustain()) voiceManager.setDCASustain(parameters.getDcaSustain()->get());
    if (parameters.getDcaRelease()) voiceManager.setDCARelease(applyTone(parameters.getDcaRelease()->get()));
}

void CZ101AudioProcessor::updateLFO()
{
    float lRate = parameters.getLfoRate() ? parameters.getLfoRate()->get() : 1.0f;
    int lWave = parameters.getLfoWaveform() ? parameters.getLfoWaveform()->getIndex() : 0;
    float lDepth = parameters.getLfoDepth() ? parameters.getLfoDepth()->get() : 0.0f;
    float lDelay = parameters.getLfoDelay() ? parameters.getLfoDelay()->get() : 0.0f;
    
    if (lRate != paramCache.lfoRate) { voiceManager.setLFOFrequency(lRate); paramCache.lfoRate = lRate; }
    if (lWave != paramCache.lfoWave) { voiceManager.setLFOWaveform(static_cast<CZ101::DSP::LFO::Waveform>(lWave)); paramCache.lfoWave = lWave; }
    if (lDepth != paramCache.lfoDepth) { voiceManager.setVibratoDepth(lDepth); paramCache.lfoDepth = lDepth; }
    if (lDelay != paramCache.lfoDelay) { voiceManager.setLFODelay(lDelay); paramCache.lfoDelay = lDelay; }
}

void CZ101AudioProcessor::updateModMatrix()
{
    float vDcw = parameters.getModVeloToDcw()->get();
    float vDca = parameters.getModVeloToDca()->get();
    float wDcw = parameters.getModWheelToDcw()->get();
    float wLfo = parameters.getModWheelToLfoRate()->get();
    float wVib = parameters.getModWheelToVibrato()->get();
    float atDcw = parameters.getModAtToDcw()->get();
    float atVib = parameters.getModAtToVibrato()->get();
    float ktDcw = parameters.getKeyTrackDcw()->get();
    float ktPitch = parameters.getKeyTrackPitch()->get();
    int kfDcoVal = parameters.getKeyFollowDco()->getIndex();
    int kfDcwVal = parameters.getKeyFollowDcw()->getIndex();
    int kfDcaVal = parameters.getKeyFollowDca()->getIndex();

    bool matrixChanged = (vDcw != paramCache.veloToDcw || vDca != paramCache.veloToDca ||
                          wDcw != paramCache.wheelToDcw || wLfo != paramCache.wheelToLfo || wVib != paramCache.wheelToVib ||
                          atDcw != paramCache.atToDcw || atVib != paramCache.atToVib ||
                          ktDcw != paramCache.ktDcw || ktPitch != paramCache.ktPitch ||
                          kfDcoVal != paramCache.kfDco || kfDcwVal != paramCache.kfDcw || kfDcaVal != paramCache.kfDca);
                          
    if (matrixChanged)
    {
        CZ101::Core::Voice::ModulationMatrix matrix;
        matrix.veloToDcw = vDcw; matrix.veloToDca = vDca;
        matrix.wheelToDcw = wDcw; matrix.wheelToLfoRate = wLfo; matrix.wheelToVibrato = wVib;
        matrix.atToDcw = atDcw; matrix.atToVibrato = atVib;
        matrix.keyTrackDcw = ktDcw; matrix.keyTrackPitch = ktPitch;
        matrix.kfDco = kfDcoVal; matrix.kfDcw = kfDcwVal; matrix.kfDca = kfDcaVal;
        
        voiceManager.setModulationMatrix(matrix);
        
        paramCache.veloToDcw = vDcw; paramCache.veloToDca = vDca;
        paramCache.wheelToDcw = wDcw; paramCache.wheelToLfo = wLfo; paramCache.wheelToVib = wVib;
        paramCache.atToDcw = atDcw; paramCache.atToVib = atVib;
        paramCache.ktDcw = ktDcw; paramCache.ktPitch = ktPitch;
        paramCache.kfDco = kfDcoVal; paramCache.kfDcw = kfDcwVal; paramCache.kfDca = kfDcaVal;
    }
}

void CZ101AudioProcessor::updateEffects(const MacroValues& m)
{
    float cRate = parameters.getChorusRate() ? parameters.getChorusRate()->get() : 0.5f;
    float cDepth = parameters.getChorusDepth() ? parameters.getChorusDepth()->get() : 0.0f;
    float cMix = parameters.getChorusMix() ? (parameters.getChorusMix()->get() + m.spaceMix) : m.spaceMix;
    cMix = juce::jlimit(0.0f, 1.0f, cMix);
    
    if (cRate != paramCache.chorusRate) { chorus.setRate(cRate); paramCache.chorusRate = cRate; }
    if (cDepth != paramCache.chorusDepth) { chorus.setDepth(cDepth); paramCache.chorusDepth = cDepth; }
    if (cMix != paramCache.chorusMix) { chorus.setMix(cMix); paramCache.chorusMix = cMix; }
    
    float rSize = parameters.getReverbSize() ? parameters.getReverbSize()->get() : 0.5f;
    float rMix = parameters.getReverbMix() ? (parameters.getReverbMix()->get() + m.spaceMix) : m.spaceMix;
    rMix = juce::jlimit(0.0f, 1.0f, rMix);
    
    if (rSize != paramCache.revSize || rMix != paramCache.revMix)
    {
        reverbParams.roomSize = rSize;
        reverbParams.damping = 0.5f;
        reverbParams.wetLevel = rMix;
        reverbParams.dryLevel = 1.0f - (rMix * 0.5f);
        reverbParams.width = 1.0f;
        reverb.setParameters(reverbParams);
        
        paramCache.revSize = rSize; paramCache.revMix = rMix;
    }

    if (parameters.getDelayTime()) { 
        float dt = parameters.getDelayTime()->get();
        delayL.setDelayTime(dt); delayR.setDelayTime(dt); 
    }
    if (parameters.getDelayFeedback()) { float fb = parameters.getDelayFeedback()->get(); delayL.setFeedback(fb); delayR.setFeedback(fb); }
    if (parameters.getDelayMix()) { float mix = parameters.getDelayMix()->get(); delayL.setMix(mix); delayR.setMix(mix); }
    
    int chorusDelaySamples = (cMix > 0.0f) ? (int)(0.025 * getSampleRate()) : 0;
    int delaySamples = (parameters.getDelayMix() && parameters.getDelayMix()->get() > 0.0f) ? (int)(parameters.getDelayTime()->get() * getSampleRate()) : 0;
    int latency = chorusDelaySamples + (delaySamples > 0 ? 1 : 0);
    if (getLatencySamples() != latency) setLatencySamples(latency);
}

void CZ101AudioProcessor::updateSystemGlobal()
{
    // SysEx
    bool isProtected = parameters.getProtectSwitch() ? parameters.getProtectSwitch()->get() : true;
    bool isPrgEnabled = parameters.getSystemPrg() ? parameters.getSystemPrg()->get() : false;
    sysExManager.setProtectionState(isProtected, isPrgEnabled);
    
    // Operation Mode & Voice Limit
    if (auto* p = parameters.getOperationMode())
    {
        int mode = p->getIndex(); 
        auto synthModel = (mode == 0) ? CZ101::DSP::MultiStageEnvelope::Model::CZ101 
                                      : CZ101::DSP::MultiStageEnvelope::Model::CZ5000;
        voiceManager.setSynthModel(synthModel);
        
        // Audit Fix: Explicit Voice Limit Logic
        if (mode == 2) voiceManager.setVoiceLimit(16);
        else voiceManager.setVoiceLimit(mode==0 ? 4 : 8); 
    }
    
    // Phase 5.1: Oversampling Quality
    if (auto* p = parameters.getOversamplingQuality())
    {
        int qualityIndex = p->getIndex(); // 0=1x, 1=2x, 2=4x
        int factor = (qualityIndex == 0) ? 1 : (qualityIndex == 1) ? 2 : 4;
        voiceManager.setOversamplingFactor(factor);
    }
}

void CZ101AudioProcessor::updateArpeggiator()
{
    auto& arp = voiceManager.getArpeggiator();
    if (auto* p = parameters.getArpEnabled()) arp.setEnabled(p->get());
    if (auto* p = parameters.getArpLatch()) arp.setLatch(p->get());
    if (auto* p = parameters.getArpRate()) arp.setRate(static_cast<CZ101::DSP::Arpeggiator::Rate>(p->getIndex()));
    if (auto* p = parameters.getArpPattern()) arp.setPattern(static_cast<CZ101::DSP::Arpeggiator::Pattern>(p->getIndex()));
    if (auto* p = parameters.getArpOctave()) arp.setOctaveRange(p->get());
    
    // Phase 5 Preparation: Gate/Swing
    if (auto* p = parameters.getArpGate()) arp.setGateTime(p->get());
    if (auto* p = parameters.getArpSwing()) arp.setSwing(p->get());

    // Tempo Sync Logic
    double currentBpm = 120.0;
    if (auto* ph = getPlayHead()) {
        if (auto pos = ph->getPosition()) 
             if (pos->getBpm()) currentBpm = *pos->getBpm();
    }
    
    float internalBpm = parameters.getArpBpm() ? parameters.getArpBpm()->get() : 120.0f;
    if (juce::JUCEApplication::isStandaloneApp() || currentBpm <= 0.01) {
        currentBpm = internalBpm;
    }
    arp.setTempo(currentBpm);
    arp.setTempo(currentBpm);
}
