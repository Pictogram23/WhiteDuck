/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
WhiteDuckAudioProcessorEditor::WhiteDuckAudioProcessorEditor (WhiteDuckAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Mix Slider
    mixSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    mixSlider.setRange(0.0, 1.0, 0.01);
    mixSlider.setValue(audioProcessor.getMixAmount(), juce::dontSendNotification);
    mixSlider.addListener(this);
    addAndMakeVisible(mixSlider);
    
    mixLabel.setText("Mix", juce::dontSendNotification);
    mixLabel.attachToComponent(&mixSlider, true);
    addAndMakeVisible(mixLabel);
    
    // Attack Time Slider
    attackSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    attackSlider.setRange(0.1, 100.0, 0.1);
    attackSlider.setValue(audioProcessor.getAttackTime(), juce::dontSendNotification);
    attackSlider.addListener(this);
    addAndMakeVisible(attackSlider);
    
    attackLabel.setText("Attack (ms)", juce::dontSendNotification);
    attackLabel.attachToComponent(&attackSlider, true);
    addAndMakeVisible(attackLabel);
    
    // Release Time Slider
    releaseSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    releaseSlider.setRange(10.0, 2000.0, 10.0);
    releaseSlider.setValue(audioProcessor.getReleaseTime(), juce::dontSendNotification);
    releaseSlider.addListener(this);
    addAndMakeVisible(releaseSlider);
    
    releaseLabel.setText("Release (ms)", juce::dontSendNotification);
    releaseLabel.attachToComponent(&releaseSlider, true);
    addAndMakeVisible(releaseLabel);
    
    // MIDI Note Slider
    midiNoteSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    midiNoteSlider.setRange(0.0, 127.0, 1.0);
    midiNoteSlider.setValue(static_cast<float>(audioProcessor.getMidiNoteToTrigger()), juce::dontSendNotification);
    midiNoteSlider.addListener(this);
    addAndMakeVisible(midiNoteSlider);
    
    midiNoteLabel.setText("MIDI Note", juce::dontSendNotification);
    midiNoteLabel.attachToComponent(&midiNoteSlider, true);
    addAndMakeVisible(midiNoteLabel);
    
    setSize(500, 250);
}

WhiteDuckAudioProcessorEditor::~WhiteDuckAudioProcessorEditor()
{
}

//==============================================================================
void WhiteDuckAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(16.0f));
    g.drawFittedText("WhiteDuck - Ducking Plugin", getLocalBounds().reduced(10), juce::Justification::topLeft, 1);
}

void WhiteDuckAudioProcessorEditor::resized()
{
    int labelWidth = 100;
    int sliderHeight = 30;
    int yOffset = 40;
    int spacing = 40;
    
    attackSlider.setBounds(labelWidth + 10, yOffset, getWidth() - labelWidth - 20, sliderHeight);
    yOffset += spacing;
    
    releaseSlider.setBounds(labelWidth + 10, yOffset, getWidth() - labelWidth - 20, sliderHeight);
    yOffset += spacing;
    
    mixSlider.setBounds(labelWidth + 10, yOffset, getWidth() - labelWidth - 20, sliderHeight);
    yOffset += spacing;
    
    midiNoteSlider.setBounds(labelWidth + 10, yOffset, getWidth() - labelWidth - 20, sliderHeight);
}

void WhiteDuckAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &mixSlider)
    {
        audioProcessor.setMixAmount(static_cast<float>(mixSlider.getValue()));
    }
    else if (slider == &attackSlider)
    {
        audioProcessor.setAttackTime(static_cast<float>(attackSlider.getValue()));
    }
    else if (slider == &releaseSlider)
    {
        audioProcessor.setReleaseTime(static_cast<float>(releaseSlider.getValue()));
    }
    else if (slider == &midiNoteSlider)
    {
        audioProcessor.setMidiNoteToTrigger(static_cast<int>(midiNoteSlider.getValue()));
    }
}
