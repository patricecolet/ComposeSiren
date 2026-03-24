//
// Created by joseph larralde on 24/01/2026.
//

#include "MixerStripComponent.h"

//==============================================================================
// MixerStripComponent - Strip de mixage pour une sirène
MixerStripComponent::MixerStripComponent(SirenePlugAudioProcessor& p, int sireneNum)
    : audioProcessor(p),
      sireneNumber(sireneNum)
{
    // Label du nom de la sirène
    nameLabel.setText("S" + juce::String(sireneNumber), juce::dontSendNotification);
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(nameLabel);

    // Master Volume (CC70) - volume indépendant pour mixer
    volumeLabel.setText("Master (CC70)", juce::dontSendNotification);
    volumeLabel.setJustificationType(juce::Justification::centred);
    volumeLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    volumeLabel.setFont(juce::FontOptions(9.0f));
    addAndMakeVisible(volumeLabel);

    volumeSlider.setSliderStyle(juce::Slider::LinearVertical);
    volumeSlider.setRange(0.0, 1.0, 0.01);
    //volumeSlider.setValue(audioProcessor.mySynth->getvolume(sireneNumber));
    volumeSlider.setValue(1.f);
    volumeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    volumeSlider.setColour(juce::Slider::thumbColourId, juce::Colours::lightblue);
    volumeSlider.setColour(juce::Slider::trackColourId, juce::Colours::blue);
    volumeSlider.addListener(this);
    addAndMakeVisible(volumeSlider);

    // Knob de pan rotatif avec couleur pour meilleure visibilité
    panKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    panKnob.setRange(-1.f, 1.f, 0.01);
    // panKnob.setValue(audioProcessor.mySynth->getPan(sireneNumber) * 2.f - 1.f); // Convertir de 0;1 à -1;1
    panKnob.setValue(0.f);
    panKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    panKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::orange);
    panKnob.setColour(juce::Slider::thumbColourId, juce::Colours::yellow);
    panKnob.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(80, 80, 80));
    panKnob.addListener(this);
    addAndMakeVisible(panKnob);

    // Label pan avec CC
    panLabel.setText("Pan (CC10)", juce::dontSendNotification);
    panLabel.setJustificationType(juce::Justification::centred);
    panLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    panLabel.setFont(juce::FontOptions(9.0f));
    addAndMakeVisible(panLabel);

    // Démarrer le timer pour mettre à jour la LED (30 Hz)
    startTimer(33);
}

MixerStripComponent::~MixerStripComponent()
{
    stopTimer();
}

void MixerStripComponent::timerCallback()
{
    //*
    // Mettre à jour l'état de la LED
    bool newLedState = audioProcessor.myMidiInHandler->isNoteOn(sireneNumber);
    if (newLedState != ledState)
    {
        ledState = newLedState;
        repaint();
    }

    // Mettre à jour le master volume CC70 si changé par MIDI
    float currentvolume = audioProcessor.mySynth->getMasterVolume(sireneNumber);
    if (std::abs(volumeSlider.getValue() - currentvolume) > 0.01)
    {
        volumeSlider.setValue(currentvolume, juce::dontSendNotification);
    }

    // Mettre à jour le Pan si changé par MIDI
    // getPan retourne 0.5-1.5, on veut -0.5 à +0.5 pour le slider
    float panValue = audioProcessor.mySynth->getPan(sireneNumber) * 2.f - 1.f;
    if (std::abs(panKnob.getValue() - panValue) > 0.01)
    {
        panKnob.setValue(panValue, juce::dontSendNotification);
    }
    //*/
}

void MixerStripComponent::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(50, 50, 50));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(2), 5);
    g.setColour(juce::Colours::grey);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2), 5, 1);

    // Dessiner la LED Note On/Off en haut à droite
    auto ledArea = getLocalBounds().reduced(5);
    int ledSize = 10;
    int ledX = ledArea.getRight() - ledSize - 2;
    int ledY = ledArea.getY() + 2;

    // Ombre/contour de la LED
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillEllipse((float)ledX, (float)ledY, (float)ledSize, (float)ledSize);

    // LED elle-même
    if (ledState)
    {
        // LED allumée - vert brillant avec effet glow
        g.setColour(juce::Colours::lime);
        g.fillEllipse((float)(ledX + 1), (float)(ledY + 1), (float)(ledSize - 2), (float)(ledSize - 2));
        g.setColour(juce::Colours::green.brighter());
        g.fillEllipse((float)(ledX + 2), (float)(ledY + 2), (float)(ledSize - 4), (float)(ledSize - 4));
    }
    else
    {
        // LED éteinte - gris foncé
        g.setColour(juce::Colour(60, 60, 60));
        g.fillEllipse((float)(ledX + 1), (float)(ledY + 1), (float)(ledSize - 2), (float)(ledSize - 2));
    }

    // Dessiner les marqueurs L/C/R pour le pan sous le knob
    auto panBounds = panKnob.getBounds();
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(9.0f));
    g.drawText("L", panBounds.getX() - 10, panBounds.getY() + panBounds.getHeight()/2 - 5, 10, 10, juce::Justification::right);
    g.drawText("C", panBounds.getX() + panBounds.getWidth()/2 - 5, panBounds.getBottom() + 2, 10, 10, juce::Justification::centred);
    g.drawText("R", panBounds.getRight(), panBounds.getY() + panBounds.getHeight()/2 - 5, 10, 10, juce::Justification::left);
}

void MixerStripComponent::resized()
{
    auto area = getLocalBounds().reduced(5);
    nameLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(5);

    // Master Volume CC70
    volumeLabel.setBounds(area.removeFromTop(15));
    volumeSlider.setBounds(area.removeFromTop(130));

    area.removeFromTop(5);
    panLabel.setBounds(area.removeFromTop(15));
    // Knob de pan - plus grand
    panKnob.setBounds(area.removeFromTop(110));
}

void MixerStripComponent::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &volumeSlider)
    {
        // Master Volume CC70 - volume indépendant
        audioProcessor.mySynth->setMasterVolume(sireneNumber, (float)volumeSlider.getValue());
    }
    else if (slider == &panKnob)
    {
        audioProcessor.mySynth->setPan(sireneNumber, (float)panKnob.getValue() * 0.5f + 0.5f);
    }
}
