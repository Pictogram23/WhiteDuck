/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
// Envelope Generator for ducking
class DuckingEnvelope
{
public:
    enum class Phase { Idle, Attack, Release };
    
    DuckingEnvelope() : currentLevel(1.0f), phase(Phase::Idle) {}
    
    void setAttackTime(double sampleRate, float attackMs)
    {
        if (attackMs > 0.0001f)
            attackCoeff = std::pow(0.01f, 1.0 / (sampleRate * attackMs / 1000.0));
    }
    
    void setReleaseTime(double sampleRate, float releaseMs)
    {
        if (releaseMs > 0.0001f)
            releaseCoeff = std::pow(0.01f, 1.0 / (sampleRate * releaseMs / 1000.0));
    }
    
    void trigger()
    {
        currentLevel = 1.0f;
        phase = Phase::Attack;
    }
    
    float process()
    {
        if (phase == Phase::Attack)
        {
            currentLevel *= attackCoeff;
            if (currentLevel < 0.01f)
            {
                currentLevel = 0.01f;
                phase = Phase::Release;
            }
        }
        else if (phase == Phase::Release)
        {
            currentLevel /= releaseCoeff;
            if (currentLevel > 0.999f)
            {
                currentLevel = 1.0f;
                phase = Phase::Idle;
            }
        }
        return currentLevel;
    }
    
    bool getIsActive() const { return phase != Phase::Idle; }
    float getCurrentLevel() const { return currentLevel; }
    
private:
    float currentLevel;
    Phase phase;
    float attackCoeff = 0.9f;
    float releaseCoeff = 0.99f;
};

//==============================================================================
/**
*/
class WhiteDuckAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    WhiteDuckAudioProcessor();
    ~WhiteDuckAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    // Parameter setters and getters
    void setMixAmount(float newMix) { mixAmount = juce::jlimit(0.0f, 1.0f, newMix); }
    float getMixAmount() const { return mixAmount; }
    
    void setAttackTime(float newAttackMs) 
    { 
        attackTimeMs = newAttackMs;
        duckingEnvelope.setAttackTime(sampleRate, attackTimeMs);
    }
    float getAttackTime() const { return attackTimeMs; }
    
    void setReleaseTime(float newReleaseMs) 
    { 
        releaseTimeMs = newReleaseMs;
        duckingEnvelope.setReleaseTime(sampleRate, releaseTimeMs);
    }
    float getReleaseTime() const { return releaseTimeMs; }
    
    void setMidiNoteToTrigger(int newNote) { midiNoteToTrigger = juce::jlimit(0, 127, newNote); }
    int getMidiNoteToTrigger() const { return midiNoteToTrigger; }
    
    // Default values
    static constexpr float DEFAULT_MIX = 0.5f;
    static constexpr float DEFAULT_ATTACK = 5.0f;
    static constexpr float DEFAULT_RELEASE = 100.0f;
    static constexpr int DEFAULT_MIDI_NOTE = 60;

private:
    //==============================================================================
    // MIDI and Ducking parameters
    DuckingEnvelope duckingEnvelope;
    
    double sampleRate = 44100.0;
    int midiNoteToTrigger = 60; // Middle C by default
    
    // Ducking parameters
    float attackTimeMs = 5.0f;
    float releaseTimeMs = 100.0f;
    float mixAmount = 0.5f; // 0.0 = no ducking, 1.0 = full ducking
    
    // Buffer for processing
    juce::AudioBuffer<float> duckedBuffer;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WhiteDuckAudioProcessor)
};
