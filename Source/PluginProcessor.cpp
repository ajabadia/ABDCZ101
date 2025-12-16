#include "PluginProcessor.h"
#include "PluginEditor.h"

CZ101AudioProcessor::CZ101AudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      voiceManager(),
      parameters(*this),
      presetManager(&parameters, &voiceManager),
      midiProcessor(voiceManager, presetManager),
      sysExManager()
{
    parameters.createParameters();

    // Setup File Logger
    auto logFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                   .getChildFile("CZ101Emulator")
                   .getChildFile("cz101_debug.log");
                   
    fileLogger = std::make_unique<juce::FileLogger>(logFile, "CZ-101 Emulator Log");
    juce::Logger::setCurrentLogger(fileLogger.get());
    juce::Logger::writeToLog("Logger initialized at: " + logFile.getFullPathName());
    
    // Bind SysEx Callback
    sysExManager.onPresetParsed = [this](const CZ101::State::Preset& p) {
        // 1. Load into PresetManager (updates currentPreset and calls applyPresetToProcessor internally)
        // Note: loadPresetFromStruct calls applyPresetToProcessor which updates APVTS.
        // It also applies envelopes to VoiceManager.
        // HOWEVER, our applyPresetToVoiceEngine handles EXTRA logic (Line Select, calculated Levels).
        // loadPresetFromStruct basically does a subset of what we need OR essentially the same.
        // Let's call BOTH for safety:
        // - loadPresetFromStruct: Updates 'currentPreset', applies basic params + envelopes.
        // - applyPresetToVoiceEngine: Applies derived logic (Line Select 1+1' -> osc levels) + syncs params again.
        // Redundant sync is fine. Key is updating currentPreset struct.
        presetManager.loadPresetFromStruct(p);
        
        applyPresetToVoiceEngine(p);
    };
    
    midiProcessor.setSysExManager(&sysExManager);
    
    // Load default preset once parameters are ready
    if (!presetManager.getPresets().empty())
        presetManager.loadPreset(0);
}

void CZ101AudioProcessor::applyPresetToVoiceEngine(const CZ101::State::Preset& preset)
{
    // Apply Pitch Envelope
    for (int i = 0; i < 8; i++) {
        voiceManager.setPitchStage(i, preset.pitchEnv.rates[i], 
                                  preset.pitchEnv.levels[i]);
    }
    voiceManager.setPitchSustainPoint(preset.pitchEnv.sustainPoint);
    voiceManager.setPitchEndPoint(preset.pitchEnv.endPoint);
    
    // Apply DCW Envelope
    for (int i = 0; i < 8; i++) {
        voiceManager.setDCWStage(i, preset.dcwEnv.rates[i], 
                                preset.dcwEnv.levels[i]);
    }
    voiceManager.setDCWSustainPoint(preset.dcwEnv.sustainPoint);
    voiceManager.setDCWEndPoint(preset.dcwEnv.endPoint);
    
    // Apply DCA Envelope
    for (int i = 0; i < 8; i++) {
        voiceManager.setDCAStage(i, preset.dcaEnv.rates[i], 
                                preset.dcaEnv.levels[i]);
    }
    voiceManager.setDCASustainPoint(preset.dcaEnv.sustainPoint);
    voiceManager.setDCAEndPoint(preset.dcaEnv.endPoint);

    // --- Extended Parameter Mapping (Oscillators, LFO, Effects) ---

    auto getParam = [&](const std::string& key, float defaultVal) -> float {
        auto it = preset.parameters.find(key);
        return (it != preset.parameters.end()) ? it->second : defaultVal;
    };

    // 1. Oscillators & Line Select
    int osc1Wave = (int)getParam("osc1waveform", 0.0f);
    int osc2Wave = (int)getParam("osc2waveform", 0.0f);
    
    // CZ-101 Line Select: 0=1, 1=2, 2=1+1', 3=1+2
    // If not present (e.g. envelope-only test), assume 0 (Line 1) or check waveforms?
    // If we have waveforms but no line select, we might check if they are non-zero?
    // But mostly legitimate SysEx will have lineselect.
    int lineSel = (int)getParam("lineselect", 0.0f);

    float osc1Lvl = 0.0f;
    float osc2Lvl = 0.0f;
    
    switch (lineSel) {
        case 0: osc1Lvl = 1.0f; osc2Lvl = 0.0f; break; // Line 1
        case 1: osc1Lvl = 0.0f; osc2Lvl = 1.0f; break; // Line 2
        case 2: osc1Lvl = 1.0f; osc2Lvl = 1.0f; break; // Line 1+1'
        case 3: osc1Lvl = 1.0f; osc2Lvl = 1.0f; break; // Line 1+2
        default: osc1Lvl = 1.0f; osc2Lvl = 0.0f; break;
    }
    

    // Fallback: If map is empty (old SysEx logic), force Osc1
    if (preset.parameters.empty()) {
        osc1Lvl = 1.0f;
    }

    voiceManager.setOsc1Waveform(osc1Wave);
    voiceManager.setOsc2Waveform(osc2Wave);
    voiceManager.setOsc1Level(osc1Lvl);
    voiceManager.setOsc2Level(osc2Lvl);
    
    float detune = getParam("osc2detune", 0.0f);
    voiceManager.setOsc2Detune(detune);
    
    // 2. Filter (Global Open)
    filterL.setCutoff(20000.0f); 
    filterR.setCutoff(20000.0f);
    
    // 3. LFO
    if (preset.parameters.count("lforate")) {
        lfo.setFrequency(getParam("lforate", 1.0f));
    }
    if (preset.parameters.count("lfowaveform")) {
        lfo.setWaveform((CZ101::DSP::LFO::Waveform)(int)getParam("lfowaveform", 0.0f));
    }
    if (preset.parameters.count("lfodepth")) {
        voiceManager.setVibratoDepth(getParam("lfodepth", 0.0f));
    }
    
    // 4. Master Tune
    int octRange = (int)getParam("octaverange", 0.0f); 
    float masterTuneShift = 0.0f;
    if (octRange == 1) masterTuneShift = 12.0f;
    if (octRange == 2) masterTuneShift = -12.0f;
    voiceManager.setMasterTune(masterTuneShift);

    // --- SYNC TO APVTS (UI & UpdateParameters Loop) ---
    // Critical: Update the parameters so the AudioProcessorValueTreeState reflects the changes.
    // Otherwise updateParameters() inside processBlock will overwrite everything with stale UI values.

    if (parameters.osc1Waveform) parameters.osc1Waveform->setValueNotifyingHost(parameters.osc1Waveform->convertTo0to1((float)osc1Wave));
    if (parameters.osc2Waveform) parameters.osc2Waveform->setValueNotifyingHost(parameters.osc2Waveform->convertTo0to1((float)osc2Wave));
    
    // Level scaling: 0.0-1.0 matches parameter range
    if (parameters.osc1Level) parameters.osc1Level->setValueNotifyingHost(osc1Lvl);
    if (parameters.osc2Level) parameters.osc2Level->setValueNotifyingHost(osc2Lvl);
    
    if (parameters.osc2Detune) {
        // Detune coming from SysEx is in cents (+/-). Param might be normalized 0..1 or scaled.
        // Check Parameters.cpp for range. Usually NormalisableRange handles mapping if we usesetValueNotifyingHost?
        // Wait, setValueNotifyingHost EXPECTS 0..1 NORMALIZED value.
        // We need to use the parameter's range to map Real->Normalized.
        // RangedAudioParameter::convertTo0to1(realValue)
        parameters.osc2Detune->setValueNotifyingHost(parameters.osc2Detune->convertTo0to1(detune));
    }

    if (parameters.lfoRate)    parameters.lfoRate->setValueNotifyingHost(parameters.lfoRate->convertTo0to1(getParam("lforate", 1.0f)));
    if (parameters.lfoDepth)   parameters.lfoDepth->setValueNotifyingHost(parameters.lfoDepth->convertTo0to1(getParam("lfodepth", 0.0f)/2.0f)); // Undo scaling in updateParams (depth*2)
    if (parameters.lfoWaveform) parameters.lfoWaveform->setValueNotifyingHost(parameters.lfoWaveform->convertTo0to1((float)(int)getParam("lfowaveform", 0.0f)));

}

CZ101AudioProcessor::~CZ101AudioProcessor()
{
    juce::Logger::setCurrentLogger(nullptr);
}

// ... (Top of file usually)

void CZ101AudioProcessor::saveCurrentPreset(const juce::String& name)
{
    // 1. Capture current state from params/voices into CurrentPreset
    presetManager.copyStateFromProcessor();
    
    // 2. Save it to the current slot (in memory)
    // Note: To persist to disk, we rely on getStateInformation/setStateInformation or a dedicated file save.
    // For now, this updates the runtime preset so switching presets doesn't lose edits, if we stay in session.
    // Wait, switching AWAY saves? No, usually "Save" button confirms the save.
    // Switching away usually reloads the destination preset.
    
    int idx = getCurrentProgram();
    presetManager.savePreset(idx, name.toStdString());
    
    // Also update host notification?
    updateHostDisplay();
}

const juce::String CZ101AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool CZ101AudioProcessor::acceptsMidi() const
{
    return true;
}

bool CZ101AudioProcessor::producesMidi() const
{
    return false;
}

bool CZ101AudioProcessor::isMidiEffect() const
{
    return false;
}

double CZ101AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int CZ101AudioProcessor::getNumPrograms()
{
    return 1;
}

int CZ101AudioProcessor::getCurrentProgram()
{
    return 0;
}

void CZ101AudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String CZ101AudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void CZ101AudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void CZ101AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    voiceManager.setSampleRate(sampleRate);
    filterL.setSampleRate(sampleRate);
    filterR.setSampleRate(sampleRate);
    delayL.setSampleRate(sampleRate);
    delayR.setSampleRate(sampleRate);
    reverb.setSampleRate(sampleRate);
    chorus.prepare(sampleRate);
    lfo.setSampleRate(sampleRate);
    lfo.setFrequency(1.0f);
    
    // Patch 1: Deep Persistence (User Fix)
    juce::File presetsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                            .getChildFile("CZ101Emulator");
    
    if (!presetsDir.exists()) {
        presetsDir.createDirectory();
        // Simple logging if we had a logger, or just proceed
    }
    
    juce::File bankFile = presetsDir.getChildFile("userbank.json");
    
    if (bankFile.existsAsFile()) {
        presetManager.loadBank(bankFile);
        if (presetManager.getPresets().empty()) {
            presetManager.createFactoryPresets();
            presetManager.saveBank(bankFile);
        }
        presetManager.loadPreset(0);
    } else {
        presetManager.createFactoryPresets();
        presetManager.saveBank(bankFile);
        presetManager.loadPreset(0);
    } // Default rate
}

void CZ101AudioProcessor::releaseResources()
{
}

bool CZ101AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void CZ101AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    performanceMonitor.startMeasurement();
    
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
    
    updateParameters();
    
    midiProcessor.processMidiBuffer(midiMessages);
    
    auto* channelDataL = buffer.getWritePointer(0);
    auto* channelDataR = buffer.getWritePointer(1);
    
    // float lfoVal = lfo.getNextValue(); // Removed block-level update
    // voiceManager.updateLFO(lfoVal);
    
    // Pass LFO to renderNextBlock for sample-accurate modulation
    voiceManager.renderNextBlock(channelDataL, channelDataR, buffer.getNumSamples(), &lfo);
    
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        channelDataL[i] = filterL.processSample(channelDataL[i]);
        channelDataR[i] = filterR.processSample(channelDataR[i]);
        
        channelDataL[i] = delayL.processSample(channelDataL[i]);
        channelDataR[i] = delayR.processSample(channelDataR[i]);
    }
    
    // Process Reverb (Block-based)
    // Process Reverb (Block-based)
    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);
    
    // Process Chorus (Block-based)
    chorus.process(left, right, buffer.getNumSamples());
    
    // Re-get pointers (Chorus might have modified buffer/ptrs if it were internal buffer swap, though getWritePointer is persistent for block)
    // However, good practice if using distinct buffers. Here we use the same buffer. 
    // The user explicitly requested: "Fix: Obtener punteros después de chorus.process()" because "reverb.processStereo() requiere punteros".
    // Actually getWritePointer is valid for the whole block. But let's follow the instruction.
    left = buffer.getWritePointer(0);
    right = buffer.getWritePointer(1);
    
    reverb.processStereo(left, right, buffer.getNumSamples());
    
    // --- VISUALIZATION COPY ---
    // Push mono mix to circular buffer for the UI
    if (visFifo.getFreeSpace() >= buffer.getNumSamples())
    {
         int start1, size1, start2, size2;
         visFifo.prepareToWrite(buffer.getNumSamples(), start1, size1, start2, size2);
         
         // Helper to copy and mix down (Mono for display simpler for now)
         auto* l = buffer.getReadPointer(0);
         auto* r = buffer.getReadPointer(1);
         
         for (int i=0; i<size1; ++i) 
             visBuffer.setSample(0, start1 + i, (l[i] + r[i]) * 0.5f);
             
         for (int i=0; i<size2; ++i) 
             visBuffer.setSample(0, start2 + i, (l[size1 + i] + r[size1 + i]) * 0.5f);
             
         visFifo.finishedWrite(size1 + size2);
    }
    
    performanceMonitor.stopMeasurement();
}

void CZ101AudioProcessor::updateParameters()
{
    // --- VOICES (Oscillators & Envelopes) ---
    // Direct update without checks (Assumes parameters are created safely in ctor)
    voiceManager.setOsc1Level(parameters.osc1Level->get());
    voiceManager.setOsc1Waveform(parameters.osc1Waveform->getIndex());

    voiceManager.setOsc2Level(parameters.osc2Level->get());
    voiceManager.setOsc2Waveform(parameters.osc2Waveform->getIndex());
    voiceManager.setOsc2Detune(parameters.osc2Detune->get());
    
    // Hard Sync
    if (parameters.hardSync)
        voiceManager.setHardSync(parameters.hardSync->get());
    
    if (parameters.ringMod)
        voiceManager.setRingMod(parameters.ringMod->get());

    if (parameters.glideTime)
        voiceManager.setGlideTime(parameters.glideTime->get());

    // DCW
    voiceManager.setDCWAttack(parameters.dcwAttack->get());
    voiceManager.setDCWDecay(parameters.dcwDecay->get());
    voiceManager.setDCWSustain(parameters.dcwSustain->get());
    voiceManager.setDCWRelease(parameters.dcwRelease->get());

    // DCA
    voiceManager.setDCAAttack(parameters.dcaAttack->get());
    voiceManager.setDCADecay(parameters.dcaDecay->get());
    voiceManager.setDCASustain(parameters.dcaSustain->get());
    voiceManager.setDCARelease(parameters.dcaRelease->get());

    // --- EFFECTS ---
    if (parameters.filterCutoff)
    {
        float cutoff = parameters.filterCutoff->get();
        filterL.setCutoff(cutoff);
        filterR.setCutoff(cutoff);
    }
    
    if (parameters.filterResonance)
    {
        float res = parameters.filterResonance->get();
        filterL.setResonance(res);
        filterR.setResonance(res);
    }
    
    if (parameters.delayTime)
    {
        delayL.setDelayTime(parameters.delayTime->get());
        delayR.setDelayTime(parameters.delayTime->get());
    }
    
    if (parameters.delayFeedback)
    {
        delayL.setFeedback(parameters.delayFeedback->get());
        delayR.setFeedback(parameters.delayFeedback->get());
    }
    
    if (parameters.delayMix)
    {
        delayL.setMix(parameters.delayMix->get());
        delayR.setMix(parameters.delayMix->get());
    }
    
    // Chorus Parameters
    // This part of the original request was for `updateParameters`
    // The audio processing part of the request was moved to `processBlock`
    // as it's an audio processing function.
    if (parameters.chorusRate)
        chorus.setRate(parameters.chorusRate->get());
    if (parameters.chorusDepth)
        chorus.setDepth(parameters.chorusDepth->get());
    if (parameters.chorusMix)
        chorus.setMix(parameters.chorusMix->get());
    
    if (parameters.reverbSize && parameters.reverbMix)
    {
        // Reverb Params: room, damping, wet, dry, width
        float size = parameters.reverbSize->get();
        float mix = parameters.reverbMix->get();
        
        // Map Mix to Wet/Dry (simple)
        // Dry = 1.0, Wet=Mix? Or Dry=1-Mix?
        // Let's keep dry 1.0 (typical send style or parallel) or balanced.
        // For effect box: Dry reduces as Wet increases? 
        // juce::Reverb typical: WetLevel 0..1, DryLevel 0..1
        // Let's do: Dry = 1.0, Wet = Mix * 0.8
        
        // juce::Reverb::Parameters struct setter
        reverbParams.roomSize = size;
        reverbParams.damping = 0.5f;
        reverbParams.wetLevel = mix;
        reverbParams.dryLevel = 1.0f - (mix * 0.5f);
        reverbParams.width = 1.0f;
        reverbParams.freezeMode = 0.0f;
        
        reverb.setParameters(reverbParams);
    }
    
    // LFO / Vibrato
    if (parameters.lfoRate)
    {
        lfo.setFrequency(parameters.lfoRate->get());
    }
    
    if (parameters.lfoWaveform)
    {
        // Cast int/index to Waveform enum
        lfo.setWaveform(static_cast<CZ101::DSP::LFO::Waveform>(parameters.lfoWaveform->getIndex()));
    }
    
    if (parameters.lfoDepth)
    {
        // Map 0.0-1.0 to semitones (e.g. 0 to 2 semitones depth)
        float depth = parameters.lfoDepth->get() * 2.0f; 
        voiceManager.setVibratoDepth(depth);
    }
}

bool CZ101AudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* CZ101AudioProcessor::createEditor()
{
    return new CZ101AudioProcessorEditor(*this);
}

void CZ101AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::ignoreUnused(destData);
}

void CZ101AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    juce::ignoreUnused(data, sizeInBytes);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CZ101AudioProcessor();
}
