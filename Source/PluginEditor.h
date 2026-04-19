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
                                       public juce::Button::Listener,
                                       public juce::ComboBox::Listener,
                                       public juce::Timer
{
public:
    WhiteDuckAudioProcessorEditor (WhiteDuckAudioProcessor&);
    ~WhiteDuckAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;
    void timerCallback() override;
    
    void updateBandUI(int bandIndex);
    void updateModeUI();  // Update UI based on BPM sync mode

private:
    WhiteDuckAudioProcessor& audioProcessor;
    
    // Mode toggle button
    juce::ToggleButton bpmSyncModeButton{"BPM Sync"};
    juce::Label bpmModeLabel;
    
    // Global parameter sliders
    ResetableSlider attackSlider{WhiteDuckAudioProcessor::DEFAULT_ATTACK};
    juce::Label attackLabel;
    
    ResetableSlider releaseSlider{WhiteDuckAudioProcessor::DEFAULT_RELEASE};
    juce::Label releaseLabel;
    
    ResetableSlider mixSlider{WhiteDuckAudioProcessor::DEFAULT_MIX};
    juce::Label mixLabel;
    
    ResetableSlider midiNoteSlider{static_cast<float>(WhiteDuckAudioProcessor::DEFAULT_MIDI_NOTE)};
    juce::Label midiNoteLabel;
    
    // Band parameter controls
    juce::ToggleButton enableLeftButton{"Enable LEFT"};
    juce::ToggleButton enableRightButton{"Enable RIGHT"};
    
    ResetableSlider leftFreqSlider{150.0f};
    juce::Label leftFreqLabel;
    
    ResetableSlider rightFreqSlider{5000.0f};
    juce::Label rightFreqLabel;
    
    // Curve type selection
    juce::ComboBox attackCurveComboBox;
    juce::Label attackCurveLabel;
    
    juce::ComboBox releaseCurveComboBox;
    juce::Label releaseCurveLabel;
    
    juce::ComboBox releaseNoteComboBox;
    juce::Label releaseNoteLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> attackAttachment;
    std::unique_ptr<SliderAttachment> releaseAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<SliderAttachment> midiNoteAttachment;
    std::unique_ptr<SliderAttachment> leftFreqAttachment;
    std::unique_ptr<SliderAttachment> rightFreqAttachment;
    std::unique_ptr<ButtonAttachment> bpmSyncAttachment;
    std::unique_ptr<ButtonAttachment> enableLeftAttachment;
    std::unique_ptr<ButtonAttachment> enableRightAttachment;
    std::unique_ptr<ComboBoxAttachment> attackCurveAttachment;
    std::unique_ptr<ComboBoxAttachment> releaseCurveAttachment;
    std::unique_ptr<ComboBoxAttachment> releaseNoteAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WhiteDuckAudioProcessorEditor)
};
