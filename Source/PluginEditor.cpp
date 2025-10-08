/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#ifdef CMS_BUILD_WITH_PROJUCER
  // Projucer ne génère pas config.h, on définit les valeurs en dur
  #define PROJECT_VERSION_MAJOR 1
  #define PROJECT_VERSION_MINOR 5
  #define PROJECT_VERSION_PATCH 0
  #define PROJECT_DESCRIPTION "Compose Siren - Mecanique Vivante"
#else
  #include "config.h"
#endif
#include <iostream>
#include <sstream>

//==============================================================================
// Component on the top of the main window
headComponent::headComponent()
{
  addAndMakeVisible (labelPluginTitle);
  addAndMakeVisible (labelPluginSubTitle);

  labelPluginTitle.setColour (juce::Label::textColourId, juce::Colours::white);

  std::stringstream versionText;
  versionText
      << "v"
      << PROJECT_VERSION_MAJOR << "."
      << PROJECT_VERSION_MINOR << "."
      << PROJECT_VERSION_PATCH
      ;

  auto concatenatedString = versionText.str();

  auto label = "COMPOSE SIREN " + concatenatedString;
  labelPluginTitle.setText(label, juce::dontSendNotification);
  labelPluginTitle.setFont (juce::Font (24.0f, juce::Font::bold));
  labelPluginTitle.setJustificationType (juce::Justification::centredLeft);
  
  // Afficher le nom de la branche au lieu de la description
  labelPluginSubTitle.setText("custom-mix", juce::dontSendNotification);
  labelPluginSubTitle.setFont (juce::Font (14.0f, juce::Font::italic));
  labelPluginSubTitle.setJustificationType (juce::Justification::centredLeft);
  labelPluginSubTitle.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
  imgLogo = juce::ImageFileFormat::loadFrom(BinaryData::Picto_Siren_40x37_png, BinaryData::Picto_Siren_40x37_pngSize);

}

void headComponent::paint (juce::Graphics& g)
{
    //g.fillAll (juce::Colour (155, 153, 100)); // background color to see the component
    //g.drawImageAt(imgLogo, labelPluginTitle.getX() + labelPluginTitle.getWidth(), 0);


  g.drawImageAt(imgLogo, getWidth()/2 + 90, 5);
}


void headComponent::resized()
{
  auto area = getLocalBounds().reduced(10, 2);
  labelPluginTitle.setBounds(area.removeFromTop(28));
  labelPluginSubTitle.setBounds(area.removeFromTop(20));

}
//==============================================================================



//==============================================================================
// Component on the top of the main window
MainCommandsComponent::MainCommandsComponent(SirenePlugAudioProcessor& p)
    :audioProcessor(p)
{
}

MainCommandsComponent::~MainCommandsComponent()
{
}

void MainCommandsComponent::paint (juce::Graphics& g)
{
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1), 10);
    g.setColour(juce::Colour (71, 71, 71));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(2), 10);
    
   
    resetButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    // resetButton.setColour(juce::TextButton::buttonOnColourId , juce::Colours::grey);
    resetButton.setColour(juce::TextButton::textColourOffId , juce::Colours::black);
    resetButton.setButtonText ("Reset");
    resetButton.onClick = [this]
        {
            std::cout << "Reset"<<std::endl;
            audioProcessor.myMidiInHandler -> resetSireneCh(1);
            audioProcessor.myMidiInHandler -> resetSireneCh(2);
            audioProcessor.myMidiInHandler -> resetSireneCh(3);
            audioProcessor.myMidiInHandler -> resetSireneCh(4);
            audioProcessor.myMidiInHandler -> resetSireneCh(5);
            audioProcessor.myMidiInHandler -> resetSireneCh(6);
            audioProcessor.myMidiInHandler -> resetSireneCh(7);
        };
    addAndMakeVisible (resetButton);
    //resetButton.setFont (juce::Font (14.0f, juce::Font::bold)); // no set font for textbutton :-(
    //resetButton.setColour(juce::Label::textColourId, juce::Colours::black);
    
    //resetButton.onClick = [This] { doSomething(); };


}


void MainCommandsComponent::resized()
{
    resetButton.setBounds (20, 5, 60, 30);
}
//==============================================================================

//==============================================================================
// MixerStripComponent - Strip de mixage pour une sirène
MixerStripComponent::MixerStripComponent(SirenePlugAudioProcessor& p, int sireneNum)
    : audioProcessor(p), sireneNumber(sireneNum)
{
    // Label du nom de la sirène
    nameLabel.setText("S" + juce::String(sireneNumber), juce::dontSendNotification);
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(nameLabel);
    
    // Master Volume (CC70) - volume indépendant pour mixer
    masterVolumeLabel.setText("Master (CC70)", juce::dontSendNotification);
    masterVolumeLabel.setJustificationType(juce::Justification::centred);
    masterVolumeLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    masterVolumeLabel.setFont(juce::Font(9.0f));
    addAndMakeVisible(masterVolumeLabel);
    
    masterVolumeSlider.setSliderStyle(juce::Slider::LinearVertical);
    masterVolumeSlider.setRange(0.0, 1.0, 0.01);
    masterVolumeSlider.setValue(audioProcessor.mySynth->getMasterVolume(sireneNumber));
    masterVolumeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    masterVolumeSlider.setColour(juce::Slider::thumbColourId, juce::Colours::lightblue);
    masterVolumeSlider.setColour(juce::Slider::trackColourId, juce::Colours::blue);
    masterVolumeSlider.addListener(this);
    addAndMakeVisible(masterVolumeSlider);
    
    // Knob de pan rotatif avec couleur pour meilleure visibilité
    panKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    panKnob.setRange(-0.5, 0.5, 0.01);
    panKnob.setValue(audioProcessor.mySynth->getPan(sireneNumber, 0) - 0.5); // Convertir de 0-1 à -0.5-0.5
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
    panLabel.setFont(juce::Font(9.0f));
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
    // Mettre à jour l'état de la LED
    bool newLedState = audioProcessor.myMidiInHandler->isNoteOn(sireneNumber);
    if (newLedState != ledState)
    {
        ledState = newLedState;
        repaint();
    }
    
    // Mettre à jour le master volume CC70 si changé par MIDI
    float currentMasterVolume = audioProcessor.mySynth->getMasterVolume(sireneNumber);
    if (std::abs(masterVolumeSlider.getValue() - currentMasterVolume) > 0.01)
    {
        masterVolumeSlider.setValue(currentMasterVolume, juce::dontSendNotification);
    }
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
    masterVolumeLabel.setBounds(area.removeFromTop(15));
    masterVolumeSlider.setBounds(area.removeFromTop(130));
    
    area.removeFromTop(5);
    panLabel.setBounds(area.removeFromTop(15));
    // Knob de pan - plus grand
    panKnob.setBounds(area.removeFromTop(110));
}

void MixerStripComponent::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &masterVolumeSlider)
    {
        // Master Volume CC70 - volume indépendant
        audioProcessor.mySynth->setMasterVolume(sireneNumber, (float)masterVolumeSlider.getValue());
    }
    else if (slider == &panKnob)
    {
        audioProcessor.mySynth->setPan(sireneNumber, (float)panKnob.getValue());
    }
}

//==============================================================================
// ReverbComponent - Section de reverb
ReverbComponent::ReverbComponent(SirenePlugAudioProcessor& p)
    : audioProcessor(p)
{
    // Titre
    titleLabel.setText("REVERB", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(titleLabel);
    
    // Bouton d'activation
    enableButton.setButtonText("Enable (CC64 ch16)");
    enableButton.setToggleState(audioProcessor.mySynth->isReverbEnabled(), juce::dontSendNotification);
    enableButton.addListener(this);
    addAndMakeVisible(enableButton);
    
    // Room Size
    roomSizeLabel.setText("Room (CC65)", juce::dontSendNotification);
    roomSizeLabel.setJustificationType(juce::Justification::centredLeft);
    roomSizeLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    roomSizeLabel.setFont(juce::Font(10.0f));
    addAndMakeVisible(roomSizeLabel);
    
    roomSizeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    roomSizeSlider.setRange(0.0, 1.0, 0.01);
    roomSizeSlider.setValue(audioProcessor.mySynth->reverb->getroomsize());
    roomSizeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 18);
    roomSizeSlider.setColour(juce::Slider::thumbColourId, juce::Colours::cyan);
    roomSizeSlider.setColour(juce::Slider::trackColourId, juce::Colours::darkblue);
    roomSizeSlider.addListener(this);
    addAndMakeVisible(roomSizeSlider);
    
    // Dry/Wet - 0=100% dry, 0.5=50/50, 1=100% wet
    wetLabel.setText("Dry/Wet (CC66)", juce::dontSendNotification);
    wetLabel.setJustificationType(juce::Justification::centredLeft);
    wetLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    wetLabel.setFont(juce::Font(10.0f));
    addAndMakeVisible(wetLabel);
    
    wetSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    wetSlider.setRange(0.0, 1.0, 0.01);
    // Calculer la valeur dry/wet actuelle depuis wet et dry
    float currentWet = audioProcessor.mySynth->reverb->getwet();
    float currentDry = audioProcessor.mySynth->reverb->getdry();
    float dryWetValue = currentWet / (currentWet + currentDry + 0.001f); // Éviter division par zéro
    wetSlider.setValue(dryWetValue);
    wetSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 18);
    wetSlider.setColour(juce::Slider::thumbColourId, juce::Colours::green);
    wetSlider.setColour(juce::Slider::trackColourId, juce::Colours::darkgreen);
    wetSlider.addListener(this);
    addAndMakeVisible(wetSlider);
    
    // Damp
    dampLabel.setText("Damp (CC67)", juce::dontSendNotification);
    dampLabel.setJustificationType(juce::Justification::centredLeft);
    dampLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    dampLabel.setFont(juce::Font(10.0f));
    addAndMakeVisible(dampLabel);
    
    dampSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    dampSlider.setRange(0.0, 1.0, 0.01);
    dampSlider.setValue(audioProcessor.mySynth->reverb->getdamp());
    dampSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 18);
    dampSlider.setColour(juce::Slider::thumbColourId, juce::Colours::orange);
    dampSlider.setColour(juce::Slider::trackColourId, juce::Colours::darkorange);
    dampSlider.addListener(this);
    addAndMakeVisible(dampSlider);
    
    // Width
    widthLabel.setText("Width (CC70)", juce::dontSendNotification);
    widthLabel.setJustificationType(juce::Justification::centredLeft);
    widthLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    widthLabel.setFont(juce::Font(10.0f));
    addAndMakeVisible(widthLabel);
    
    widthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    widthSlider.setRange(0.0, 1.0, 0.01);
    widthSlider.setValue(audioProcessor.mySynth->reverb->getwidth());
    widthSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 18);
    widthSlider.setColour(juce::Slider::thumbColourId, juce::Colours::violet);
    widthSlider.setColour(juce::Slider::trackColourId, juce::Colours::purple);
    widthSlider.addListener(this);
    addAndMakeVisible(widthSlider);
    
    // Highpass Filter (CC68 sur canal 16)
    highpassLabel.setText("HPF (CC68)", juce::dontSendNotification);
    highpassLabel.setJustificationType(juce::Justification::centredLeft);
    highpassLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    highpassLabel.setFont(juce::Font(10.0f));
    addAndMakeVisible(highpassLabel);
    
    highpassSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    highpassSlider.setRange(20.0, 2000.0, 1.0);
    highpassSlider.setValue(audioProcessor.mySynth->getReverbHighpass());
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
    lowpassLabel.setFont(juce::Font(10.0f));
    addAndMakeVisible(lowpassLabel);
    
    lowpassSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    lowpassSlider.setRange(2000.0, 20000.0, 1.0);
    lowpassSlider.setValue(audioProcessor.mySynth->getReverbLowpass());
    lowpassSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 18);
    lowpassSlider.setTextValueSuffix(" Hz");
    lowpassSlider.setColour(juce::Slider::thumbColourId, juce::Colours::hotpink);
    lowpassSlider.setColour(juce::Slider::trackColourId, juce::Colour(139, 0, 139));
    lowpassSlider.setSkewFactorFromMidPoint(8000.0); // Logarithmique
    lowpassSlider.addListener(this);
    addAndMakeVisible(lowpassSlider);
    
    // Info CC sur canal 16
    ccInfoLabel.setText("Canal 16", juce::dontSendNotification);
    ccInfoLabel.setJustificationType(juce::Justification::centred);
    ccInfoLabel.setColour(juce::Label::textColourId, juce::Colours::yellow);
    ccInfoLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    addAndMakeVisible(ccInfoLabel);
}

ReverbComponent::~ReverbComponent()
{
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
    
    // Info CC en bas
    area.removeFromTop(5);
    ccInfoLabel.setBounds(area.removeFromTop(20));
}

void ReverbComponent::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &roomSizeSlider)
    {
        audioProcessor.mySynth->reverb->setroomsize((float)roomSizeSlider.getValue());
    }
    else if (slider == &wetSlider)
    {
        // Dry/Wet : 0=100% dry, 0.5=50/50, 1=100% wet
        float dryWetValue = (float)wetSlider.getValue();
        audioProcessor.mySynth->reverb->setwet(dryWetValue);
        audioProcessor.mySynth->reverb->setdry(1.0f - dryWetValue);
    }
    else if (slider == &dampSlider)
    {
        audioProcessor.mySynth->reverb->setdamp((float)dampSlider.getValue());
    }
    else if (slider == &widthSlider)
    {
        audioProcessor.mySynth->reverb->setwidth((float)widthSlider.getValue());
    }
    else if (slider == &highpassSlider)
    {
        audioProcessor.mySynth->setReverbHighpass((float)highpassSlider.getValue());
    }
    else if (slider == &lowpassSlider)
    {
        audioProcessor.mySynth->setReverbLowpass((float)lowpassSlider.getValue());
    }
}

void ReverbComponent::buttonClicked(juce::Button* button)
{
    if (button == &enableButton)
    {
        audioProcessor.mySynth->setReverbEnabled(enableButton.getToggleState());
    }
}

//==============================================================================
// MixerComponent - Mixeur complet
MixerComponent::MixerComponent(SirenePlugAudioProcessor& p)
    : audioProcessor(p)
{
    // Créer les 7 strips
    for (int i = 0; i < 7; i++)
    {
        strips[i] = std::make_unique<MixerStripComponent>(audioProcessor, i + 1);
        addAndMakeVisible(strips[i].get());
    }
    
    // Créer la section reverb
    reverb = std::make_unique<ReverbComponent>(audioProcessor);
    addAndMakeVisible(reverb.get());
}

MixerComponent::~MixerComponent()
{
}

void MixerComponent::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(30, 30, 30));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 10);
}

void MixerComponent::resized()
{
    auto area = getLocalBounds().reduced(10);
    int stripWidth = 70; // Réduit - un seul fader maintenant
    int reverbWidth = 210; // Pour 6 knobs
    
    // Positionner les 7 strips
    for (int i = 0; i < 7; i++)
    {
        strips[i]->setBounds(area.removeFromLeft(stripWidth));
        area.removeFromLeft(3); // Espacement
    }
    
    // Positionner la reverb
    reverb->setBounds(area);
}

//==============================================================================



//==============================================================================
SirenePlugAudioProcessorEditor::SirenePlugAudioProcessorEditor (SirenePlugAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), mainCommands(audioProcessor), mixer(audioProcessor)
{
    
    //mainCommands = new MainCommandsComponent(audioProcessor);
    
    setSize (750, 400);
    addAndMakeVisible (head);
    
    addAndMakeVisible (mainCommands);
    addAndMakeVisible (mixer);
    //mainCommands.resetButton.addListener(this);




}

SirenePlugAudioProcessorEditor::~SirenePlugAudioProcessorEditor()
{
    //mainCommands.resetButton.removeListener(this);
}

//==============================================================================
void SirenePlugAudioProcessorEditor::paint (juce::Graphics& g)
{

    g.fillAll (juce::Colour (45, 45, 45)); // background color (gris foncé)
   
}


void SirenePlugAudioProcessorEditor::resized()
{

    // void Component::setBounds  (int x, int y, int width, int height) - top left
    head.setBounds(0, 0, getWidth(), 50);
    mainCommands.setBounds (10, 55, 100, 40);
    mixer.setBounds(10, 100, getWidth() - 20, 290);

}


