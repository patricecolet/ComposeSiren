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

    wood = juce::Drawable::createFromImageData(
        BinaryData::boiserie_png,
        BinaryData::boiserie_pngSize
    );

    bottomColour = sirenColourById.at(sirenOrder.back());

    int sirenWidth = static_cast<int>(sirenTracks.at(sirenOrder[0])->getMinWidth());
    setSize(sirenWidth + 2 * woodThickness, 630);
}

SirenOrchestraPluginEditor::~SirenOrchestraPluginEditor()
{
    audioProcessor.getVoiceManagerState()
                  .removeListener(VoiceManagerState::Listener::Key::midiInput,
                                  this);
    sirenStripMenu.removeListener();
}
//==============================================================================
// AudioProcessorEditor
void SirenOrchestraPluginEditor::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::black);
    g.fillRect(getLocalBounds().toFloat());
    g.setColour(juce::Colour{mecaviv::Colours::darkTransparentBackground});
    g.fillRect(getLocalBounds().toFloat());

    const int& spacer = controlStripLayout::spacerSize;
    int sirenWidth = static_cast<int>(sirenTracks.at(sirenOrder[0])->getMinWidth());
    const int ensembleHeight = (sirenOrder.size() - 1) * (55 + spacer) + 200;

    bool drawWood = woodThickness > 0;
    if (drawWood) {
        // wood borders
        auto rect1 = juce::Rectangle<float>(0, 0, woodThickness, ensembleHeight);
        wood->drawWithin(g, rect1, juce::RectanglePlacement::stretchToFit, 1.0f);
        auto rect2 = rect1.withX(sirenWidth + woodThickness);
        wood->drawWithin(g, rect2, juce::RectanglePlacement::stretchToFit, 1.0f);

        // wood borders inner shadows
        juce::Path rectPath;
        rectPath.addRectangle(rect1);
        rectPath.addRectangle(rect2);
        // invert the path's fill shape and enlarge it,
        // so it casts a shadow
        juce::Path shadowPath(rectPath);
        shadowPath.addRectangle(shadowPath.getBounds().expanded(10));
        shadowPath.setUsingNonZeroWinding(false);
        // reduce clip region to avoid the shadow
        // being drawn outside of the shape to cast the shadow on
        g.reduceClipRegion(rectPath);

        juce::DropShadow ds(juce::Colours::black, 5, {0, 0});
        ds.drawForPath(g, shadowPath);
    }
}

void SirenOrchestraPluginEditor::resized()
{
    const int& spacer = controlStripLayout::spacerSize;
    int sirenWidth = static_cast<int>(sirenTracks.at(sirenOrder[0])->getMinWidth());
    int sirenTitleWidth = static_cast<int>(sirenTracks.at(sirenOrder[0])->getTitleWidth());
    int sirenControlsWidth = static_cast<int>(sirenTracks.at(sirenOrder[0])->getSirenControlsWidth());
    int sirenTrackControlsWidth = static_cast<int>(sirenTracks.at(sirenOrder[0])->getTrackControlsWidth());

    int mainButtonsHeight = 25;
    mainButtons.setBounds(0, 0, sirenWidth, mainButtonsHeight);

    int tracksy = 30;
    for (std::size_t i = 0; i < sirenOrder.size(); ++i) {
        auto& track = sirenTracks.at(sirenOrder[i]);
        track->setBackgroundColour(juce::Colour(sirenColourById.at(sirenOrder[i])));
        track->setTitle(sirenTitleById.at(sirenOrder[i]));
        // track->setShowTitle(true);

        if (i==0) {
            track->setShowGroupLabels(true);
            track->setShowKnobLabels(true);
            track->setShowTextBox(true);
            track->setBounds(woodThickness, tracksy, sirenWidth, controlStripLayout::minFullStripHeight);
        } else {
            track->setShowGroupLabels(false);
            track->setShowKnobLabels(false);
            track->setShowTextBox(true);
            track->setBounds(
                woodThickness,
                tracksy + (i - 1) * (minSirenHeight + spacer) + controlStripLayout::minFullStripHeight + spacer,
                sirenWidth,
                minSirenHeight
            );
        }
    }

    int reverby = tracksy +
                  (sirenOrder.size() - 1) * (minSirenHeight + spacer) +
                  controlStripLayout::minFullStripHeight + spacer;
    int reverbh = controlStripLayout::minFullStripHeight - controlStripLayout::groupLabelHeight;// - 2 * controlStripLayout::spacerSize;

    rvbStrip.setTitle("Reverb");
    rvbStrip.setShowTitle(true);
    rvbStrip.setShowGroupLabels(false);
    rvbStrip.setShowKnobLabels(true);
    rvbStrip.setShowTextBox(true);
    rvbStrip.setBounds(
        woodThickness + controlStripLayout::spacerSize,
        reverby,
        sirenControlsWidth - 2 * controlStripLayout::spacerSize,
        reverbh
    );
    // rvbStrip.setBackgroundColour(juce::Colours::transparentBlack);
    // rvbStrip.setBackgroundColour(bottomColour);
    rvbStrip.setBackgroundColour(juce::Colour{0x22ffffff});
    // rvbStrip.setBackgroundColour(juce::Colour{mecaviv::Colours::SirenPalette::darkBlue});
    rvbStrip.setCellBackgroundColour(juce::Colours::transparentBlack);
    // rvbStrip.setCellBackgroundColour(juce::Colour{0x22ffffff});
    // rvbStrip.setCellBackgroundColour(bottomColour);
    rvbStrip.setBackgroundStripColour(bottomColour);

    int midiKeyboardHeight = 70;

    masterVolume.setTitle("Master Volume");
    masterVolume.setShowTitle(true);
    masterVolume.setShowGroupLabels(false);
    masterVolume.setShowKnobLabels(true);
    masterVolume.setShowTextBox(true);
    masterVolume.setBounds(
        woodThickness + sirenControlsWidth,
        reverby,
        sirenTrackControlsWidth + sirenTitleWidth - controlStripLayout::spacerSize,
        reverbh + midiKeyboardHeight + spacer
    );
    // masterVolume.setBackgroundColour(juce::Colours::transparentBlack);
    // masterVolume.setBackgroundColour(bottomColour);
    masterVolume.setBackgroundColour(juce::Colour{0x22ffffff});
    // masterVolume.setBackgroundColour(juce::Colour{mecaviv::Colours::SirenPalette::darkBlue});
    masterVolume.setCellBackgroundColour(juce::Colours::transparentBlack);
    // masterVolume.setCellBackgroundColour(bottomColour);
    masterVolume.setBackgroundStripColour(bottomColour);

    int nexty = reverby + reverbh + spacer;
    midiKeyboard.setBounds(woodThickness, nexty, sirenControlsWidth, midiKeyboardHeight);
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
