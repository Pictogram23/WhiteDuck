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
    // Initialize bands with default values (will be overridden by setStateInformation if state exists)
    initializeBands();
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
    
    // Update filter coefficients with current band settings
    // (initializeBands is called only in constructor to preserve loaded state)
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

    // Get BPM from DAW if BPM sync mode is enabled
    if (bpmSyncEnabled)
    {
        if (auto playHead = getPlayHead())
        {
            auto posInfo = playHead->getPosition();
            if (posInfo)
            {
                auto bpmValue = posInfo->getBpm();
                if (bpmValue.hasValue() && *bpmValue > 0)
                {
                    bpmFromDAW = static_cast<float>(*bpmValue);
                    // Update attack/release times based on current DAW BPM
                    updateAttackReleaseFromDawBpm();
                }
            }
        }
    }

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
    
    // Pre-calculate envelope levels for all samples (for consistent L/R processing)
    std::vector<float> envelopeLevels(numSamples);
    for (int sample = 0; sample < numSamples; ++sample)
    {
        envelopeLevels[sample] = duckingEnvelope.process();
    }
    
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
    
    
    // Track phase transitions (for debugging/future use)
    static DuckingEnvelope::Phase lastPhase = DuckingEnvelope::Phase::Idle;
    DuckingEnvelope::Phase currentPhase = duckingEnvelope.getPhase();
    
    // Do NOT reset filter on phase transitions - maintain smooth ducking
    // even when Attack and Release overlap. This prevents clicks/artifacts.
    lastPhase = currentPhase;
    
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float envelopeLevel = envelopeLevels[sample];
            
            // Clamp envelope
            envelopeLevel = juce::jlimit(0.01f, 1.0f, envelopeLevel);
            
            // gainReduction: 1.0 = full signal, 0.01 = heavily reduced
            float gainReduction = envelopeLevel * mixAmount + (1.0f - mixAmount);
            gainReduction = juce::jlimit(0.01f, 1.0f, gainReduction);
            
            // Original signal, clamped and denormal-flushed
            float output = channelData[sample];
            if (std::abs(output) < 1e-8f) output = 0.0f;
            output = juce::jlimit(-1.0f, 1.0f, output);
            
            // Frequency-selective ducking: only LEFT/RIGHT band gets reduced
            // Both Attack and Release process the band through both filters (in series)
            if (anyBandEnabled && gainReduction < 0.99f)
            {
                // Denormal flush on input
                if (std::abs(output) < 1e-10f) output = 0.0f;
                
                // Extract the band component (LEFT to RIGHT frequencies only)
                // Apply HIGH-PASS filter first (removes frequencies below LEFT)
                float bandComponent = duckingBands[0].highPassFilter[channel].process(output);
                
                // Aggressive denormal flush between filters
                if (std::abs(bandComponent) < 1e-10f) bandComponent = 0.0f;
                
                // Then apply LOW-PASS filter (removes frequencies above RIGHT)
                bandComponent = duckingBands[0].lowPassFilter[channel].process(bandComponent);
                
                // Clamp band component with denormal flush
                if (std::abs(bandComponent) < 1e-10f) bandComponent = 0.0f;
                bandComponent = juce::jlimit(-1.0f, 1.0f, bandComponent);
                
                // Calculate how much to reduce this band
                float duckAmount = 1.0f - gainReduction;
                duckAmount = juce::jlimit(0.0f, 1.0f, duckAmount);
                
                // Reduce only the band component
                // Everything outside LEFT/RIGHT remains unaffected
                output = output - (bandComponent * duckAmount);
                
                // Safety clip with headroom
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
    
    // Save version number
    mos.writeInt(3);  // Version 3 - includes BPM sync parameters
    
    // Global parameters
    mos.writeFloat(mixAmount);
    mos.writeFloat(attackTimeMs);
    mos.writeFloat(releaseTimeMs);
    mos.writeInt(midiNoteToTrigger);
    
    // BPM Sync parameters (v3)
    mos.writeInt(bpmSyncEnabled ? 1 : 0);  // Save mode toggle state
    mos.writeFloat(DEFAULT_BPM);  // Placeholder, actual BPM comes from DAW at runtime
    mos.writeInt(static_cast<int>(attackNoteValue));
    mos.writeInt(static_cast<int>(releaseNoteValue));
    
    // Band parameters - use int (0/1) instead of bool for consistency
    for (int b = 0; b < NUM_BANDS; ++b)
    {
        mos.writeFloat(duckingBands[b].leftFreq);
        mos.writeFloat(duckingBands[b].rightFreq);
        mos.writeInt(duckingBands[b].enabled ? 1 : 0);
        mos.writeInt(duckingBands[b].leftEnabled ? 1 : 0);
        mos.writeInt(duckingBands[b].rightEnabled ? 1 : 0);
    }
}

void WhiteDuckAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::MemoryInputStream mis(data, sizeInBytes, false);
    
    if (sizeInBytes < 20)  // Minimum: version(4) + mix(4) + attack(4) + release(4) + midi(4)
        return;
    
    int version = mis.readInt();
    
    if (version == 1 || version == 2 || version == 3)
    {
        mixAmount = mis.readFloat();
        attackTimeMs = mis.readFloat();
        releaseTimeMs = mis.readFloat();
        midiNoteToTrigger = mis.readInt();
        
        duckingEnvelope.setAttackTime(sampleRate, attackTimeMs);
        duckingEnvelope.setReleaseTime(sampleRate, releaseTimeMs);
        
        // Load BPM Sync parameters (v3 only)
        if (version == 3)
        {
            if (mis.getPosition() + 16 <= mis.getTotalLength())  // 4 ints = 16 bytes
            {
                bpmSyncEnabled = mis.readInt() != 0;  // Load mode toggle state
                float savedBpm = mis.readFloat();  // Load placeholder (will be overridden by DAW BPM)
                attackNoteValue = static_cast<NoteValue>(mis.readInt());
                releaseNoteValue = static_cast<NoteValue>(mis.readInt());
                // Update times with current settings
                if (bpmSyncEnabled)
                {
                    updateAttackReleaseFromDawBpm();
                }
            }
        }
        
        // Load band parameters
        for (int b = 0; b < NUM_BANDS; ++b)
        {
            // Version 2+ requires 20 bytes per band (leftFreq + rightFreq + enabled + leftEnabled + rightEnabled)
            int bytesNeeded = (version == 1) ? 12 : 20;
            if (mis.getPosition() + bytesNeeded <= mis.getTotalLength())
            {
                duckingBands[b].leftFreq = mis.readFloat();
                duckingBands[b].rightFreq = mis.readFloat();
                duckingBands[b].enabled = mis.readInt() != 0;
                
                if (version >= 2)
                {
                    duckingBands[b].leftEnabled = mis.readInt() != 0;
                    duckingBands[b].rightEnabled = mis.readInt() != 0;
                }
            }
        }
        
        // Only update coefficients if sampleRate is valid (will be called again in prepareToPlay)
        if (sampleRate > 1000.0)  // Sanity check - sample rate should be at least 44100
        {
            updateBandCoefficients();
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WhiteDuckAudioProcessor();
}
