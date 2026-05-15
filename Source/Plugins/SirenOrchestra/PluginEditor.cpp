//
// Created by joseph larralde on 20/02/2026.
//

#include "PluginEditor.h"
#include <colourUtilities.h>

static std::map<sirenId, std::unique_ptr<SirenTrackComponent>>
makeSirenTracks(juce::AudioProcessorValueTreeState& vts,
                std::vector<parameterLayoutGroupData>& paramGroupData,
                SirenStateMonitor& ssm)
{
    std::map<sirenId, std::unique_ptr<SirenTrackComponent>> tracks;
    for (auto& group : paramGroupData) {
        if (sirenIdByStrId.contains(group.id)) {
            auto sid = sirenIdByStrId.at(group.id);
            tracks.emplace(sid, std::make_unique<SirenTrackComponent>(sid, vts, group, ssm));
            tracks.at(sid)->setBackgroundColour(juce::Colour(sirenColourById.at(sid)));
        }
    }
    return tracks;
}

SirenOrchestraPluginEditor::SirenOrchestraPluginEditor(SirenOrchestraPluginProcessor& p) :
    AudioProcessorEditor(p),
    audioProcessor(p),
    mainButtons(p, true),
    sirenTracks(
        makeSirenTracks(
            p.getAudioProcessorValueTreeState(),
            p.getParameterLayoutData(),
            p.getSirenStateMonitor()
        )
    ),
    rvbStrip(p.getAudioProcessorValueTreeState(), "R"),
    masterVolume(p.getAudioProcessorValueTreeState(), "M"),
    midiKeyboard(p.getMidiKeyboardState(),
                 p.getVoiceManagerState(),
                 p.getSirenStateMonitor()),
    sirenStripMenu(sirenTracks)
{
    sirenStripMenu.setListener(this);

    addAndMakeVisible(mainButtons);

    for (auto i : sirenOrder) {
        addAndMakeVisible(sirenTracks.at(i).get());
    }

    addAndMakeVisible(rvbStrip);
    addAndMakeVisible(masterVolume);

    audioProcessor.getVoiceManagerState().addListener(VoiceManagerState::Listener::Key::midiInput, this);

    auto inch = audioProcessor.getVoiceManagerState().getMidiInput();
    if (inch.isAny) { inch = AnyOrOneBasedMidiChannel::specific({1}); }
    sirenStripMenu.setSelectedSirenTrack(sirenPropertiesByChannel.at(inch.channel)->id);
    addAndMakeVisible(midiKeyboard);

    bottomColour = sirenColourById.at(sirenOrder.back());

    int sirenWidth = static_cast<int>(sirenTracks.at(sirenOrder[0])->getMinWidth());
    setSize(sirenWidth, 630);
}

SirenOrchestraPluginEditor::~SirenOrchestraPluginEditor()
{
    audioProcessor.getVoiceManagerState()
                  .removeListener(VoiceManagerState::Listener::Key::midiInput,
                                  this);
    sirenStripMenu.removeListener();
}

void SirenOrchestraPluginEditor::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::black);
    g.fillRect(getLocalBounds().toFloat());
    g.setColour(juce::Colour{mecaviv::Colours::darkTransparentBackground});
    g.fillRect(getLocalBounds().toFloat());
}

void SirenOrchestraPluginEditor::resized()
{
    constexpr int spacer = static_cast<int>(controlStripLayout::spacerSize);
    constexpr int minSirenHeight = 55;
    constexpr int fullSirenHeight = static_cast<int>(controlStripLayout::minFullStripHeight);

    int sirenWidth = static_cast<int>(sirenTracks.at(sirenOrder[0])->getMinWidth());
    int sirenTitleWidth = static_cast<int>(sirenTracks.at(sirenOrder[0])->getTitleWidth());
    int sirenControlsWidth = static_cast<int>(sirenTracks.at(sirenOrder[0])->getSirenControlsWidth());
    int sirenTrackControlsWidth = static_cast<int>(sirenTracks.at(sirenOrder[0])->getTrackControlsWidth());

    constexpr int mainButtonsHeight = 30;
    mainButtons.setBounds(0, 0, sirenWidth, mainButtonsHeight);

    constexpr int tracksY = mainButtonsHeight;
    for (std::size_t i = 0; i < sirenOrder.size(); ++i) {
        auto& track = sirenTracks.at(sirenOrder[i]);
        track->setBackgroundColour(juce::Colour(sirenColourById.at(sirenOrder[i])));
        track->setTitle(sirenTitleById.at(sirenOrder[i]));

        if (i==0) {
            track->setShowGroupLabels(true);
            track->setShowKnobLabels(true);
            track->setShowTextBox(true);
            track->setBounds(
                0,
                tracksY,
                sirenWidth,
                fullSirenHeight
            );
        } else {
            track->setShowGroupLabels(false);
            track->setShowKnobLabels(false);
            track->setShowTextBox(true);
            track->setBounds(
                0,
                tracksY + fullSirenHeight + spacer + static_cast<int>(i - 1) * (minSirenHeight + spacer),
                sirenWidth,
                minSirenHeight
            );
        }
    }

    constexpr int reverbY = tracksY + fullSirenHeight + spacer +
                            static_cast<int>(sirenOrder.size() - 1) * (minSirenHeight + spacer);
    constexpr int reverbH = fullSirenHeight - static_cast<int>(controlStripLayout::groupLabelHeight);

    rvbStrip.setTitle("Reverb");
    rvbStrip.setShowTitle(true);
    rvbStrip.setShowGroupLabels(false);
    rvbStrip.setShowKnobLabels(true);
    rvbStrip.setShowTextBox(true);
    rvbStrip.setBounds(
        spacer,
        reverbY,
        sirenControlsWidth - 2 * spacer,
        reverbH
    );
    rvbStrip.setBackgroundColour(juce::Colour{0x22ffffff});
    rvbStrip.setCellBackgroundColour(juce::Colours::transparentBlack);
    rvbStrip.setBackgroundStripColour(bottomColour);

    constexpr int keyboardH = 70;

    masterVolume.setTitle("Master Volume");
    masterVolume.setShowTitle(true);
    masterVolume.setShowGroupLabels(false);
    masterVolume.setShowKnobLabels(true);
    masterVolume.setShowTextBox(true);
    masterVolume.setBounds(
        sirenControlsWidth,
        reverbY,
        sirenTrackControlsWidth + sirenTitleWidth - spacer,
        reverbH + keyboardH + spacer
    );
    masterVolume.setBackgroundColour(juce::Colour{0x22ffffff});
    masterVolume.setCellBackgroundColour(juce::Colours::transparentBlack);
    masterVolume.setBackgroundStripColour(bottomColour);

    constexpr int keyboardY = reverbY + reverbH + spacer;
    midiKeyboard.setBounds(0, keyboardY, sirenControlsWidth, keyboardH);
}

void SirenOrchestraPluginEditor::midiInputChanged(AnyOrOneBasedMidiChannel inch)
{
    if (inch.isAny) { inch = AnyOrOneBasedMidiChannel::specific({1}); }
    midiKeyboard.setCurrentChannel(inch.channel);
}

void SirenOrchestraPluginEditor::sirenStripMenuItemSelected(std::optional<sirenId> s)
{
    mainButtons.setSirenIdToReset(s);

    if (s.has_value()) {
        AnyOrOneBasedMidiChannel ch{false, sirenPropertiesById.at(s.value())->oneBasedMidiChannel};
        audioProcessor.getVoiceManagerState().setMidiInput(ch, true);
    } else {
        audioProcessor.getVoiceManagerState().setMidiInput(AnyOrOneBasedMidiChannel::any(), true);
    }
}
