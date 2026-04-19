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
    // Global Sliders
    attackSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    attackSlider.setRange(0.1, 100.0, 0.1);
    attackSlider.setValue(audioProcessor.getAttackTime(), juce::dontSendNotification);
    attackSlider.addListener(this);
    addAndMakeVisible(attackSlider);
    attackLabel.setText("Attack", juce::dontSendNotification);
    attackLabel.attachToComponent(&attackSlider, true);
    addAndMakeVisible(attackLabel);
    
    releaseSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    releaseSlider.setRange(10.0, 2000.0, 10.0);
    releaseSlider.setValue(audioProcessor.getReleaseTime(), juce::dontSendNotification);
    releaseSlider.addListener(this);
    addAndMakeVisible(releaseSlider);
    releaseLabel.setText("Release", juce::dontSendNotification);
    releaseLabel.attachToComponent(&releaseSlider, true);
    addAndMakeVisible(releaseLabel);
    
    mixSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    mixSlider.setRange(0.0, 1.0, 0.01);
    mixSlider.setValue(audioProcessor.getMixAmount(), juce::dontSendNotification);
    mixSlider.addListener(this);
    addAndMakeVisible(mixSlider);
    mixLabel.setText("Mix", juce::dontSendNotification);
    mixLabel.attachToComponent(&mixSlider, true);
    addAndMakeVisible(mixLabel);
    
    midiNoteSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    midiNoteSlider.setRange(0.0, 127.0, 1.0);
    midiNoteSlider.setValue(static_cast<float>(audioProcessor.getMidiNoteToTrigger()), juce::dontSendNotification);
    midiNoteSlider.addListener(this);
    addAndMakeVisible(midiNoteSlider);
    midiNoteLabel.setText("MIDI", juce::dontSendNotification);
    midiNoteLabel.attachToComponent(&midiNoteSlider, true);
    addAndMakeVisible(midiNoteLabel);
    
    // Band enable toggles (separate for LEFT and RIGHT)
    enableLeftButton.addListener(this);
    addAndMakeVisible(enableLeftButton);
    
    enableRightButton.addListener(this);
    addAndMakeVisible(enableRightButton);
    
    // LEFT frequency slider
    leftFreqSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    leftFreqSlider.setRange(20.0, 19999.0, 1.0);
    leftFreqSlider.setSkewFactorFromMidPoint(500.0);
    leftFreqSlider.setValue(audioProcessor.getBandFrequency(0, true), juce::dontSendNotification);
    leftFreqSlider.addListener(this);
    addAndMakeVisible(leftFreqSlider);
    leftFreqLabel.setText("LEFT", juce::dontSendNotification);
    leftFreqLabel.attachToComponent(&leftFreqSlider, true);
    addAndMakeVisible(leftFreqLabel);
    
    // RIGHT frequency slider
    rightFreqSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    rightFreqSlider.setRange(21.0, 20000.0, 1.0);
    rightFreqSlider.setSkewFactorFromMidPoint(5000.0);
    rightFreqSlider.setValue(audioProcessor.getBandFrequency(0, false), juce::dontSendNotification);
    rightFreqSlider.addListener(this);
    addAndMakeVisible(rightFreqSlider);
    rightFreqLabel.setText("RIGHT", juce::dontSendNotification);
    rightFreqLabel.attachToComponent(&rightFreqSlider, true);
    addAndMakeVisible(rightFreqLabel);
    
    // Update UI with current band state
    enableLeftButton.setToggleState(audioProcessor.isBandLeftEnabled(0), juce::dontSendNotification);
    enableRightButton.setToggleState(audioProcessor.isBandRightEnabled(0), juce::dontSendNotification);
    setSize(550, 310);
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
    int labelWidth = 70;
    int sliderHeight = 25;
    int borderY = 35;
    int spacing = 28;
    
    // Global parameters section
    int yOffset = borderY;
    attackSlider.setBounds(labelWidth + 10, yOffset, getWidth() - labelWidth - 20, sliderHeight);
    yOffset += spacing;
    
    releaseSlider.setBounds(labelWidth + 10, yOffset, getWidth() - labelWidth - 20, sliderHeight);
    yOffset += spacing;
    
    mixSlider.setBounds(labelWidth + 10, yOffset, getWidth() - labelWidth - 20, sliderHeight);
    yOffset += spacing;
    
    midiNoteSlider.setBounds(labelWidth + 10, yOffset, getWidth() - labelWidth - 20, sliderHeight);
    
    // Band controls - compact layout
    int bandSettingsY = yOffset + spacing + 10;
    enableLeftButton.setBounds(20, bandSettingsY, 130, 20);
    enableRightButton.setBounds(160, bandSettingsY, 130, 20);
    
    int leftFreqY = bandSettingsY + 30;
    leftFreqSlider.setBounds(labelWidth + 10, leftFreqY, getWidth() - labelWidth - 20, sliderHeight);
    
    int rightFreqY = leftFreqY + spacing;
    rightFreqSlider.setBounds(labelWidth + 10, rightFreqY, getWidth() - labelWidth - 20, sliderHeight);
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
    else if (slider == &leftFreqSlider)
    {
        audioProcessor.setBandFrequency(0, static_cast<float>(leftFreqSlider.getValue()), true);
    }
    else if (slider == &rightFreqSlider)
    {
        audioProcessor.setBandFrequency(0, static_cast<float>(rightFreqSlider.getValue()), false);
    }
}

void WhiteDuckAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &enableLeftButton)
    {
        audioProcessor.setBandLeftEnabled(0, enableLeftButton.getToggleState());
    }
    else if (button == &enableRightButton)
    {
        audioProcessor.setBandRightEnabled(0, enableRightButton.getToggleState());
    }
}

void WhiteDuckAudioProcessorEditor::updateBandUI(int bandIndex)
{
    // Placeholder for future use
}
