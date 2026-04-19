/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    template <typename ParameterType>
    void setParameterValue(juce::AudioProcessorValueTreeState& state, const char* paramID, float value)
    {
        if (auto* param = dynamic_cast<ParameterType*>(state.getParameter(paramID)))
            param->setValueNotifyingHost(param->convertTo0to1(value));
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout WhiteDuckAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(PARAM_MIX, "Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), DEFAULT_MIX));
    layout.add(std::make_unique<juce::AudioParameterFloat>(PARAM_ATTACK_MS, "Attack (ms)", juce::NormalisableRange<float>(0.1f, 200.0f, 0.1f), DEFAULT_ATTACK));
    layout.add(std::make_unique<juce::AudioParameterFloat>(PARAM_RELEASE_MS, "Release (ms)", juce::NormalisableRange<float>(10.0f, 2000.0f, 10.0f), DEFAULT_RELEASE));
    layout.add(std::make_unique<juce::AudioParameterInt>(PARAM_MIDI_NOTE, "MIDI Note", 0, 127, DEFAULT_MIDI_NOTE));
    layout.add(std::make_unique<juce::AudioParameterBool>(PARAM_BPM_SYNC, "BPM Sync", false));

    juce::StringArray curveChoices { "Balanced", "Slow", "Fast" };
    layout.add(std::make_unique<juce::AudioParameterChoice>(PARAM_ATTACK_CURVE, "Attack Curve", curveChoices, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(PARAM_RELEASE_CURVE, "Release Curve", curveChoices, 0));

    juce::StringArray releaseNoteChoices
    {
        "64th", "Dotted 64th", "32nd", "Dotted 32nd", "16th", "Dotted 16th",
        "8th", "Dotted 8th", "Quarter", "Dotted Qtr", "Half", "Dotted Half", "Whole"
    };
    layout.add(std::make_unique<juce::AudioParameterChoice>(PARAM_RELEASE_NOTE, "Release Note", releaseNoteChoices, 8));

    layout.add(std::make_unique<juce::AudioParameterBool>(PARAM_BAND_ENABLED, "Band Enabled", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(PARAM_BAND_LEFT_ENABLED, "Left Enabled", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(PARAM_BAND_RIGHT_ENABLED, "Right Enabled", true));
    layout.add(std::make_unique<juce::AudioParameterFloat>(PARAM_BAND_LEFT_FREQ, "Left Freq", juce::NormalisableRange<float>(20.0f, 19999.0f, 1.0f, 0.25f), 150.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(PARAM_BAND_RIGHT_FREQ, "Right Freq", juce::NormalisableRange<float>(21.0f, 20000.0f, 1.0f, 0.25f), 5000.0f));

    return layout;
}

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
                   ),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
#else
    : apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
#endif
{
    // Initialize bands with default values (will be overridden by setStateInformation if state exists)
    initializeBands();
    syncParametersFromState();
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

    syncParametersFromState();
    
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

    if (auto* syncParam = apvts.getRawParameterValue(PARAM_BPM_SYNC))
    {
        bpmSyncEnabled = syncParam->load() > 0.5f;
    }

    if (bpmSyncEnabled)
    {
        if (auto playHead = getPlayHead())
        {
            auto posInfo = playHead->getPosition();
            if (posInfo)
            {
                auto bpmValue = posInfo->getBpm();
                if (bpmValue.hasValue() && *bpmValue > 0)
                    bpmFromDAW = static_cast<float>(*bpmValue);
            }
        }
    }

    syncParametersFromState();

    // Clear output channels that don't have input
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Apply ducking to audio with band processing
    int numSamples = buffer.getNumSamples();

    // Build per-sample trigger map so Attack can start before note-on timing.
    const int attackSamples = juce::jmax(0, static_cast<int>(std::ceil(sampleRate * attackTimeMs / 1000.0)));
    std::vector<bool> triggerAtSample(static_cast<size_t>(numSamples), false);
    std::vector<int> catchUpSamplesAtTrigger(static_cast<size_t>(numSamples), 0);

    for (auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn() && msg.getNoteNumber() == midiNoteToTrigger)
        {
            const int noteOnSample = juce::jlimit(0, numSamples - 1, metadata.samplePosition);
            const int triggerSample = juce::jmax(0, noteOnSample - attackSamples);
            triggerAtSample[static_cast<size_t>(triggerSample)] = true;

            // If trigger had to be clamped to block start, fast-forward Attack to
            // emulate the part that should have happened before this block.
            const int missingSamples = juce::jmax(0, attackSamples - noteOnSample);
            catchUpSamplesAtTrigger[static_cast<size_t>(triggerSample)] =
                juce::jmax(catchUpSamplesAtTrigger[static_cast<size_t>(triggerSample)], missingSamples);
        }
    }

    // Clear MIDI messages - no need to pass them through
    midiMessages.clear();

    // Pre-calculate envelope levels for all samples (for consistent L/R processing)
    std::vector<float> envelopeLevels(numSamples);
    for (int sample = 0; sample < numSamples; ++sample)
    {
        if (triggerAtSample[static_cast<size_t>(sample)])
        {
            duckingEnvelope.trigger();
            duckingEnvelope.advanceAttackSamples(catchUpSamplesAtTrigger[static_cast<size_t>(sample)]);
        }

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
    if (auto xmlState = apvts.copyState().createXml())
    {
        auto xmlString = xmlState->toString();
        destData.append(xmlString.toRawUTF8(), xmlString.getNumBytesAsUTF8());
    }
}

void WhiteDuckAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xmlString = juce::String::fromUTF8(static_cast<const char*>(data), sizeInBytes);
    if (xmlString.isNotEmpty())
    {
        if (auto xmlState = juce::parseXML(xmlString))
        {
            auto newState = juce::ValueTree::fromXml(*xmlState);
            if (newState.isValid())
            {
                apvts.replaceState(newState);
                syncParametersFromState();
                return;
            }
        }
    }

    juce::MemoryInputStream mis(data, sizeInBytes, false);

    if (sizeInBytes < 20)
        return;

    const int version = mis.readInt();

    if (version == 1 || version == 2 || version == 3)
    {
        const float savedMix = mis.readFloat();
        const float savedAttack = mis.readFloat();
        const float savedRelease = mis.readFloat();
        const int savedMidiNote = mis.readInt();

        setParameterValue<juce::RangedAudioParameter>(apvts, PARAM_MIX, savedMix);
        setParameterValue<juce::RangedAudioParameter>(apvts, PARAM_ATTACK_MS, savedAttack);
        setParameterValue<juce::RangedAudioParameter>(apvts, PARAM_RELEASE_MS, savedRelease);
        setParameterValue<juce::RangedAudioParameter>(apvts, PARAM_MIDI_NOTE, static_cast<float>(savedMidiNote));

        if (version == 3)
        {
            if (mis.getPosition() + 16 <= mis.getTotalLength())
            {
                const bool savedSync = mis.readInt() != 0;
                mis.readFloat();
                mis.readInt();
                const int savedReleaseNote = mis.readInt();

                setParameterValue<juce::AudioParameterBool>(apvts, PARAM_BPM_SYNC, savedSync ? 1.0f : 0.0f);
                setParameterValue<juce::AudioParameterChoice>(apvts, PARAM_RELEASE_NOTE, static_cast<float>(savedReleaseNote));
            }
        }

        for (int b = 0; b < NUM_BANDS; ++b)
        {
            const int bytesNeeded = (version == 1) ? 12 : 20;
            if (mis.getPosition() + bytesNeeded <= mis.getTotalLength())
            {
                const float leftFreq = mis.readFloat();
                const float rightFreq = mis.readFloat();
                const bool enabled = mis.readInt() != 0;

                setParameterValue<juce::RangedAudioParameter>(apvts, PARAM_BAND_LEFT_FREQ, leftFreq);
                setParameterValue<juce::RangedAudioParameter>(apvts, PARAM_BAND_RIGHT_FREQ, rightFreq);
                setParameterValue<juce::AudioParameterBool>(apvts, PARAM_BAND_ENABLED, enabled ? 1.0f : 0.0f);

                if (version >= 2)
                {
                    const bool leftEnabled = mis.readInt() != 0;
                    const bool rightEnabled = mis.readInt() != 0;
                    setParameterValue<juce::AudioParameterBool>(apvts, PARAM_BAND_LEFT_ENABLED, leftEnabled ? 1.0f : 0.0f);
                    setParameterValue<juce::AudioParameterBool>(apvts, PARAM_BAND_RIGHT_ENABLED, rightEnabled ? 1.0f : 0.0f);
                }
            }
        }

        syncParametersFromState();
    }
}

void WhiteDuckAudioProcessor::syncParametersFromState()
{
    if (auto* param = apvts.getRawParameterValue(PARAM_MIX))
        mixAmount = param->load();

    if (auto* param = apvts.getRawParameterValue(PARAM_ATTACK_MS))
        attackTimeMs = param->load();

    if (auto* param = apvts.getRawParameterValue(PARAM_RELEASE_MS))
        manualReleaseTimeMs = param->load();

    if (auto* param = apvts.getRawParameterValue(PARAM_MIDI_NOTE))
        midiNoteToTrigger = static_cast<int>(std::round(param->load()));

    if (auto* param = apvts.getRawParameterValue(PARAM_BPM_SYNC))
        bpmSyncEnabled = param->load() > 0.5f;

    if (auto* param = apvts.getRawParameterValue(PARAM_ATTACK_CURVE))
        duckingEnvelope.setAttackCurve(static_cast<DuckingEnvelope::CurveType>(static_cast<int>(std::round(param->load()))));

    if (auto* param = apvts.getRawParameterValue(PARAM_RELEASE_CURVE))
        duckingEnvelope.setReleaseCurve(static_cast<DuckingEnvelope::CurveType>(static_cast<int>(std::round(param->load()))));

    if (auto* param = apvts.getRawParameterValue(PARAM_RELEASE_NOTE))
        releaseNoteValue = static_cast<NoteValue>(static_cast<int>(std::round(param->load())));

    if (auto* param = apvts.getRawParameterValue(PARAM_BAND_ENABLED))
        duckingBands[0].enabled = param->load() > 0.5f;

    if (auto* param = apvts.getRawParameterValue(PARAM_BAND_LEFT_ENABLED))
        duckingBands[0].leftEnabled = param->load() > 0.5f;

    if (auto* param = apvts.getRawParameterValue(PARAM_BAND_RIGHT_ENABLED))
        duckingBands[0].rightEnabled = param->load() > 0.5f;

    if (auto* param = apvts.getRawParameterValue(PARAM_BAND_LEFT_FREQ))
        duckingBands[0].leftFreq = param->load();

    if (auto* param = apvts.getRawParameterValue(PARAM_BAND_RIGHT_FREQ))
        duckingBands[0].rightFreq = param->load();

    if (bpmSyncEnabled)
        releaseTimeMs = calculateTimeFromBpm(releaseNoteValue);
    else
        releaseTimeMs = manualReleaseTimeMs;

    duckingEnvelope.setAttackTime(sampleRate, attackTimeMs);
    duckingEnvelope.setReleaseTime(sampleRate, releaseTimeMs);
    updateBandCoefficients();
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WhiteDuckAudioProcessor();
}
