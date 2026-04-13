/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Custom Slider with Ctrl+Click reset functionality
class ResetableSlider : public juce::Slider
{
public:
    ResetableSlider(float defaultValue) : defaultValue(defaultValue) {}
    
    void mouseDown(const juce::MouseEvent& event) override
    {
        if (event.mods.isCtrlDown())
        {
            setValue(defaultValue);
        }
        else
        {
            juce::Slider::mouseDown(event);
        }
    }
    
private:
    float defaultValue;
};

//==============================================================================
/**
*/
class WhiteDuckAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                       public juce::Slider::Listener
{
public:
    WhiteDuckAudioProcessorEditor (WhiteDuckAudioProcessor&);
    ~WhiteDuckAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    void sliderValueChanged(juce::Slider* slider) override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    WhiteDuckAudioProcessor& audioProcessor;
    
    // UI Components with reset functionality
    ResetableSlider mixSlider{WhiteDuckAudioProcessor::DEFAULT_MIX};
    juce::Label mixLabel;
    
    ResetableSlider attackSlider{WhiteDuckAudioProcessor::DEFAULT_ATTACK};
    juce::Label attackLabel;
    
    ResetableSlider releaseSlider{WhiteDuckAudioProcessor::DEFAULT_RELEASE};
    juce::Label releaseLabel;
    
    ResetableSlider midiNoteSlider{static_cast<float>(WhiteDuckAudioProcessor::DEFAULT_MIDI_NOTE)};
    juce::Label midiNoteLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WhiteDuckAudioProcessorEditor)
};
