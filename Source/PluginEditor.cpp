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
    // Mode toggle button
    bpmSyncModeButton.setToggleState(audioProcessor.getBpmSyncMode(), juce::dontSendNotification);
    bpmSyncModeButton.addListener(this);
    addAndMakeVisible(bpmSyncModeButton);
    bpmModeLabel.setText("Sync Mode:", juce::dontSendNotification);
    addAndMakeVisible(bpmModeLabel);
    
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
    
    // Attack note value combo box
    attackNoteComboBox.addItem("64th", 1);
    attackNoteComboBox.addItem("Dotted 64th", 2);
    attackNoteComboBox.addItem("32nd", 3);
    attackNoteComboBox.addItem("Dotted 32nd", 4);
    attackNoteComboBox.addItem("16th", 5);
    attackNoteComboBox.addItem("Dotted 16th", 6);
    attackNoteComboBox.addItem("8th", 7);
    attackNoteComboBox.addItem("Dotted 8th", 8);
    attackNoteComboBox.addItem("Quarter", 9);
    attackNoteComboBox.addItem("Dotted Qtr", 10);
    attackNoteComboBox.addItem("Half", 11);
    attackNoteComboBox.addItem("Dotted Half", 12);
    attackNoteComboBox.addItem("Whole", 13);
    attackNoteComboBox.setSelectedId(audioProcessor.getAttackNoteValue() + 1, juce::dontSendNotification);
    attackNoteComboBox.addListener(this);
    addAndMakeVisible(attackNoteComboBox);
    attackNoteLabel.setText("Note", juce::dontSendNotification);
    attackNoteLabel.attachToComponent(&attackNoteComboBox, true);
    addAndMakeVisible(attackNoteLabel);
    
    // Release note value combo box
    releaseNoteComboBox.addItem("64th", 1);
    releaseNoteComboBox.addItem("Dotted 64th", 2);
    releaseNoteComboBox.addItem("32nd", 3);
    releaseNoteComboBox.addItem("Dotted 32nd", 4);
    releaseNoteComboBox.addItem("16th", 5);
    releaseNoteComboBox.addItem("Dotted 16th", 6);
    releaseNoteComboBox.addItem("8th", 7);
    releaseNoteComboBox.addItem("Dotted 8th", 8);
    releaseNoteComboBox.addItem("Quarter", 9);
    releaseNoteComboBox.addItem("Dotted Qtr", 10);
    releaseNoteComboBox.addItem("Half", 11);
    releaseNoteComboBox.addItem("Dotted Half", 12);
    releaseNoteComboBox.addItem("Whole", 13);
    releaseNoteComboBox.setSelectedId(audioProcessor.getReleaseNoteValue() + 1, juce::dontSendNotification);
    releaseNoteComboBox.addListener(this);
    addAndMakeVisible(releaseNoteComboBox);
    releaseNoteLabel.setText("Note", juce::dontSendNotification);
    releaseNoteLabel.attachToComponent(&releaseNoteComboBox, true);
    addAndMakeVisible(releaseNoteLabel);
    
    // Update UI with current band state
    enableLeftButton.setToggleState(audioProcessor.isBandLeftEnabled(0), juce::dontSendNotification);
    enableRightButton.setToggleState(audioProcessor.isBandRightEnabled(0), juce::dontSendNotification);
    
    // Initialize UI based on mode
    updateModeUI();
    
    setSize(550, 365);  // Increased height for mode toggle button
}

WhiteDuckAudioProcessorEditor::~WhiteDuckAudioProcessorEditor()
{
}

//==============================================================================
void WhiteDuckAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(12.0f));
    g.drawFittedText("WhiteDuck", getLocalBounds().withRight(getWidth() - 10).withTop(5), juce::Justification::topRight, 1);
}

void WhiteDuckAudioProcessorEditor::resized()
{
    int labelWidth = 70;
    int sliderHeight = 25;
    int borderY = 35;  // Increased to make room for title
    int spacing = 28;
    
    // Mode toggle button (below title area)
    bpmModeLabel.setBounds(10, borderY - 25, 65, 20);
    bpmSyncModeButton.setBounds(78, borderY - 25, 80, 20);
    
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
    
    // Note value selection (BPM sync is automatic from DAW)
    int notesSectionY = rightFreqY + spacing + 10;
    attackNoteComboBox.setBounds(labelWidth + 10, notesSectionY, getWidth() - labelWidth - 20, 20);
    
    int releaseNoteY = notesSectionY + spacing;
    releaseNoteComboBox.setBounds(labelWidth + 10, releaseNoteY, getWidth() - labelWidth - 20, 20);
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
    if (button == &bpmSyncModeButton)
    {
        bool newMode = bpmSyncModeButton.getToggleState();
        audioProcessor.setBpmSyncMode(newMode);
        updateModeUI();
    }
    else if (button == &enableLeftButton)
    {
        audioProcessor.setBandLeftEnabled(0, enableLeftButton.getToggleState());
    }
    else if (button == &enableRightButton)
    {
        audioProcessor.setBandRightEnabled(0, enableRightButton.getToggleState());
    }
}

void WhiteDuckAudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (comboBox == &attackNoteComboBox)
    {
        audioProcessor.setAttackNoteValue(comboBox->getSelectedId() - 1);
    }
    else if (comboBox == &releaseNoteComboBox)
    {
        audioProcessor.setReleaseNoteValue(comboBox->getSelectedId() - 1);
    }
}

void WhiteDuckAudioProcessorEditor::updateBandUI(int bandIndex)
{
    // Placeholder for future use
}

void WhiteDuckAudioProcessorEditor::updateModeUI()
{
    bool isBpmSync = audioProcessor.getBpmSyncMode();
    
    // Enable/disable sliders based on mode
    attackSlider.setEnabled(!isBpmSync);
    releaseSlider.setEnabled(!isBpmSync);
    
    // Show/hide combo boxes
    attackNoteComboBox.setVisible(isBpmSync);
    attackNoteLabel.setVisible(isBpmSync);
    releaseNoteComboBox.setVisible(isBpmSync);
    releaseNoteLabel.setVisible(isBpmSync);
    
    // Update labels
    if (isBpmSync)
    {
        attackLabel.setText("Attack", juce::dontSendNotification);
        releaseLabel.setText("Release", juce::dontSendNotification);
    }
    else
    {
        attackLabel.setText("Attack (ms)", juce::dontSendNotification);
        releaseLabel.setText("Release (ms)", juce::dontSendNotification);
    }
}
