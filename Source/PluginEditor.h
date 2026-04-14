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
                                       public juce::Slider::Listener,
                                       public juce::Button::Listener
{
public:
    WhiteDuckAudioProcessorEditor (WhiteDuckAudioProcessor&);
    ~WhiteDuckAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;
    
    void updateBandUI(int bandIndex);

private:
    WhiteDuckAudioProcessor& audioProcessor;
    
    // Global parameter sliders
    ResetableSlider attackSlider{WhiteDuckAudioProcessor::DEFAULT_ATTACK};
    juce::Label attackLabel;
    
    ResetableSlider releaseSlider{WhiteDuckAudioProcessor::DEFAULT_RELEASE};
    juce::Label releaseLabel;
    
    ResetableSlider mixSlider{WhiteDuckAudioProcessor::DEFAULT_MIX};
    juce::Label mixLabel;
    
    ResetableSlider midiNoteSlider{static_cast<float>(WhiteDuckAudioProcessor::DEFAULT_MIDI_NOTE)};
    juce::Label midiNoteLabel;
    
    // Band selection buttons (LEFT/RIGHT frequency boundaries)
    juce::TextButton leftBandButton{"LEFT"};
    juce::TextButton rightBandButton{"RIGHT"};
    
    // Band parameter controls
    juce::ToggleButton enableBandButton{"Enable"};
    
    ResetableSlider bandFreqSlider{100.0f};
    juce::Label bandFreqLabel;
    
    int currentBandIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WhiteDuckAudioProcessorEditor)
};
