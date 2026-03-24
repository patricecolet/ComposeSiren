//
// Created by joseph larralde on 24/01/2026.
//

#include "ReverbComponent.h"

//==============================================================================
// ReverbComponent - Section de reverb
ReverbComponent::ReverbComponent(SirenePlugAudioProcessor& p)
    : audioProcessor(p)
{
    // Titre
    titleLabel.setText("REVERB", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    addAndMakeVisible(titleLabel);

    // Bouton d'activation
    enableButton.setButtonText("Enable (CC64 ch16)");
    enableButton.setToggleState(audioProcessor.mySynth->isReverbEnabled(), juce::dontSendNotification);
    // enableButton.setToggleState(true, juce::dontSendNotification);
    enableButton.addListener(this);
    addAndMakeVisible(enableButton);

    // Room Size
    roomSizeLabel.setText("Room (CC65)", juce::dontSendNotification);
    roomSizeLabel.setJustificationType(juce::Justification::centredLeft);
    roomSizeLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    roomSizeLabel.setFont(juce::FontOptions(10.0f));
    addAndMakeVisible(roomSizeLabel);

    roomSizeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    roomSizeSlider.setRange(0.0, 1.0, 0.01);
    roomSizeSlider.setValue(audioProcessor.mySynth->reverb.getroomsize());
    // roomSizeSlider.setValue(0.5f);
    roomSizeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 18);
    roomSizeSlider.setColour(juce::Slider::thumbColourId, juce::Colours::cyan);
    roomSizeSlider.setColour(juce::Slider::trackColourId, juce::Colours::darkblue);
    roomSizeSlider.addListener(this);
    addAndMakeVisible(roomSizeSlider);

    // Dry/Wet - 0=100% dry, 0.5=50/50, 1=100% wet
    wetLabel.setText("Dry/Wet (CC66)", juce::dontSendNotification);
    wetLabel.setJustificationType(juce::Justification::centredLeft);
    wetLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    wetLabel.setFont(juce::FontOptions(10.0f));
    addAndMakeVisible(wetLabel);

    wetSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    wetSlider.setRange(0.0, 1.0, 0.01);
    // Calculer la valeur dry/wet actuelle depuis wet et dry
    float currentWet = audioProcessor.mySynth->reverb.getwet();
    float currentDry = audioProcessor.mySynth->reverb.getdry();
    float dryWetValue = currentWet / (currentWet + currentDry + 0.001f); // Éviter division par zéro
    wetSlider.setValue(dryWetValue);
    // wetSlider.setValue(0.5f);
    wetSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 18);
    wetSlider.setColour(juce::Slider::thumbColourId, juce::Colours::green);
    wetSlider.setColour(juce::Slider::trackColourId, juce::Colours::darkgreen);
    wetSlider.addListener(this);
    addAndMakeVisible(wetSlider);

    // Damp
    dampLabel.setText("Damp (CC67)", juce::dontSendNotification);
    dampLabel.setJustificationType(juce::Justification::centredLeft);
    dampLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    dampLabel.setFont(juce::FontOptions(10.0f));
    addAndMakeVisible(dampLabel);

    dampSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    dampSlider.setRange(0.0, 1.0, 0.01);
    dampSlider.setValue(audioProcessor.mySynth->reverb.getdamp());
    // dampSlider.setValue(0.5f);
    dampSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 18);
    dampSlider.setColour(juce::Slider::thumbColourId, juce::Colours::orange);
    dampSlider.setColour(juce::Slider::trackColourId, juce::Colours::darkorange);
    dampSlider.addListener(this);
    addAndMakeVisible(dampSlider);

    // Width
    widthLabel.setText("Width (CC70)", juce::dontSendNotification);
    widthLabel.setJustificationType(juce::Justification::centredLeft);
    widthLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    widthLabel.setFont(juce::FontOptions(10.0f));
    addAndMakeVisible(widthLabel);

    widthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    widthSlider.setRange(0.0, 1.0, 0.01);
    widthSlider.setValue(audioProcessor.mySynth->reverb.getwidth());
    // widthSlider.setValue(0.5f);
    widthSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 18);
    widthSlider.setColour(juce::Slider::thumbColourId, juce::Colours::violet);
    widthSlider.setColour(juce::Slider::trackColourId, juce::Colours::purple);
    widthSlider.addListener(this);
    addAndMakeVisible(widthSlider);

    // Highpass Filter (CC68 sur canal 16)
    highpassLabel.setText("HPF (CC68)", juce::dontSendNotification);
    highpassLabel.setJustificationType(juce::Justification::centredLeft);
    highpassLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    highpassLabel.setFont(juce::FontOptions(10.0f));
    addAndMakeVisible(highpassLabel);

    highpassSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    highpassSlider.setRange(20.0, 2000.0, 1.0);
    highpassSlider.setValue(audioProcessor.mySynth->getReverbHighpass());
    // highpassSlider.setValue(20.0);
    highpassSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 18);
    highpassSlider.setTextValueSuffix(" Hz");
    highpassSlider.setColour(juce::Slider::thumbColourId, juce::Colours::gold);
    highpassSlider.setColour(juce::Slider::trackColourId, juce::Colour(100, 100, 0));
    highpassSlider.setSkewFactorFromMidPoint(200.0); // Logarithmique
    highpassSlider.addListener(this);
    addAndMakeVisible(highpassSlider);

    // Lowpass Filter (CC69 sur canal 16)
    lowpassLabel.setText("LPF (CC69)", juce::dontSendNotification);
    lowpassLabel.setJustificationType(juce::Justification::centredLeft);
    lowpassLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    lowpassLabel.setFont(juce::FontOptions(10.0f));
    addAndMakeVisible(lowpassLabel);

    lowpassSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    lowpassSlider.setRange(2000.0, 20000.0, 1.0);
    lowpassSlider.setValue(audioProcessor.mySynth->getReverbLowpass());
    // lowpassSlider.setValue(20000.0);
    lowpassSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 18);
    lowpassSlider.setTextValueSuffix(" Hz");
    lowpassSlider.setColour(juce::Slider::thumbColourId, juce::Colours::hotpink);
    lowpassSlider.setColour(juce::Slider::trackColourId, juce::Colour(139, 0, 139));
    lowpassSlider.setSkewFactorFromMidPoint(8000.0); // Logarithmique
    lowpassSlider.addListener(this);
    addAndMakeVisible(lowpassSlider);

    // Démarrer le timer pour synchroniser l'UI avec les changements MIDI
    startTimer(50); // 50ms = 20 Hz
}

ReverbComponent::~ReverbComponent()
{
    stopTimer();
}

void ReverbComponent::timerCallback()
{
    //*
    // Synchroniser l'état Enable
    bool currentEnabled = audioProcessor.mySynth->isReverbEnabled();
    if (enableButton.getToggleState() != currentEnabled)
    {
        enableButton.setToggleState(currentEnabled, juce::dontSendNotification);
    }

    // Synchroniser Room Size (CC65)
    float currentRoomSize = audioProcessor.mySynth->reverb.getroomsize();
    if (std::abs(roomSizeSlider.getValue() - currentRoomSize) > 0.01)
    {
        roomSizeSlider.setValue(currentRoomSize, juce::dontSendNotification);
    }

    // Synchroniser Dry/Wet (CC66)
    // On synchronise directement avec wet (qui varie de 0 à 1)
    float currentWet = audioProcessor.mySynth->reverb.getwet();
    if (std::abs(wetSlider.getValue() - currentWet) > 0.02)  // Seuil augmenté pour éviter l'oscillation
    {
        wetSlider.setValue(currentWet, juce::dontSendNotification);
    }

    // Synchroniser Damp (CC67)
    float currentDamp = audioProcessor.mySynth->reverb.getdamp();
    if (std::abs(dampSlider.getValue() - currentDamp) > 0.01)
    {
        dampSlider.setValue(currentDamp, juce::dontSendNotification);
    }

    // Synchroniser Width (CC70)
    float currentWidth = audioProcessor.mySynth->reverb.getwidth();
    if (std::abs(widthSlider.getValue() - currentWidth) > 0.01)
    {
        widthSlider.setValue(currentWidth, juce::dontSendNotification);
    }

    // Synchroniser Highpass (CC68)
    float currentHighpass = audioProcessor.mySynth->getReverbHighpass();
    if (std::abs(highpassSlider.getValue() - currentHighpass) > 1.0)
    {
        highpassSlider.setValue(currentHighpass, juce::dontSendNotification);
    }

    // Synchroniser Lowpass (CC69)
    float currentLowpass = audioProcessor.mySynth->getReverbLowpass();
    if (std::abs(lowpassSlider.getValue() - currentLowpass) > 1.0)
    {
        lowpassSlider.setValue(currentLowpass, juce::dontSendNotification);
    }
    //*/
}

void ReverbComponent::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(40, 40, 60));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(2), 5);
    g.setColour(juce::Colours::lightblue);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2), 5, 2);
}

void ReverbComponent::resized()
{
    auto area = getLocalBounds().reduced(8);
    titleLabel.setBounds(area.removeFromTop(22));
    area.removeFromTop(2);
    enableButton.setBounds(area.removeFromTop(22));
    area.removeFromTop(5);

    // Sliders horizontaux empilés verticalement
    auto sliderHeight = 25;

    // Room Size
    auto roomRow = area.removeFromTop(sliderHeight);
    roomSizeLabel.setBounds(roomRow.removeFromLeft(80));
    roomSizeSlider.setBounds(roomRow);
    area.removeFromTop(3);

    // Dry/Wet
    auto wetRow = area.removeFromTop(sliderHeight);
    wetLabel.setBounds(wetRow.removeFromLeft(80));
    wetSlider.setBounds(wetRow);
    area.removeFromTop(3);

    // Damp
    auto dampRow = area.removeFromTop(sliderHeight);
    dampLabel.setBounds(dampRow.removeFromLeft(80));
    dampSlider.setBounds(dampRow);
    area.removeFromTop(3);

    // Width
    auto widthRow = area.removeFromTop(sliderHeight);
    widthLabel.setBounds(widthRow.removeFromLeft(80));
    widthSlider.setBounds(widthRow);
    area.removeFromTop(3);

    // Highpass
    auto hpfRow = area.removeFromTop(sliderHeight);
    highpassLabel.setBounds(hpfRow.removeFromLeft(80));
    highpassSlider.setBounds(hpfRow);
    area.removeFromTop(3);

    // Lowpass
    auto lpfRow = area.removeFromTop(sliderHeight);
    lowpassLabel.setBounds(lpfRow.removeFromLeft(80));
    lowpassSlider.setBounds(lpfRow);
}

void ReverbComponent::sliderValueChanged(juce::Slider* slider)
{
    //*
    if (slider == &roomSizeSlider)
    {
        audioProcessor.mySynth->reverb.setroomsize((float)roomSizeSlider.getValue());
    }
    else if (slider == &wetSlider)
    {
        // Dry/Wet : 0=100% dry, 0.5=50/50, 1=100% wet
        float dryWetValue = (float)wetSlider.getValue();
        audioProcessor.mySynth->reverb.setwet(dryWetValue);
        audioProcessor.mySynth->reverb.setdry(1.0f - dryWetValue);
    }
    else if (slider == &dampSlider)
    {
        audioProcessor.mySynth->reverb.setdamp((float)dampSlider.getValue());
    }
    else if (slider == &widthSlider)
    {
        audioProcessor.mySynth->reverb.setwidth((float)widthSlider.getValue());
    }
    else if (slider == &highpassSlider)
    {
        audioProcessor.mySynth->setReverbHighpass((float)highpassSlider.getValue());
    }
    else if (slider == &lowpassSlider)
    {
        audioProcessor.mySynth->setReverbLowpass((float)lowpassSlider.getValue());
    }
    //*/
}

void ReverbComponent::buttonClicked(juce::Button* button)
{
    //*
    if (button == &enableButton)
    {
        audioProcessor.mySynth->setReverbEnabled(enableButton.getToggleState());
    }
    //*/
}
