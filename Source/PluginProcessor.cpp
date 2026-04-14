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
    
    // Initialize bands and reset filter states
    initializeBands();
    updateBandCoefficients();
    for (int b = 0; b < NUM_BANDS; ++b)
    {
        duckingBands[b].reset();
    }
    
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
                // Trigger new attack - do NOT reset filter to maintain ducking continuity
                duckingEnvelope.trigger();
            }
        }
    }
    
    // Clear MIDI messages - no need to pass them through
    midiMessages.clear();

    // Apply ducking to audio with band processing
    int numSamples = buffer.getNumSamples();
    
    // Check if any band is enabled
    bool anyBandEnabled = false;
    for (int b = 0; b < NUM_BANDS; ++b)
    {
        if (duckingBands[b].enabled)
        {
            anyBandEnabled = true;
            break;
        }
    }
    
    // Track phase transitions for filter cleanup
    static DuckingEnvelope::Phase lastPhase = DuckingEnvelope::Phase::Idle;
    DuckingEnvelope::Phase currentPhase = duckingEnvelope.getPhase();
    
    // Reset filter when returning to Idle (end of release cycle)
    if (currentPhase == DuckingEnvelope::Phase::Idle && 
        lastPhase != DuckingEnvelope::Phase::Idle)
    {
        for (int b = 0; b < NUM_BANDS; ++b)
        {
            duckingBands[b].reset();
        }
    }
    lastPhase = currentPhase;
    
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float envelopeLevel = duckingEnvelope.process();
            
            // Clamp envelope
            envelopeLevel = juce::jlimit(0.01f, 1.0f, envelopeLevel);
            
            // gainReduction: 1.0 = full signal, 0.01 = heavily reduced
            float gainReduction = envelopeLevel * mixAmount + (1.0f - mixAmount);
            gainReduction = juce::jlimit(0.01f, 1.0f, gainReduction);
            
            // Original signal, clamped and denormal-flushed
            float output = channelData[sample];
            if (std::abs(output) < 1e-8f) output = 0.0f;
            output = juce::jlimit(-1.0f, 1.0f, output);
            
            // Only process filter during ATTACK phase to avoid filter drift
            // During Release, use gainReduction directly without filter processing
            bool isAttackPhase = (currentPhase == DuckingEnvelope::Phase::Attack);
            
            if (anyBandEnabled && gainReduction < 0.99f && isAttackPhase)
            {
                // Process through single bandpass filter (Attack only)
                float bandComponent = duckingBands[0].bandPassFilter.process(output);
                
                // Clamp band component with denormal flush
                if (std::abs(bandComponent) < 1e-8f) bandComponent = 0.0f;
                bandComponent = juce::jlimit(-1.0f, 1.0f, bandComponent);
                
                // Calculate how much to reduce this band
                float duckAmount = 1.0f - gainReduction;
                duckAmount = juce::jlimit(0.0f, 1.0f, duckAmount);
                
                // Remove the band component and apply ducking
                output = output - (bandComponent * duckAmount);
                
                // Safety clip with headroom
                output = juce::jlimit(-0.99f, 0.99f, output);
            }
            else if (anyBandEnabled && gainReduction < 1.0f && !isAttackPhase)
            {
                // Release phase: apply pure gain reduction without filter processing
                // This prevents filter state drift noise during long releases
                output = output * gainReduction;
                output = juce::jlimit(-0.99f, 0.99f, output);
            }
            
            // Denormal flush on output
            if (std::abs(output) < 1e-8f) output = 0.0f;
            
            // Update the output
            channelData[sample] = output;
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
    
    mos.writeFloat(mixAmount);
    mos.writeFloat(attackTimeMs);
    mos.writeFloat(releaseTimeMs);
    mos.writeInt(midiNoteToTrigger);
    
    // Save band parameters (LEFT/RIGHT frequencies)
    for (int b = 0; b < NUM_BANDS; ++b)
    {
        mos.writeFloat(duckingBands[b].leftFreq);
        mos.writeFloat(duckingBands[b].rightFreq);
        mos.writeBool(duckingBands[b].enabled);
    }
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
        
        duckingEnvelope.setAttackTime(sampleRate, attackTimeMs);
        duckingEnvelope.setReleaseTime(sampleRate, releaseTimeMs);
        
        // Load band parameters if available (LEFT/RIGHT frequencies)
        for (int b = 0; b < NUM_BANDS; ++b)
        {
            if (mis.getPosition() < mis.getTotalLength())
            {
                duckingBands[b].leftFreq = mis.readFloat();
                duckingBands[b].rightFreq = mis.readFloat();
                duckingBands[b].enabled = mis.readBool();
            }
        }
        
        updateBandCoefficients();
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WhiteDuckAudioProcessor();
}
