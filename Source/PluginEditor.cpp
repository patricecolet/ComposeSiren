/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "config.h"
#include <iostream>
#include <sstream>

//==============================================================================
// Component on the top of the main window
headComponent::headComponent()
{
  addAndMakeVisible (labelPluginTitle);
  addAndMakeVisible (labelPluginSubTitle);

  labelPluginTitle.setColour (juce::Label::textColourId, juce::Colours::black);

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
  labelPluginTitle.setFont (juce::Font (24.0f, juce::Font::italic));
  labelPluginTitle.setJustificationType (juce::Justification::centred);
  labelPluginSubTitle.setText(PROJECT_DESCRIPTION, juce::dontSendNotification);
  labelPluginSubTitle.setFont (juce::Font (12.0f, juce::Font::italic));
  labelPluginSubTitle.setJustificationType (juce::Justification::left);
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
  auto area = getLocalBounds();
  labelPluginTitle.setBounds(area.removeFromTop(30));
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
SirenePlugAudioProcessorEditor::SirenePlugAudioProcessorEditor (SirenePlugAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), mainCommands(audioProcessor)
{
    
    //mainCommands = new MainCommandsComponent(audioProcessor);
    
    setSize (300, 200);
    addAndMakeVisible (head);
    
    addAndMakeVisible (mainCommands);
    //mainCommands.resetButton.addListener(this);




}

SirenePlugAudioProcessorEditor::~SirenePlugAudioProcessorEditor()
{
    //mainCommands.resetButton.removeListener(this);
}

//==============================================================================
void SirenePlugAudioProcessorEditor::paint (juce::Graphics& g)
{

    g.fillAll (juce::Colour (255, 153, 0)); // background color (orange)
   
}


void SirenePlugAudioProcessorEditor::resized()
{

    // void Component::setBounds  (int x, int y, int width, int height) - top left
    head.setBounds(0, 0, getWidth(), 50);
    mainCommands.setBounds (10, 50, 100, 40);

}


