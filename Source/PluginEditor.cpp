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

  std::string label = "COMPOSE SIREN " + concatenatedString;
  labelPluginTitle.setText(label, juce::dontSendNotification);
  labelPluginTitle.setFont (juce::Font (24.0f, juce::Font::italic));
  labelPluginTitle.setJustificationType (juce::Justification::centredLeft);
  labelPluginTitle.setColour (juce::Label::textColourId, juce::Colours::whitesmoke);

  std::string description = "custom-mix - " + std::string(PROJECT_DESCRIPTION);
  labelPluginSubTitle.setText(description, juce::dontSendNotification);
  labelPluginSubTitle.setFont (juce::Font (12.0f, juce::Font::italic));
  labelPluginSubTitle.setJustificationType (juce::Justification::centredLeft);
  labelPluginSubTitle.setColour (juce::Label::textColourId, juce::Colours::lightgrey);

  imgLogo = juce::ImageFileFormat::loadFrom(BinaryData::Picto_Siren_40x37_png, BinaryData::Picto_Siren_40x37_pngSize);

}

void headComponent::paint (juce::Graphics& g)
{
  // g.fillAll (juce::Colour (155, 153, 100)); // background color to see the component
  // g.drawImageAt(imgLogo, labelPluginTitle.getX() + labelPluginTitle.getWidth(), 0);
  g.drawImageAt(imgLogo, getWidth()/2 + 90, 5);
  // g.drawImageAt(imgLogo, 10, 10);
}


void headComponent::resized()
{
  auto area = getLocalBounds().reduced(10, 2);
  labelPluginTitle.setBounds(area.removeFromTop(28));
  labelPluginSubTitle.setBounds(area.removeFromTop(20));

  // auto area = getLocalBounds();
  // labelPluginTitle.setBounds(area.removeFromTop(25));
  // labelPluginSubTitle.setBounds(area.removeFromTop(25));
}
//==============================================================================

//==============================================================================
SirenePlugAudioProcessorEditor::SirenePlugAudioProcessorEditor (SirenePlugAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), mainCommands(audioProcessor), mixer(audioProcessor)
        //, mixer(audioProcessor)
{
    /*
    const int NB_SIRENES = 7;
    for (std::size_t i = 0; i < NB_SIRENES; ++i) {
        orchestra.push_back(std::make_unique<SirenStripComponent>(p));
    }
    //*/

    setSize (800, 400);
    addAndMakeVisible (head);
    addAndMakeVisible (mainCommands);
    addAndMakeVisible(mixer);
    //mainCommands.resetButton.addListener(this);

    /*
    auto strip1 = orchestra[0].get();
    auto strip2 = orchestra[1].get();
    auto strip3 = orchestra[2].get();
    auto strip4 = orchestra[3].get();
    auto strip5 = orchestra[4].get();
    auto strip6 = orchestra[5].get();
    auto strip7 = orchestra[6].get();

    strip1->setShowTitle(true);
    strip1->setShowGroupLabels(true);
    strip1->setShowKnobLabels(true);
    strip1->setShowTextBox(true);
    strip1->setBackgroundColour(juce::Colour{70,80,200});
    addAndMakeVisible(strip1);

    strip2->setShowTitle(true);
    strip2->setShowGroupLabels(false);
    strip2->setShowKnobLabels(false);
    strip2->setShowTextBox(true);
    strip2->setBackgroundColour(juce::Colour{0,180,200});
    addAndMakeVisible(strip2);

    strip3->setShowTitle(true);
    strip3->setShowGroupLabels(false);
    strip3->setShowKnobLabels(false);
    strip3->setShowTextBox(true);
    strip3->setBackgroundColour(juce::Colour{60,140,40});
    addAndMakeVisible(strip3);

    strip4->setShowTitle(true);
    strip4->setShowGroupLabels(false);
    strip4->setShowKnobLabels(false);
    strip4->setShowTextBox(true);
    strip4->setBackgroundColour(juce::Colour{120,180,40});
    addAndMakeVisible(strip4);

    strip5->setShowTitle(true);
    strip5->setShowGroupLabels(false);
    strip5->setShowKnobLabels(false);
    strip5->setShowTextBox(true);
    // strip5.setBackgroundColour(juce::Colours::goldenrod);
    strip5->setBackgroundColour(juce::Colour{215, 183, 0});
    addAndMakeVisible(strip5);

    strip6->setShowTitle(true);
    strip6->setShowGroupLabels(false);
    strip6->setShowKnobLabels(false);
    strip6->setShowTextBox(true);
    strip6->setBackgroundColour(juce::Colour{255, 127, 0});
    addAndMakeVisible(strip6);

    strip7->setShowTitle(true);
    strip7->setShowGroupLabels(false);
    strip7->setShowKnobLabels(false);
    strip7->setShowTextBox(true);
    strip7->setBackgroundColour(juce::Colours::orangered);
    addAndMakeVisible(strip7);
    //*/
}

SirenePlugAudioProcessorEditor::~SirenePlugAudioProcessorEditor()
{
    //mainCommands.resetButton.removeListener(this);
}

//==============================================================================
void SirenePlugAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (45, 45, 45)); // background color (gris foncé)
    // g.fillAll (juce::Colour (255, 153, 0)); // background color (orange)
}


void SirenePlugAudioProcessorEditor::resized()
{

    // void Component::setBounds  (int x, int y, int width, int height) - top left
    head.setBounds(0, 0, getWidth(), 50);
    mainCommands.setBounds (10, 50, getWidth() - 20, 50);
    mixer.setBounds(10, 100, getWidth() - 20, 290);
    /*
    auto strip1 = orchestra[0].get();
    strip1->setBounds(10, 90, getWidth() - 20, 99);

    auto strip2 = orchestra[1].get();
    strip2->setBounds(10, 189, getWidth() - 20, 73);

    auto strip3 = orchestra[2].get();
    strip3->setBounds(10, 262, getWidth() - 20, 73);

    auto strip4 = orchestra[3].get();
    strip4->setBounds(10, 335, getWidth() - 20, 73);

    auto strip5 = orchestra[4].get();
    strip5->setBounds(10, 408, getWidth() - 20, 73);

    auto strip6 = orchestra[5].get();
    strip6->setBounds(10, 481, getWidth() - 20, 73);

    auto strip7 = orchestra[6].get();
    strip7->setBounds(10, 554, getWidth() - 20, 73);
    //*/
}


