//
// Created by joseph larralde on 20/02/2026.
//

#include "PluginEditor.h"

#include <colourUtilities.h>
#include <lib/definitions/palette.h>

OneSirenPluginEditor::OneSirenPluginEditor(OneSirenPluginProcessor& p) :
    juce::AudioProcessorEditor(&p),
    audioProcessor(p),
    mainButtons(p),
    voiceManager(p.getVoiceManagerState()),
    sirenStrip(
        p.getAudioProcessorValueTreeState(),
        p.getParameterLayoutData()[0]
    ),
    midiKeyboard(
        p.getMidiKeyboardState(),
        p.getVoiceManagerState()
    )
{
    audioProcessor.getVoiceManagerState().addListener(this);
    auto cat = audioProcessor.getVoiceManagerState().getSirenCategory();

    addAndMakeVisible(mainButtons);
    addAndMakeVisible(voiceManager);

    sirenStrip.setShowTitle(true);
    sirenStrip.setShowGroupLabels(true);
    sirenStrip.setShowKnobLabels(true);
    sirenStrip.setShowTextBox(true);
    sirenStrip.setBackgroundColour(
        sirenColourById.at(defaultSirenIdByCategory.at(cat))
        // juce::Colour(mecaviv::Colours::SirenPalette::darkBlue)
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
void OneSirenPluginEditor::paint (juce::Graphics& g)
{
    g.setColour(juce::Colours::black);
    g.fillRect(getLocalBounds().toFloat());
    g.setColour(juce::Colour{mecaviv::Colours::darkTransparentBackground});
    g.fillRect(getLocalBounds().toFloat());
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
    sirenStrip.setBackgroundColour(
        sirenColourById.at(defaultSirenIdByCategory.at(c))
    );
}
