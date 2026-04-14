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
    
    // Band selection buttons (LEFT/RIGHT frequency boundaries)
    leftBandButton.addListener(this);
    leftBandButton.setColour(juce::TextButton::buttonColourId, juce::Colours::grey);
    addAndMakeVisible(leftBandButton);
    
    rightBandButton.addListener(this);
    rightBandButton.setColour(juce::TextButton::buttonColourId, juce::Colours::grey);
    addAndMakeVisible(rightBandButton);
    
    // Band enable toggle
    enableBandButton.addListener(this);
    addAndMakeVisible(enableBandButton);
    
    // Band frequency slider (LEFT or RIGHT cutoff frequency)
    bandFreqSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    bandFreqSlider.setRange(20.0, 20000.0, 1.0);
    bandFreqSlider.setSkewFactorFromMidPoint(500.0);
    bandFreqSlider.addListener(this);
    addAndMakeVisible(bandFreqSlider);
    bandFreqLabel.setText("Freq", juce::dontSendNotification);
    bandFreqLabel.attachToComponent(&bandFreqSlider, true);
    addAndMakeVisible(bandFreqLabel);
    
    updateBandUI(0);
    setSize(550, 320);
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
    
    // Band selection buttons (LEFT/RIGHT)
    int bandButtonY = yOffset + spacing + 15;
    int bandButtonWidth = 50;
    int bandButtonX = 20;
    leftBandButton.setBounds(bandButtonX, bandButtonY, bandButtonWidth, 25);
    rightBandButton.setBounds(bandButtonX + bandButtonWidth + 10, bandButtonY, bandButtonWidth, 25);
    
    // Band settings
    int bandSettingsY = bandButtonY + 40;
    enableBandButton.setBounds(20, bandSettingsY, 80, 20);
    
    int bandParamY = bandSettingsY + 35;
    bandFreqSlider.setBounds(labelWidth + 10, bandParamY, getWidth() - labelWidth - 20, sliderHeight);
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
    else if (slider == &bandFreqSlider)
    {
        audioProcessor.setBandFrequency(currentBandIndex, static_cast<float>(bandFreqSlider.getValue()));
        
        // Sync UI with processor values in case they were adjusted (LEFT/RIGHT order check)
        updateBandUI(currentBandIndex);
    }
}

void WhiteDuckAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &leftBandButton)
    {
        updateBandUI(0);
    }
    else if (button == &rightBandButton)
    {
        updateBandUI(1);
    }
    else if (button == &enableBandButton)
    {
        audioProcessor.setBandEnabled(currentBandIndex, enableBandButton.getToggleState());
    }
}

void WhiteDuckAudioProcessorEditor::updateBandUI(int bandIndex)
{
    currentBandIndex = bandIndex;
    
    // Update button states (LEFT/RIGHT)
    leftBandButton.setToggleState(bandIndex == 0, juce::dontSendNotification);
    rightBandButton.setToggleState(bandIndex == 1, juce::dontSendNotification);
    
    leftBandButton.setColour(juce::TextButton::buttonColourId, bandIndex == 0 ? juce::Colours::blue : juce::Colours::grey);
    rightBandButton.setColour(juce::TextButton::buttonColourId, bandIndex == 1 ? juce::Colours::blue : juce::Colours::grey);
    
    // Update band parameter sliders
    enableBandButton.setToggleState(audioProcessor.isBandEnabled(bandIndex), juce::dontSendNotification);
    bandFreqSlider.setValue(audioProcessor.getBandFrequency(bandIndex), juce::dontSendNotification);
}
