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
    voiceManager(
        p.getVoiceManagerState(),
        p.getSirenStateMonitor()
    ),
    sirenStrip(
        p.getAudioProcessorValueTreeState(),
        p.getParameterLayoutData()[0]
    ),
    midiKeyboard(
        p.getMidiKeyboardState(),
        p.getVoiceManagerState(),
        p.getSirenStateMonitor()
    )
{
    addAndMakeVisible(mainButtons);
    addAndMakeVisible(voiceManager);

    audioProcessor.getVoiceManagerState().addListener(this);
    auto cat = audioProcessor.getVoiceManagerState().getSirenCategory();

    sirenStrip.setShowGroupLabels(true);
    sirenStrip.setShowKnobLabels(true);
    sirenStrip.setShowTextBox(true);
    sirenStrip.setBackgroundColour(
        sirenColourById.at(defaultSirenIdByCategory.at(cat))
    );
    addAndMakeVisible(sirenStrip);

    auto inch = audioProcessor.getVoiceManagerState().getMidiInput();
    if (inch.isAny) { inch = AnyOrOneBasedMidiChannel::specific({1}); }
    midiKeyboard.setCurrentChannel(inch.channel);
    midiKeyboard.setCurrentSirenCategory(cat);

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
    constexpr int sirenHeight = static_cast<int>(controlStripLayout::minFullStripHeight);
    int sirenWidth = static_cast<int>(sirenStrip.getMinWidth());

    mainButtons.setBounds(0, 0, sirenWidth, 30);
    voiceManager.setBounds(0, 0, voiceManager.getMinWidth(), 30);
    sirenStrip.setBounds(0, 30, sirenWidth, sirenHeight);
    midiKeyboard.setBounds(0, sirenHeight + 30, sirenWidth, 70);
}

void OneSirenPluginEditor::categoryChanged(sirenCategory c)
{
    sirenStrip.setBackgroundColour(
        sirenColourById.at(defaultSirenIdByCategory.at(c))
    );
}

void OneSirenPluginEditor::midiInputChanged(AnyOrOneBasedMidiChannel inch)
{
    if (inch.isAny) { inch = AnyOrOneBasedMidiChannel::specific({1}); }
    midiKeyboard.setCurrentChannel(inch.channel);
}
