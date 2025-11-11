/*
  ==============================================================================

    PluginEditor_S1.cpp
    ComposeSiren S1 Alto - Interface utilisateur Solo avec contrôles MIDI

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SireneS1AudioProcessorEditor::SireneS1AudioProcessorEditor (SireneS1AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      lastNoteOn(false), lastNote(0), lastVelocity(0)
{
    setSize (720, 580);
    
    // === En-tête : Indicateurs MIDI ===
    noteLabel.setText("Note: ---", juce::dontSendNotification);
    noteLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    noteLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(noteLabel);
    
    velocityLabel.setText("Vel: ---", juce::dontSendNotification);
    velocityLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    velocityLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(velocityLabel);
    
    // === Section VIBRATO ===
    vibratoGroup.setText("Vibrato");
    vibratoGroup.setTextLabelPosition(juce::Justification::centredTop);
    vibratoGroup.setColour(juce::GroupComponent::outlineColourId, juce::Colour(0xff4a90e2));
    vibratoGroup.setColour(juce::GroupComponent::textColourId, juce::Colour(0xff4a90e2));
    addAndMakeVisible(vibratoGroup);
    
    createKnob(vibratoDepthKnob, vibratoDepthLabel, "Depth", 1);
    createKnob(vibratoRateKnob, vibratoRateLabel, "Rate", 9);
    createKnob(vibratoAttackKnob, vibratoAttackLabel, "Attack", 11);
    
    // === Section TREMOLO ===
    tremoloGroup.setText("Tremolo");
    tremoloGroup.setTextLabelPosition(juce::Justification::centredTop);
    tremoloGroup.setColour(juce::GroupComponent::outlineColourId, juce::Colour(0xffe28a4a));
    tremoloGroup.setColour(juce::GroupComponent::textColourId, juce::Colour(0xffe28a4a));
    addAndMakeVisible(tremoloGroup);
    
    createKnob(tremoloRateKnob, tremoloRateLabel, "Rate", 15);
    createKnob(tremoloDepthKnob, tremoloDepthLabel, "Depth", 92);
    
    // === Section ENVELOPPE ===
    envelopeGroup.setText("Enveloppe");
    envelopeGroup.setTextLabelPosition(juce::Justification::centredTop);
    envelopeGroup.setColour(juce::GroupComponent::outlineColourId, juce::Colour(0xff8a4ae2));
    envelopeGroup.setColour(juce::GroupComponent::textColourId, juce::Colour(0xff8a4ae2));
    addAndMakeVisible(envelopeGroup);
    
    createKnob(attackKnob, attackLabel, "Attack", 73);
    createKnob(releaseKnob, releaseLabel, "Release", 72);
    
    // === Section PORTAMENTO ===
    portamentoGroup.setText("Portamento");
    portamentoGroup.setTextLabelPosition(juce::Justification::centredTop);
    portamentoGroup.setColour(juce::GroupComponent::outlineColourId, juce::Colour(0xff4ae28a));
    portamentoGroup.setColour(juce::GroupComponent::textColourId, juce::Colour(0xff4ae28a));
    addAndMakeVisible(portamentoGroup);
    
    createKnob(portamentoKnob, portamentoLabel, "Vitesse", 5);
    
    // Démarrer le timer pour rafraîchir l'affichage (30 FPS)
    startTimerHz(30);
}

SireneS1AudioProcessorEditor::~SireneS1AudioProcessorEditor()
{
    stopTimer();
}

void SireneS1AudioProcessorEditor::createKnob(juce::Slider& slider, juce::Label& label, 
                                               const juce::String& name, int ccNumber)
{
    // Configuration du slider en mode rotatif (knob)
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    slider.setRange(0.0, 127.0, 1.0);
    slider.setValue(0.0);
    slider.setTextValueSuffix(" CC" + juce::String(ccNumber));
    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff4a90e2));
    slider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    slider.addListener(this);
    addAndMakeVisible(slider);
    
    // Label au-dessus du knob
    label.setText(name, juce::dontSendNotification);
    label.setFont(juce::Font(13.0f));
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(label);
}

//==============================================================================
void SireneS1AudioProcessorEditor::paint (juce::Graphics& g)
{
    // Fond gris foncé
    g.fillAll(juce::Colour(0xff2a2a2a));
    
    // En-tête avec titre
    g.setColour(juce::Colour(0xff404040));
    g.fillRect(0, 0, getWidth(), 100);
    
    // Titre
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(32.0f, juce::Font::bold));
    g.drawText("ComposeSiren S1", 20, 15, getWidth() - 40, 35, juce::Justification::centred);
    
    g.setFont(juce::Font(18.0f));
    g.setColour(juce::Colour(0xffaaaaaa));
    g.drawText("Alto", 20, 50, getWidth() - 40, 25, juce::Justification::centred);
    
    // LED Note On/Off (petite, dans le header)
    g.setColour(lastNoteOn ? juce::Colours::green : juce::Colour(0xff333333));
    g.fillEllipse(20, 80, 15, 15);
}

void SireneS1AudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    
    // En-tête (titre + indicateurs MIDI)
    auto header = area.removeFromTop(100);
    noteLabel.setBounds(50, 78, 150, 20);
    velocityLabel.setBounds(220, 78, 150, 20);
    
    // Marge
    area.removeFromTop(10);
    area.reduce(15, 0);
    
    // Organisation en 2 lignes de 2 colonnes
    int groupHeight = 220;
    int groupSpacing = 10;
    
    // Ligne 1 : Vibrato + Tremolo
    auto line1 = area.removeFromTop(groupHeight);
    
    auto vibratoArea = line1.removeFromLeft((getWidth() - 30 - groupSpacing) / 2);
    vibratoGroup.setBounds(vibratoArea);
    
    line1.removeFromLeft(groupSpacing);
    
    auto tremoloArea = line1;
    tremoloGroup.setBounds(tremoloArea);
    
    // Layout interne Vibrato (3 knobs)
    auto vibratoContent = vibratoArea.reduced(10, 25);
    int knobWidth = vibratoContent.getWidth() / 3 - 5;
    
    vibratoDepthLabel.setBounds(vibratoContent.removeFromLeft(knobWidth).removeFromTop(20));
    vibratoDepthKnob.setBounds(vibratoDepthLabel.getBounds().withY(vibratoDepthLabel.getBottom()).withHeight(100));
    vibratoContent.removeFromLeft(5);
    
    vibratoRateLabel.setBounds(vibratoContent.removeFromLeft(knobWidth).removeFromTop(20));
    vibratoRateKnob.setBounds(vibratoRateLabel.getBounds().withY(vibratoRateLabel.getBottom()).withHeight(100));
    vibratoContent.removeFromLeft(5);
    
    vibratoAttackLabel.setBounds(vibratoContent.removeFromLeft(knobWidth).removeFromTop(20));
    vibratoAttackKnob.setBounds(vibratoAttackLabel.getBounds().withY(vibratoAttackLabel.getBottom()).withHeight(100));
    
    // Layout interne Tremolo (2 knobs)
    auto tremoloContent = tremoloArea.reduced(10, 25);
    knobWidth = tremoloContent.getWidth() / 2 - 5;
    
    tremoloRateLabel.setBounds(tremoloContent.removeFromLeft(knobWidth).removeFromTop(20));
    tremoloRateKnob.setBounds(tremoloRateLabel.getBounds().withY(tremoloRateLabel.getBottom()).withHeight(100));
    tremoloContent.removeFromLeft(10);
    
    tremoloDepthLabel.setBounds(tremoloContent.removeFromLeft(knobWidth).removeFromTop(20));
    tremoloDepthKnob.setBounds(tremoloDepthLabel.getBounds().withY(tremoloDepthLabel.getBottom()).withHeight(100));
    
    // Ligne 2 : Enveloppe + Portamento
    area.removeFromTop(groupSpacing);
    auto line2 = area.removeFromTop(groupHeight);
    
    auto envelopeArea = line2.removeFromLeft((getWidth() - 30 - groupSpacing) / 2);
    envelopeGroup.setBounds(envelopeArea);
    
    line2.removeFromLeft(groupSpacing);
    
    auto portamentoArea = line2;
    portamentoGroup.setBounds(portamentoArea);
    
    // Layout interne Enveloppe (2 knobs)
    auto envelopeContent = envelopeArea.reduced(10, 25);
    knobWidth = envelopeContent.getWidth() / 2 - 5;
    
    attackLabel.setBounds(envelopeContent.removeFromLeft(knobWidth).removeFromTop(20));
    attackKnob.setBounds(attackLabel.getBounds().withY(attackLabel.getBottom()).withHeight(100));
    envelopeContent.removeFromLeft(10);
    
    releaseLabel.setBounds(envelopeContent.removeFromLeft(knobWidth).removeFromTop(20));
    releaseKnob.setBounds(releaseLabel.getBounds().withY(releaseLabel.getBottom()).withHeight(100));
    
    // Layout interne Portamento (1 knob centré)
    auto portamentoContent = portamentoArea.reduced(10, 25);
    int centerX = portamentoContent.getWidth() / 2 - 60;
    
    portamentoLabel.setBounds(portamentoContent.removeFromLeft(portamentoContent.getWidth()).removeFromTop(20));
    portamentoLabel.setJustificationType(juce::Justification::centred);
    portamentoKnob.setBounds(centerX, portamentoLabel.getBottom(), 120, 100);
}

void SireneS1AudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    int ccNumber = -1;
    int ccValue = (int)slider->getValue();
    
    // Identifier quel CC envoyer (uniquement les paramètres de synthèse, pas de volume/pan)
    if (slider == &vibratoDepthKnob) ccNumber = 1;
    else if (slider == &vibratoRateKnob) ccNumber = 9;
    else if (slider == &vibratoAttackKnob) ccNumber = 11;
    else if (slider == &tremoloRateKnob) ccNumber = 15;
    else if (slider == &tremoloDepthKnob) ccNumber = 92;
    else if (slider == &attackKnob) ccNumber = 73;
    else if (slider == &releaseKnob) ccNumber = 72;
    else if (slider == &portamentoKnob) ccNumber = 5;
    
    if (ccNumber >= 0)
    {
        // Transmettre directement au handler MIDI du processeur
        auto* midiHandler = audioProcessor.myMidiInHandler;
        if (midiHandler) {
            midiHandler->HandleControlChange(1, ccNumber, ccValue);
        }
    }
}

void SireneS1AudioProcessorEditor::timerCallback()
{
    // Récupérer l'état MIDI depuis le processeur
    auto midiState = audioProcessor.getMidiState();
    
    // Mettre à jour les labels d'affichage
    if (midiState.noteOn)
    {
        const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        int octave = (midiState.currentNote / 12) - 1;
        juce::String noteName = juce::String(noteNames[midiState.currentNote % 12]) + juce::String(octave);
        noteLabel.setText("Note: " + juce::String(midiState.currentNote) + " (" + noteName + ")", 
                         juce::dontSendNotification);
        
        velocityLabel.setText("Vel: " + juce::String(midiState.currentVelocity), 
                             juce::dontSendNotification);
    }
    else
    {
        noteLabel.setText("Note: ---", juce::dontSendNotification);
        velocityLabel.setText("Vel: ---", juce::dontSendNotification);
    }
    
    // Synchroniser les knobs avec les CC reçus via MIDI externe
    for (const auto& pair : midiState.activeCC)
    {
        int ccNum = pair.first;
        int ccVal = pair.second;
        
        // Mettre à jour le knob correspondant (sans déclencher de callback)
        juce::Slider* targetKnob = nullptr;
        
        if (ccNum == 1) targetKnob = &vibratoDepthKnob;
        else if (ccNum == 9) targetKnob = &vibratoRateKnob;
        else if (ccNum == 11) targetKnob = &vibratoAttackKnob;
        else if (ccNum == 15) targetKnob = &tremoloRateKnob;
        else if (ccNum == 92) targetKnob = &tremoloDepthKnob;
        else if (ccNum == 73) targetKnob = &attackKnob;
        else if (ccNum == 72) targetKnob = &releaseKnob;
        else if (ccNum == 5) targetKnob = &portamentoKnob;
        
        if (targetKnob && targetKnob->getValue() != ccVal)
        {
            targetKnob->setValue(ccVal, juce::dontSendNotification);
        }
    }
    
    // Rafraîchir si l'état a changé
    if (midiState.noteOn != lastNoteOn)
    {
        lastNoteOn = midiState.noteOn;
        repaint();
    }
}
