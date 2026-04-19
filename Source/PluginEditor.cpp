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
    addAndMakeVisible(bpmSyncModeButton);
    bpmSyncModeButton.addListener(this);
    bpmModeLabel.setText("Sync Mode:", juce::dontSendNotification);
    addAndMakeVisible(bpmModeLabel);
    
    // Global Sliders
    attackSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    attackSlider.setRange(0.1, 200.0, 0.1);
    addAndMakeVisible(attackSlider);
    attackLabel.setText("Attack", juce::dontSendNotification);
    attackLabel.attachToComponent(&attackSlider, true);
    addAndMakeVisible(attackLabel);
    
    releaseSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    releaseSlider.setRange(10.0, 2000.0, 10.0);
    addAndMakeVisible(releaseSlider);
    releaseLabel.setText("Release", juce::dontSendNotification);
    releaseLabel.attachToComponent(&releaseSlider, true);
    addAndMakeVisible(releaseLabel);
    
    mixSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    mixSlider.setRange(0.0, 1.0, 0.01);
    addAndMakeVisible(mixSlider);
    mixLabel.setText("Mix", juce::dontSendNotification);
    mixLabel.attachToComponent(&mixSlider, true);
    addAndMakeVisible(mixLabel);
    
    midiNoteSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    midiNoteSlider.setRange(0.0, 127.0, 1.0);
    addAndMakeVisible(midiNoteSlider);
    midiNoteLabel.setText("MIDI", juce::dontSendNotification);
    midiNoteLabel.attachToComponent(&midiNoteSlider, true);
    addAndMakeVisible(midiNoteLabel);
    
    // Band enable toggles (separate for LEFT and RIGHT)
    addAndMakeVisible(enableLeftButton);
    
    addAndMakeVisible(enableRightButton);
    
    // LEFT frequency slider
    leftFreqSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    leftFreqSlider.setRange(20.0, 19999.0, 1.0);
    leftFreqSlider.setSkewFactorFromMidPoint(500.0);
    addAndMakeVisible(leftFreqSlider);
    leftFreqLabel.setText("LEFT", juce::dontSendNotification);
    leftFreqLabel.attachToComponent(&leftFreqSlider, true);
    addAndMakeVisible(leftFreqLabel);
    
    // RIGHT frequency slider
    rightFreqSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    rightFreqSlider.setRange(21.0, 20000.0, 1.0);
    rightFreqSlider.setSkewFactorFromMidPoint(5000.0);
    addAndMakeVisible(rightFreqSlider);
    rightFreqLabel.setText("RIGHT", juce::dontSendNotification);
    rightFreqLabel.attachToComponent(&rightFreqSlider, true);
    addAndMakeVisible(rightFreqLabel);
    
    
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
    releaseNoteComboBox.addListener(this);
    addAndMakeVisible(releaseNoteComboBox);
    releaseNoteLabel.setText("Release Note", juce::dontSendNotification);
    releaseNoteLabel.attachToComponent(&releaseNoteComboBox, true);
    addAndMakeVisible(releaseNoteLabel);
    
    // Curve type combo boxes
    attackCurveComboBox.addItem("Balanced", 1);
    attackCurveComboBox.addItem("Slow", 2);
    attackCurveComboBox.addItem("Fast", 3);
    addAndMakeVisible(attackCurveComboBox);
    attackCurveLabel.setText("Attack Curve", juce::dontSendNotification);
    attackCurveLabel.attachToComponent(&attackCurveComboBox, true);
    addAndMakeVisible(attackCurveLabel);
    
    releaseCurveComboBox.addItem("Balanced", 1);
    releaseCurveComboBox.addItem("Slow", 2);
    releaseCurveComboBox.addItem("Fast", 3);
    addAndMakeVisible(releaseCurveComboBox);
    releaseCurveLabel.setText("Release Curve", juce::dontSendNotification);
    releaseCurveLabel.attachToComponent(&releaseCurveComboBox, true);
    addAndMakeVisible(releaseCurveLabel);
    
    // Attachments for automated parameters
    auto& state = audioProcessor.getValueTreeState();
    bpmSyncAttachment = std::make_unique<ButtonAttachment>(state, WhiteDuckAudioProcessor::PARAM_BPM_SYNC, bpmSyncModeButton);
    attackAttachment = std::make_unique<SliderAttachment>(state, WhiteDuckAudioProcessor::PARAM_ATTACK_MS, attackSlider);
    releaseAttachment = std::make_unique<SliderAttachment>(state, WhiteDuckAudioProcessor::PARAM_RELEASE_MS, releaseSlider);
    mixAttachment = std::make_unique<SliderAttachment>(state, WhiteDuckAudioProcessor::PARAM_MIX, mixSlider);
    midiNoteAttachment = std::make_unique<SliderAttachment>(state, WhiteDuckAudioProcessor::PARAM_MIDI_NOTE, midiNoteSlider);
    leftFreqAttachment = std::make_unique<SliderAttachment>(state, WhiteDuckAudioProcessor::PARAM_BAND_LEFT_FREQ, leftFreqSlider);
    rightFreqAttachment = std::make_unique<SliderAttachment>(state, WhiteDuckAudioProcessor::PARAM_BAND_RIGHT_FREQ, rightFreqSlider);
    enableLeftAttachment = std::make_unique<ButtonAttachment>(state, WhiteDuckAudioProcessor::PARAM_BAND_LEFT_ENABLED, enableLeftButton);
    enableRightAttachment = std::make_unique<ButtonAttachment>(state, WhiteDuckAudioProcessor::PARAM_BAND_RIGHT_ENABLED, enableRightButton);
    attackCurveAttachment = std::make_unique<ComboBoxAttachment>(state, WhiteDuckAudioProcessor::PARAM_ATTACK_CURVE, attackCurveComboBox);
    releaseCurveAttachment = std::make_unique<ComboBoxAttachment>(state, WhiteDuckAudioProcessor::PARAM_RELEASE_CURVE, releaseCurveComboBox);
    releaseNoteAttachment = std::make_unique<ComboBoxAttachment>(state, WhiteDuckAudioProcessor::PARAM_RELEASE_NOTE, releaseNoteComboBox);
    
    // Initialize UI based on mode
    updateModeUI();
    startTimerHz(10);
    
    setSize(640, 380);  // Wider layout to avoid text clipping
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
    int labelWidth = 125;
    int sliderHeight = 25;
    int borderY = 35;  // Increased to make room for title
    int spacing = 28;
    
    // Mode toggle button (below title area)
    bpmModeLabel.setBounds(10, borderY - 25, 95, 20);
    bpmSyncModeButton.setBounds(108, borderY - 25, 150, 20);
    
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
    enableLeftButton.setBounds(20, bandSettingsY, 150, 20);
    enableRightButton.setBounds(180, bandSettingsY, 150, 20);
    
    int leftFreqY = bandSettingsY + 30;
    leftFreqSlider.setBounds(labelWidth + 10, leftFreqY, getWidth() - labelWidth - 20, sliderHeight);
    
    int rightFreqY = leftFreqY + spacing;
    rightFreqSlider.setBounds(labelWidth + 10, rightFreqY, getWidth() - labelWidth - 20, sliderHeight);
    
    // Release note selection (BPM sync)
    int releaseNoteY = rightFreqY + spacing + 10;
    releaseNoteComboBox.setBounds(labelWidth + 10, releaseNoteY, getWidth() - labelWidth - 20, 20);
    
    // Curve type selection
    int curvesSectionY = releaseNoteY + spacing + 10;
    attackCurveComboBox.setBounds(labelWidth + 10, curvesSectionY, getWidth() - labelWidth - 20, 22);
    
    int releaseCurveY = curvesSectionY + spacing;
    releaseCurveComboBox.setBounds(labelWidth + 10, releaseCurveY, getWidth() - labelWidth - 20, 22);
}

void WhiteDuckAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
}

void WhiteDuckAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &bpmSyncModeButton)
    {
        audioProcessor.setBpmSyncMode(bpmSyncModeButton.getToggleState());
        updateModeUI();
    }
}

void WhiteDuckAudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (comboBox == &releaseNoteComboBox)
        audioProcessor.setReleaseNoteValue(comboBox->getSelectedId() - 1);
}

void WhiteDuckAudioProcessorEditor::updateBandUI(int bandIndex)
{
    // Placeholder for future use
}

void WhiteDuckAudioProcessorEditor::updateModeUI()
{
    bool isBpmSync = bpmSyncModeButton.getToggleState();
    
    // Sync mode only affects Release selection
    releaseSlider.setEnabled(!isBpmSync);
    releaseNoteComboBox.setVisible(isBpmSync);
    releaseNoteLabel.setVisible(isBpmSync);
    
    // Attack is always in milliseconds
    attackLabel.setText("Attack (ms)", juce::dontSendNotification);
    releaseLabel.setText(isBpmSync ? "Release (Note)" : "Release (ms)", juce::dontSendNotification);

    if (isBpmSync)
        releaseSlider.setValue(audioProcessor.getDisplayedReleaseTimeMs(), juce::dontSendNotification);
}

void WhiteDuckAudioProcessorEditor::timerCallback()
{
    updateModeUI();
}
