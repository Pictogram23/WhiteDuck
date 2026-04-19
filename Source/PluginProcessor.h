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
        if (std::abs(z1) < 1e-10f) z1 = 0.0f;
        if (std::abs(z2) < 1e-10f) z2 = 0.0f;
        
        float output = b0 * input + z1;
        z1 = b1 * input - a1 * output + z2;
        z2 = b2 * input - a2 * output;
        
        // Flush filter state on very small values (more aggressive)
        if (std::abs(z1) < 1e-10f) z1 = 0.0f;
        if (std::abs(z2) < 1e-10f) z2 = 0.0f;
        
        // Flush output if denormal
        if (std::abs(output) < 1e-8f) output = 0.0f;
        
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
    // Separate filter instances for left and right channels to prevent cross-channel interference
    BiquadFilter highPassFilter[2];    // [0] = Left channel, [1] = Right channel
    BiquadFilter lowPassFilter[2];     // [0] = Left channel, [1] = Right channel
    float leftFreq = 150.0f;       // Hz - lower frequency boundary (default 150)
    float rightFreq = 5000.0f;     // Hz - upper frequency boundary (default 5000)
    bool enabled = true;           // Band enable toggle
    bool leftEnabled = true;       // LEFT frequency boundary enable
    bool rightEnabled = true;      // RIGHT frequency boundary enable
    
    void reset() 
    { 
        highPassFilter[0].reset();
        highPassFilter[1].reset();
        lowPassFilter[0].reset();
        lowPassFilter[1].reset();
    }
    
    void updateCoefficients(float sampleRate)
    {
        // Determine effective frequency boundaries based on enable flags
        float left = leftFreq;
        float right = rightFreq;
        
        // If LEFT is disabled, set high-pass to 20Hz (allow all low frequencies)
        if (!leftEnabled)
            left = 20.0f;
        
        // If RIGHT is disabled, set low-pass to 20kHz (allow all high frequencies)
        if (!rightEnabled)
            right = 20000.0f;
        
        // Clamp frequencies
        left = juce::jlimit(20.0f, 19999.0f, left);
        right = juce::jlimit(21.0f, 20000.0f, right);
        if (left >= right) right = left + 100.0f;
        
        // ========== HIGH-PASS FILTER (at leftFreq) ==========
        // Use higher Q for low frequencies to avoid instability
        float Q_hp = juce::jlimit(0.5f, 1.0f, 100.0f / left);  // Q increases for lower frequencies
        
        float w0_hp = 2.0f * 3.14159265f * left / sampleRate;
        // Clamp w0 to prevent numerical issues at very low frequencies
        w0_hp = juce::jlimit(0.001f, 3.14f, w0_hp);
        
        float sinW0_hp = std::sin(w0_hp);
        float cosW0_hp = std::cos(w0_hp);
        float alpha_hp = sinW0_hp / (2.0f * Q_hp);
        
        // High-pass coefficients
        float b0_hp = (1.0f + cosW0_hp) / 2.0f;
        float b1_hp = -(1.0f + cosW0_hp);
        float b2_hp = (1.0f + cosW0_hp) / 2.0f;
        float a0_hp = 1.0f + alpha_hp;
        float a1_hp = -2.0f * cosW0_hp;
        float a2_hp = 1.0f - alpha_hp;
        
        // Ensure stability
        if (std::abs(a0_hp) < 1e-6f) a0_hp = 1e-6f;
        // Set coefficients for both channels
        highPassFilter[0].setCoefficients(b0_hp / a0_hp, b1_hp / a0_hp, b2_hp / a0_hp, a1_hp / a0_hp, a2_hp / a0_hp);
        highPassFilter[1].setCoefficients(b0_hp / a0_hp, b1_hp / a0_hp, b2_hp / a0_hp, a1_hp / a0_hp, a2_hp / a0_hp);
        
        // ========== LOW-PASS FILTER (at rightFreq) ==========
        // Use higher Q for low frequencies to avoid instability
        float Q_lp = juce::jlimit(0.5f, 1.0f, 100.0f / right);  // Q increases for lower frequencies
        
        float w0_lp = 2.0f * 3.14159265f * right / sampleRate;
        // Clamp w0 to prevent numerical issues
        w0_lp = juce::jlimit(0.001f, 3.14f, w0_lp);
        
        float sinW0_lp = std::sin(w0_lp);
        float cosW0_lp = std::cos(w0_lp);
        float alpha_lp = sinW0_lp / (2.0f * Q_lp);
        
        // Low-pass coefficients
        float b0_lp = (1.0f - cosW0_lp) / 2.0f;
        float b1_lp = 1.0f - cosW0_lp;
        float b2_lp = (1.0f - cosW0_lp) / 2.0f;
        float a0_lp = 1.0f + alpha_lp;
        float a1_lp = -2.0f * cosW0_lp;
        float a2_lp = 1.0f - alpha_lp;
        
        // Ensure stability
        if (std::abs(a0_lp) < 1e-6f) a0_lp = 1e-6f;
        // Set coefficients for both channels
        lowPassFilter[0].setCoefficients(b0_lp / a0_lp, b1_lp / a0_lp, b2_lp / a0_lp, a1_lp / a0_lp, a2_lp / a0_lp);
        lowPassFilter[1].setCoefficients(b0_lp / a0_lp, b1_lp / a0_lp, b2_lp / a0_lp, a1_lp / a0_lp, a2_lp / a0_lp);
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
        // Do NOT reset currentLevel to 1.0 - maintain continuity
        // If already in Release, re-trigger Attack from current level
        // If in Idle, start from 1.0
        if (phase == Phase::Idle)
        {
            currentLevel = 1.0f;
        }
        // If already in Attack or Release, just switch to Attack
        // without resetting level - ensures smooth transition
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

    void advanceAttackSamples(int numSamples)
    {
        if (numSamples <= 0)
            return;

        for (int i = 0; i < numSamples; ++i)
        {
            if (phase != Phase::Attack)
                break;

            currentLevel -= attackStep;
            if (currentLevel < 0.01f)
            {
                currentLevel = 0.01f;
                phase = Phase::Release;
                break;
            }
        }

        if (std::abs(currentLevel) < 1e-10f)
            currentLevel = 1.0f;
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
    
    // Note value enum for BPM sync (must be before BPM sync methods)
    enum class NoteValue {
        SixtyFourth = 0,         // 64分音符 (0.0625 beat)
        DottedSixtyfourth = 1,   // 付点64分音符 (0.09375 beat)
        ThirtySecond = 2,        // 32分音符 (0.125 beat)
        DottedThirtySecond = 3,  // 付点32分音符 (0.1875 beat)
        Sixteenth = 4,           // 16分音符 (0.25 beat)
        DottedSixteenth = 5,     // 付点16分音符 (0.375 beat)
        Eighth = 6,              // 8分音符 (0.5 beat)
        DottedEighth = 7,        // 付点8分音符 (0.75 beat)
        Quarter = 8,             // 4分音符 (1 beat)
        DottedQuarter = 9,       // 付点4分音符 (1.5 beat)
        Half = 10,               // 2分音符 (2 beat)
        DottedHalf = 11,         // 付点2分音符 (3 beat)
        Whole = 12               // 全音符 (4 beat)
    };
    
    // Calculate milliseconds from BPM and note value
    float calculateTimeFromBpm(NoteValue noteValue) const
    {
        // 1 beat (quarter note) = 60000 / BPM milliseconds
        // Use DAW BPM if available, otherwise use default
        float beatMs = 60000.0f / bpmFromDAW;
        
        float beatLength = 0.0f;
        switch (noteValue)
        {
            case NoteValue::SixtyFourth:     beatLength = 0.0625f; break;   // 64分音符
            case NoteValue::DottedSixtyfourth: beatLength = 0.09375f; break; // 付点64分音符
            case NoteValue::ThirtySecond:   beatLength = 0.125f; break;    // 32分音符
            case NoteValue::DottedThirtySecond: beatLength = 0.1875f; break; // 付点32分音符
            case NoteValue::Sixteenth:      beatLength = 0.25f; break;    // 16分音符
            case NoteValue::DottedSixteenth: beatLength = 0.375f; break;   // 付点16分音符
            case NoteValue::Eighth:         beatLength = 0.5f; break;     // 8分音符
            case NoteValue::DottedEighth:   beatLength = 0.75f; break;    // 付点8分音符
            case NoteValue::Quarter:        beatLength = 1.0f; break;     // 4分音符
            case NoteValue::DottedQuarter:  beatLength = 1.5f; break;    // 付点4分音符
            case NoteValue::Half:           beatLength = 2.0f; break;     // 2分音符
            case NoteValue::DottedHalf:     beatLength = 3.0f; break;    // 付点2分音符
            case NoteValue::Whole:          beatLength = 4.0f; break;     // 全音符
            default:                        beatLength = 1.0f; break;
        }
        
        return beatMs * beatLength;
    }
    
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
    
    // BPM sync mode toggle
    void setBpmSyncMode(bool enabled)
    {
        bpmSyncEnabled = enabled;
    }
    bool getBpmSyncMode() const { return bpmSyncEnabled; }
    
    // Diagnostic: Check if bandpass filter is working
    
    // Band parameter accessors (LEFT=0, RIGHT=1)
    float getBandFrequency(int bandIndex, bool isLeftFreq = true) const
    {
        if (bandIndex >= 0 && bandIndex < NUM_BANDS)
        {
            return isLeftFreq ? duckingBands[bandIndex].leftFreq : duckingBands[bandIndex].rightFreq;
        }
        return 100.0f;
    }
    
    bool isBandEnabled(int bandIndex) const
    {
        if (bandIndex >= 0 && bandIndex < NUM_BANDS)
            return duckingBands[bandIndex].enabled;
        return false;
    }
    
    void setBandFrequency(int bandIndex, float freq, bool isLeftFreq = true)
    {
        if (bandIndex >= 0 && bandIndex < NUM_BANDS)
        {
            if (isLeftFreq)
            {
                // Setting LEFT (lower) frequency boundary
                duckingBands[bandIndex].leftFreq = juce::jlimit(20.0f, 19999.0f, freq);
                // Ensure LEFT < RIGHT
                if (duckingBands[bandIndex].leftFreq >= duckingBands[bandIndex].rightFreq)
                    duckingBands[bandIndex].rightFreq = duckingBands[bandIndex].leftFreq + 100.0f;
            }
            else
            {
                // Setting RIGHT (upper) frequency boundary
                duckingBands[bandIndex].rightFreq = juce::jlimit(21.0f, 20000.0f, freq);
                // Ensure RIGHT > LEFT
                if (duckingBands[bandIndex].rightFreq <= duckingBands[bandIndex].leftFreq)
                    duckingBands[bandIndex].leftFreq = duckingBands[bandIndex].rightFreq - 100.0f;
            }
            updateBandCoefficients();
        }
    }
    
    void setBandEnabled(int bandIndex, bool enable)
    {
        if (bandIndex >= 0 && bandIndex < NUM_BANDS)
            duckingBands[bandIndex].enabled = enable;
    }
    
    bool isBandLeftEnabled(int bandIndex) const
    {
        if (bandIndex >= 0 && bandIndex < NUM_BANDS)
            return duckingBands[bandIndex].leftEnabled;
        return true;
    }
    
    void setBandLeftEnabled(int bandIndex, bool enable)
    {
        if (bandIndex >= 0 && bandIndex < NUM_BANDS)
        {
            duckingBands[bandIndex].leftEnabled = enable;
            updateBandCoefficients();
        }
    }
    
    bool isBandRightEnabled(int bandIndex) const
    {
        if (bandIndex >= 0 && bandIndex < NUM_BANDS)
            return duckingBands[bandIndex].rightEnabled;
        return true;
    }
    
    void setBandRightEnabled(int bandIndex, bool enable)
    {
        if (bandIndex >= 0 && bandIndex < NUM_BANDS)
        {
            duckingBands[bandIndex].rightEnabled = enable;
            updateBandCoefficients();
        }
    }
    
    // BPM is automatically synced from DAW - always enabled
    void updateAttackReleaseFromDawBpm()
    {
        // Called from processBlock() with current DAW BPM
        attackTimeMs = calculateTimeFromBpm(attackNoteValue);
        releaseTimeMs = calculateTimeFromBpm(releaseNoteValue);
        duckingEnvelope.setAttackTime(sampleRate, attackTimeMs);
        duckingEnvelope.setReleaseTime(sampleRate, releaseTimeMs);
    }
    
    void setAttackNoteValue(int noteIndex)
    {
        attackNoteValue = static_cast<NoteValue>(juce::jlimit(0, 12, noteIndex));
        if (bpmSyncEnabled)
        {
            attackTimeMs = calculateTimeFromBpm(attackNoteValue);
            duckingEnvelope.setAttackTime(sampleRate, attackTimeMs);
        }
    }
    
    void setReleaseNoteValue(int noteIndex)
    {
        releaseNoteValue = static_cast<NoteValue>(juce::jlimit(0, 12, noteIndex));
        if (bpmSyncEnabled)
        {
            releaseTimeMs = calculateTimeFromBpm(releaseNoteValue);
            duckingEnvelope.setReleaseTime(sampleRate, releaseTimeMs);
        }
    }
    
    int getAttackNoteValue() const { return static_cast<int>(attackNoteValue); }
    int getReleaseNoteValue() const { return static_cast<int>(releaseNoteValue); }
    

    
    // Band initialization and coefficient update
    void initializeBands()
    {
        // Single band with LEFT/RIGHT frequency boundaries
        duckingBands[0].leftFreq = 150.0f;
        duckingBands[0].rightFreq = 5000.0f;
        duckingBands[0].enabled = true;
        duckingBands[0].leftEnabled = true;
        duckingBands[0].rightEnabled = true;
        
        // Band 1 is unused (kept for potential expansion)
        duckingBands[1].enabled = false;
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
    static constexpr float DEFAULT_BPM = 120.0f;

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
    
    // BPM sync parameters
    bool bpmSyncEnabled = false;      // Toggle between seconds (false) and BPM sync (true)
    float bpmFromDAW = DEFAULT_BPM;   // Current BPM from DAW
    NoteValue attackNoteValue = NoteValue::Sixteenth;   // Default: 16分音符
    NoteValue releaseNoteValue = NoteValue::Quarter;    // Default: 4分音符
    
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
