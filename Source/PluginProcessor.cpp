#include "PluginProcessor.h"
#include "PluginEditor.h"

CZ101AudioProcessor::CZ101AudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      midiProcessor(voiceManager),
      parameters(*this)
{
    parameters.createParameters();
    presetManager.setParameters(&parameters);
}

CZ101AudioProcessor::~CZ101AudioProcessor()
{
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
    juce::ignoreUnused(samplesPerBlock);
    
    voiceManager.setSampleRate(sampleRate);
    filterL.setSampleRate(sampleRate);
    filterR.setSampleRate(sampleRate);
    delayL.setSampleRate(sampleRate);
    delayR.setSampleRate(sampleRate);
    lfo.setSampleRate(sampleRate);
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
    
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
    
    updateParameters();
    
    midiProcessor.processMidiBuffer(midiMessages);
    
    auto* channelDataL = buffer.getWritePointer(0);
    auto* channelDataR = buffer.getWritePointer(1);
    
    voiceManager.renderNextBlock(channelDataL, channelDataR, buffer.getNumSamples());
    
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        channelDataL[i] = filterL.processSample(channelDataL[i]);
        channelDataR[i] = filterR.processSample(channelDataR[i]);
        
        channelDataL[i] = delayL.processSample(channelDataL[i]);
        channelDataR[i] = delayR.processSample(channelDataR[i]);
    }
}

void CZ101AudioProcessor::updateParameters()
{
    if (parameters.filterCutoff != nullptr)
    {
        float cutoff = parameters.filterCutoff->get();
        filterL.setCutoff(cutoff);
        filterR.setCutoff(cutoff);
    }
    
    if (parameters.filterResonance != nullptr)
    {
        float res = parameters.filterResonance->get();
        filterL.setResonance(res);
        filterR.setResonance(res);
    }
    
    if (parameters.delayTime != nullptr)
    {
        delayL.setDelayTime(parameters.delayTime->get());
        delayR.setDelayTime(parameters.delayTime->get());
    }
    
    if (parameters.delayFeedback != nullptr)
    {
        delayL.setFeedback(parameters.delayFeedback->get());
        delayR.setFeedback(parameters.delayFeedback->get());
    }
    
    if (parameters.delayMix != nullptr)
    {
        delayL.setMix(parameters.delayMix->get());
        delayR.setMix(parameters.delayMix->get());
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
