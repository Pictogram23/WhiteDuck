/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
// Simple Biquad Filter with aggressive Denormal protection
class BiquadFilter
{
public:
    void setCoefficients(float b0, float b1, float b2, float a1, float a2)
    {
        this->b0 = b0; this->b1 = b1; this->b2 = b2;
        this->a1 = a1; this->a2 = a2;
    }
    
    float process(float input)
    {
        // Aggressive denormal flushing
        if (std::abs(input) < 1e-8f) input = 0.0f;
        if (std::abs(z1) < 1e-8f) z1 = 0.0f;
        if (std::abs(z2) < 1e-8f) z2 = 0.0f;
        
        float output = b0 * input + z1;
        z1 = b1 * input - a1 * output + z2;
        z2 = b2 * input - a2 * output;
        
        // Flush filter state on very small values
        if (std::abs(z1) < 1e-8f) z1 = 0.0f;
        if (std::abs(z2) < 1e-8f) z2 = 0.0f;
        
        // Clip output to prevent extreme values
        output = juce::jlimit(-1.0f, 1.0f, output);
        
        return output;
    }
    
    void reset() { z1 = z2 = 0.0f; }
    
private:
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;
};

//==============================================================================
// Band Processing - Two-frequency band (LEFT/RIGHT)
struct DuckingBand
{
    BiquadFilter bandPassFilter;   // Bandpass filter between leftFreq and rightFreq
    float leftFreq = 100.0f;       // Hz - lower frequency boundary
    float rightFreq = 5000.0f;     // Hz - upper frequency boundary
    bool enabled = true;
    
    void reset() 
    { 
        bandPassFilter.reset();
    }
    
    void updateCoefficients(float sampleRate)
    {
        // Clamp frequencies
        float left = juce::jlimit(20.0f, 19999.0f, leftFreq);
        float right = juce::jlimit(21.0f, 20000.0f, rightFreq);
        if (left >= right) right = left + 100.0f;
        
        // Calculate center frequency and bandwidth
        float centerFreq = (left + right) / 2.0f;
        float bandwidth = right - left;
        
        // Q from center frequency and bandwidth
        float q = centerFreq / bandwidth;
        
        // If bandwidth is very wide (>95% of audible spectrum), bypass filter
        // to avoid numerical instability
        if (bandwidth > 19000.0f)
        {
            // Passthrough filter (b0=1, b1=0, b2=0, a1=0, a2=0)
            bandPassFilter.setCoefficients(1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            return;
        }
        
        // Clamp Q for stability
        q = juce::jlimit(0.5f, 2.0f, q);
        
        // Bandpass filter coefficients (constant-skirt style)
        float w0 = 2.0f * 3.14159265f * centerFreq / sampleRate;
        float sinW0 = std::sin(w0);
        float cosW0 = std::cos(w0);
        float alpha = sinW0 / (2.0f * q);
        
        // Bandpass: b0=alpha, b1=0, b2=-alpha
        float b0 = alpha;
        float b1 = 0.0f;
        float b2 = -alpha;
        float a0 = 1.0f + alpha;
        float a1 = -2.0f * cosW0;
        float a2 = 1.0f - alpha;
        
        // Ensure a0 is not zero
        if (std::abs(a0) < 1e-10f) a0 = 1.0f;
        
        bandPassFilter.setCoefficients(b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0);
    }
};

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
        {
            // Linear envelope: how much to decrease per sample
            attackStep = 0.99f / (sampleRate * attackMs / 1000.0f);
        }
    }
    
    void setReleaseTime(double sampleRate, float releaseMs)
    {
        if (releaseMs > 0.0001f)
        {
            // Linear envelope: how much to increase per sample
            releaseStep = 0.99f / (sampleRate * releaseMs / 1000.0f);
        }
    }
    
    void trigger()
    {
        currentLevel = 1.0f;
        phase = Phase::Attack;
    }
    
    float process()
    {
        Phase prevPhase = phase;
        
        if (phase == Phase::Attack)
        {
            currentLevel -= attackStep;
            if (currentLevel < 0.01f)
            {
                currentLevel = 0.01f;
                phase = Phase::Release;
            }
        }
        else if (phase == Phase::Release)
        {
            currentLevel += releaseStep;
            if (currentLevel > 0.999f)
            {
                currentLevel = 1.0f;
                phase = Phase::Idle;
            }
        }
        
        // Flush denormals
        if (std::abs(currentLevel) < 1e-10f) 
            currentLevel = 1.0f;
        
        return currentLevel;
    }
    
    bool getIsActive() const { return phase != Phase::Idle; }
    float getCurrentLevel() const { return currentLevel; }
    Phase getPhase() const { return phase; }
    
private:
    float currentLevel;
    Phase phase;
    float attackStep = 0.0001f;
    float releaseStep = 0.0001f;
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
    
    // Band parameter accessors (LEFT=0, RIGHT=1)
    float getBandFrequency(int bandIndex) const
    {
        if (bandIndex >= 0 && bandIndex < NUM_BANDS)
        {
            if (bandIndex == 0)  // LEFT
                return duckingBands[0].leftFreq;
            else                 // RIGHT
                return duckingBands[1].rightFreq;
        }
        return 100.0f;
    }
    
    bool isBandEnabled(int bandIndex) const
    {
        if (bandIndex >= 0 && bandIndex < NUM_BANDS)
            return duckingBands[bandIndex].enabled;
        return false;
    }
    
    void setBandFrequency(int bandIndex, float freq)
    {
        if (bandIndex >= 0 && bandIndex < NUM_BANDS)
        {
            if (bandIndex == 0)  // LEFT
            {
                duckingBands[0].leftFreq = juce::jlimit(20.0f, 19999.0f, freq);
                if (duckingBands[0].leftFreq >= duckingBands[1].rightFreq)
                    duckingBands[1].rightFreq = duckingBands[0].leftFreq + 100.0f;
            }
            else  // RIGHT
            {
                duckingBands[1].rightFreq = juce::jlimit(21.0f, 20000.0f, freq);
                if (duckingBands[1].rightFreq <= duckingBands[0].leftFreq)
                    duckingBands[0].leftFreq = duckingBands[1].rightFreq - 100.0f;
            }
            updateBandCoefficients();
        }
    }
    
    void setBandEnabled(int bandIndex, bool enable)
    {
        if (bandIndex >= 0 && bandIndex < NUM_BANDS)
            duckingBands[bandIndex].enabled = enable;
    }
    
    // Band initialization and coefficient update
    void initializeBands()
    {
        // LEFT: 150 Hz cutoff (lower boundary)
        duckingBands[0].leftFreq = 150.0f;
        duckingBands[0].rightFreq = 5000.0f;
        duckingBands[0].enabled = true;
        
        // RIGHT: 5000 Hz cutoff (upper boundary)
        duckingBands[1].leftFreq = 150.0f;
        duckingBands[1].rightFreq = 5000.0f;
        duckingBands[1].enabled = true;
    }
    
    void updateBandCoefficients()
    {
        for (int b = 0; b < NUM_BANDS; ++b)
        {
            duckingBands[b].updateCoefficients(sampleRate);
        }
    }
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
    
    // Band ducking parameters
    static constexpr int NUM_BANDS = 2;
    DuckingBand duckingBands[NUM_BANDS];
    
    // For state saving
    int activeBandIndex = 0;
    
    // Buffer for processing
    juce::AudioBuffer<float> duckedBuffer;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WhiteDuckAudioProcessor)
};
