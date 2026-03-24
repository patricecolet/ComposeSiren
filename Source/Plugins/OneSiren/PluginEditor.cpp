//
// Created by joseph larralde on 20/02/2026.
//

#include "PluginEditor.h"
#include "lib/definitions/palette.h"

OneSirenPluginEditor::OneSirenPluginEditor(OneSirenPluginProcessor& p) :
    juce::AudioProcessorEditor(&p),
    audioProcessor(p),
    mainButtons(p),
    voiceManager(p.getVoiceManagerState()),
    sirenStrip(p.getAudioProcessorValueTreeState(), "S"),
    midiKeyboard(p.getMidiKeyboardState(), p.getVoiceManagerState())
{
    audioProcessor.getVoiceManagerState().addListener(this);
    addAndMakeVisible(mainButtons);
    addAndMakeVisible(voiceManager);

    sirenStrip.setShowTitle(true);
    sirenStrip.setShowGroupLabels(true);
    sirenStrip.setShowKnobLabels(true);
    sirenStrip.setShowTextBox(true);
    sirenStrip.setBackgroundColour(
        juce::Colour(mecaviv::Colours::SirenPalette::darkBlue)
    );
    addAndMakeVisible(sirenStrip);
    addAndMakeVisible(midiKeyboard);

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(754, 200);
}

OneSirenPluginEditor::~OneSirenPluginEditor()
{
    audioProcessor.getVoiceManagerState().removeListener(this);
}
//==============================================================================
// AudioProcessorEditor
void OneSirenPluginEditor::paint (juce::Graphics&)
{
    // nothing for now
}

void OneSirenPluginEditor::resized()
{
    int sirenWidth = static_cast<int>(sirenStrip.getMinWidth());
    mainButtons.setBounds(0,0,sirenWidth,25);
    voiceManager.setBounds(0,0,sirenWidth/2,30);
    sirenStrip.setBounds(0,30,sirenWidth,100);
    midiKeyboard.setBounds(0, 130, sirenWidth, 70);
}

void OneSirenPluginEditor::categoryChanged(sirenCategory c)
{
    // sirenStrip.setBackgroundColour(
    //     juce::Colour(sirenCategoriesData.at(c).colour)
    // );
}