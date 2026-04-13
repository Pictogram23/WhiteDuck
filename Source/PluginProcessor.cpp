/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
WhiteDuckAudioProcessor::WhiteDuckAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

WhiteDuckAudioProcessor::~WhiteDuckAudioProcessor()
{
}

//==============================================================================
const juce::String WhiteDuckAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool WhiteDuckAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool WhiteDuckAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool WhiteDuckAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double WhiteDuckAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int WhiteDuckAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int WhiteDuckAudioProcessor::getCurrentProgram()
{
    return 0;
}

void WhiteDuckAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String WhiteDuckAudioProcessor::getProgramName (int index)
{
    return {};
}

void WhiteDuckAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void WhiteDuckAudioProcessor::prepareToPlay (double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    
    // Initialize envelope times
    duckingEnvelope.setAttackTime(sampleRate, attackTimeMs);
    duckingEnvelope.setReleaseTime(sampleRate, releaseTimeMs);
    
    // Prepare buffer
    duckedBuffer.setSize(2, samplesPerBlock);
}

void WhiteDuckAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool WhiteDuckAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void WhiteDuckAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear output channels that don't have input
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Process MIDI messages to trigger ducking envelope
    for (auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            if (msg.getNoteNumber() == midiNoteToTrigger)
            {
                duckingEnvelope.trigger();
            }
        }
    }
    
    // Clear MIDI messages - no need to pass them through
    midiMessages.clear();

    // Apply ducking to audio
    int numSamples = buffer.getNumSamples();
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float envelopeLevel = duckingEnvelope.process();
        
        // envelopeLevel: 0.01f (\u6700\u5c0f\u98b6) ~ 1.0f (\u6b63\u5e38)
        // gainReduction: 0 (\u5b8c\u5168\u30df\u30e5\u30fc\u30c8) ~ 1.0f (\u6b63\u5e38\u97f3)
        float gainReduction = envelopeLevel * mixAmount + (1.0f - mixAmount);
        
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            channelData[sample] *= gainReduction;
        }
    }
}

//==============================================================================
bool WhiteDuckAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* WhiteDuckAudioProcessor::createEditor()
{
    return new WhiteDuckAudioProcessorEditor (*this);
}

//==============================================================================
void WhiteDuckAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream mos(destData, true);
    
    // Binary format で保存（シンプルで確実）
    mos.writeFloat(mixAmount);
    mos.writeFloat(attackTimeMs);
    mos.writeFloat(releaseTimeMs);
    mos.writeInt(midiNoteToTrigger);
}

void WhiteDuckAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::MemoryInputStream mis(data, sizeInBytes, false);
    
    if (sizeInBytes >= (sizeof(float) * 3 + sizeof(int)))
    {
        mixAmount = mis.readFloat();
        attackTimeMs = mis.readFloat();
        releaseTimeMs = mis.readFloat();
        midiNoteToTrigger = mis.readInt();
        
        // Envelope を再初期化
        duckingEnvelope.setAttackTime(sampleRate, attackTimeMs);
        duckingEnvelope.setReleaseTime(sampleRate, releaseTimeMs);
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WhiteDuckAudioProcessor();
}
