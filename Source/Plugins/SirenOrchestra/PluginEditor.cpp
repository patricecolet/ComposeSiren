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
#if COMPOSESIREN_PARK_BRIDGE
    mainButtons(p, true, true),
#else
    mainButtons(p, true, false),
#endif
    sirenTracks(
        makeSirenTracks(
            p.getAudioProcessorValueTreeState(),
            p.getParameterLayoutData(),
            p.getSirenStateMonitor()
        )
    ),
    rvbStrip(p.getAudioProcessorValueTreeState(), "R"),
#if COMPOSESIREN_CLIC
    clicPane(p.getAudioProcessorValueTreeState()),
#endif
    masterVolume(p.getAudioProcessorValueTreeState(), "M"),
    midiKeyboard(p.getMidiKeyboardState(),
                 p.getVoiceManagerState(),
                 p.getSirenStateMonitor()),
    sirenStripMenu(sirenTracks)
{
    endTeardown.processor = &p;
    sirenStripMenu.setListener(this);

    addAndMakeVisible(mainButtons);

    for (auto i : sirenOrder) {
        addAndMakeVisible(sirenTracks.at(i).get());
#if COMPOSESIREN_PARK_BRIDGE
        stLeds.emplace(i, std::make_unique<StLedComponent>());
        addAndMakeVisible(stLeds.at(i).get());
#endif
    }
#if COMPOSESIREN_PARK_BRIDGE
    startTimerHz(4); // rafraîchissement des LEDs d'état ST
#endif

#if COMPOSESIREN_SIREN_WAVES
    addChildComponent(wavesOverflow); // above the tracks
#endif

    addAndMakeVisible(rvbStrip);
#if COMPOSESIREN_CLIC
    addAndMakeVisible(clicPane);
#endif
    addAndMakeVisible(masterVolume);

    audioProcessor.getVoiceManagerState().addListener(VoiceManagerState::Listener::Key::midiInput, this);

    auto inch = audioProcessor.getVoiceManagerState().getMidiInput();
    if (inch.isAny) { inch = AnyOrOneBasedMidiChannel::specific({1}); }
    sirenStripMenu.setSelectedSirenTrack(sirenPropertiesByChannel.at(inch.channel)->id);
    addAndMakeVisible(midiKeyboard);

    bottomColour = sirenColourById.at(sirenOrder.back());

    int sirenWidth = static_cast<int>(sirenTracks.at(sirenOrder[0])->getMinWidth());
    setSize(sirenWidth, 630);

#if COMPOSESIREN_SIREN_WAVES
    audioProcessor.getSirenStateMonitor().addListener(this);
    settings->addListener(this);
    waveTuning = cs::waves::Tuning::from(*settings);
    waveStats.on = juce::SystemStats::getEnvironmentVariable("COMPOSESIREN_WAVES_STATS", {}).isNotEmpty();
    waveColumn.setTitleFont(juce::FontOptions(controlStripLayout::titleFontSize, juce::Font::bold));
    vblank = std::make_unique<juce::VBlankAttachment>(this, [this](double now) { advanceWaves(now); });
    updateWavesSetup();
#endif
}

SirenOrchestraPluginEditor::~SirenOrchestraPluginEditor()
{
    // Freeze message-thread callbacks into this hierarchy before any member
    // dies. Cmd+Q was racing MCP pump / SirenStateMonitor into WavesOverflow
    // (see docs/bug-reports/sirenorchestra/cmd-q-crash-wavesoverflow-string-release).
    audioProcessor.beginEditorTeardown();
    audioProcessor.getSirenStateMonitor().removeAllListeners();

#if COMPOSESIREN_SIREN_WAVES
    vblank.reset();
    settings->removeListener(this);
    glContext.detach();
    // Quiet and detach the overflow painter: its Component strings are what
    // faulted if anything still writes into this object mid-teardown.
    wavesOverflow.silence();
    removeChildComponent(&wavesOverflow);
    waveColumn.setCells({});
#endif
#if COMPOSESIREN_PARK_BRIDGE
    stopTimer();
#endif
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
#if COMPOSESIREN_SIREN_WAVES
    if (waveTuning.enabled && g.clipRegionIntersects(waveColumnBounds())) {
        const double start = juce::Time::getMillisecondCounterHiRes();
        waveColumn.paintCells(g, waveTuning, controlStripLayout::cornerSize);
        if (waveStats.on) {
            const double ms = juce::Time::getMillisecondCounterHiRes() - start;
            ++waveStats.paints;
            waveStats.paintMs += ms;
            waveStats.paintMaxMs = juce::jmax(waveStats.paintMaxMs, ms);
        }
    }
#endif
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

#if COMPOSESIREN_PARK_BRIDGE
        // LED d'état ST, à gauche du nom dans la zone de titre du strip
        constexpr int ledSize = 9;
        const auto trackBounds = track->getBounds();
        stLeds.at(sirenOrder[i])->setBounds(
            trackBounds.getRight() - sirenTitleWidth + 4,
            trackBounds.getCentreY() - ledSize / 2,
            ledSize,
            ledSize
        );
#endif
    }

    constexpr int reverbY = tracksY + fullSirenHeight + spacer +
                            static_cast<int>(sirenOrder.size() - 1) * (minSirenHeight + spacer);
    constexpr int reverbH = fullSirenHeight - static_cast<int>(controlStripLayout::groupLabelHeight);

    rvbStrip.setTitle("Reverb");
    rvbStrip.setShowTitle(true);
    rvbStrip.setShowGroupLabels(false);
    rvbStrip.setShowKnobLabels(true);
    rvbStrip.setShowTextBox(true);

    constexpr int keyboardH = 70;
#if COMPOSESIREN_CLIC
    // Single Clic pane between reverb/keyboard and Master (validated mockup:
    // clicBorrow 136, master ~119 wide — no Clic block inside Master).
    constexpr int clicBorrow = 136;
    clicPane.setBounds(sirenControlsWidth - clicBorrow, reverbY, clicBorrow,
                       reverbH + keyboardH + spacer);
    masterVolume.setBounds(
        sirenControlsWidth,
        reverbY,
        sirenTrackControlsWidth + sirenTitleWidth - spacer,
        reverbH + keyboardH + spacer
    );
    rvbStrip.setBounds(
        spacer,
        reverbY,
        sirenControlsWidth - 2 * spacer - clicBorrow,
        reverbH
    );
    constexpr int keyboardY = reverbY + reverbH + spacer;
    midiKeyboard.setBounds(0, keyboardY, sirenControlsWidth - clicBorrow, keyboardH);
#else
    constexpr int clicBorrow = 0;
    masterVolume.setBounds(
        sirenControlsWidth,
        reverbY,
        sirenTrackControlsWidth + sirenTitleWidth - spacer,
        reverbH + keyboardH + spacer
    );
    rvbStrip.setBounds(
        spacer,
        reverbY,
        sirenControlsWidth - 2 * spacer,
        reverbH
    );
    constexpr int keyboardY = reverbY + reverbH + spacer;
    midiKeyboard.setBounds(0, keyboardY, sirenControlsWidth, keyboardH);
#endif
    rvbStrip.setBackgroundColour(juce::Colour{0x22ffffff});
    rvbStrip.setCellBackgroundColour(juce::Colours::transparentBlack);
    rvbStrip.setBackgroundStripColour(bottomColour);

#if COMPOSESIREN_CLIC
    clicPane.setBackgroundStripColour(bottomColour);
#endif

    masterVolume.setTitle("Master Volume");
    masterVolume.setShowTitle(true);
    masterVolume.setShowGroupLabels(false);
    masterVolume.setShowKnobLabels(true);
    masterVolume.setShowTextBox(true);
    masterVolume.setBackgroundColour(juce::Colour{0x22ffffff});
    masterVolume.setCellBackgroundColour(juce::Colours::transparentBlack);
    masterVolume.setBackgroundStripColour(bottomColour);
#if COMPOSESIREN_SIREN_WAVES
    updateWaveCells();
#endif
}

#if COMPOSESIREN_PARK_BRIDGE
void SirenOrchestraPluginEditor::timerCallback()
{
    for (auto& [id, led] : stLeds) {
        const int siren = sirenPropertiesById.at(id)->oneBasedMidiChannel.oneBased;
        led->setState(audioProcessor.getUdpBridge().getStState(siren));
    }
    mainButtons.refreshPhysicalSirensTooltip();
}
#endif

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

#if COMPOSESIREN_SIREN_WAVES
// WAVES ///////////////////////////////////////////////////////////////////////

void SirenOrchestraPluginEditor::currentSirenState(const sirenId sid, const SirenVoice::State& s)
{
    waveColumn.setSirenState(sid, s.level, s.currentPitch, s.isNoteOn);
    if (waveStats.on && s.level > 0.0f) {
        auto& peak = waveStats.peakDb.try_emplace(sid, -200.0f).first->second;
        peak = juce::jmax(peak, 20.0f * std::log10(s.level));
    }
}

void SirenOrchestraPluginEditor::settingChanged(cs::Settings::Id id)
{
    using Id = cs::Settings::Id;
    waveTuning = cs::waves::Tuning::from(*settings);
    if (id == Id::wavesEnabled || id == Id::wavesRenderer || id == Id::wavesOverflow
        || id == Id::wavesOverflowReach)
        updateWavesSetup();
    else
        repaint();
}

// The OpenGL context goes with the renderer setting: with it, JUCE renders
// the whole editor through OpenGL, and the cells use the shader.
void SirenOrchestraPluginEditor::updateWavesSetup()
{
    using Renderer = cs::waves::Tuning::Renderer;
    const bool on = waveTuning.enabled;
    for (auto& track : sirenTracks | std::views::values)
        track->setTitleFillVisible(!on);

    const bool wantOpenGL = on && waveTuning.renderer != Renderer::software;
    if (wantOpenGL && !glContext.isAttached())
        glContext.attachTo(*this);
    else if (!wantOpenGL && glContext.isAttached())
        glContext.detach();

    wavesOverflow.setVisible(on && waveTuning.overflow);
    updateWaveCells();
    repaint();
}

void SirenOrchestraPluginEditor::updateWaveCells()
{
    std::vector<cs::waves::SirenWaveColumn::Cell> cells;
    juce::Rectangle<float> column;
    for (auto id : sirenOrder) {
        const auto& track = sirenTracks.at(id);
        const auto area = track->getTitleArea() + track->getPosition().toFloat();
        cells.push_back({ id, area, track->getBackgroundColour(), track->getTitleText() });
        column = column.isEmpty() ? area : column.getUnion(area);
    }
    waveColumn.setCells(std::move(cells));

    // room for the furthest overflow: the reach, the parallax layers, the ripples
    const float room = waveTuning.overflowReach * 1.5f + 20.0f;
    wavesOverflow.setBounds(juce::Rectangle<float>(column.getX() - room, column.getY(), room, column.getHeight())
                                .getSmallestIntegerContainer());
}

juce::Rectangle<int> SirenOrchestraPluginEditor::waveColumnBounds() const
{
    juce::Rectangle<float> column;
    for (const auto& c : waveColumn.getCells())
        column = column.isEmpty() ? c.bounds : column.getUnion(c.bounds);
    return column.getSmallestIntegerContainer();
}

void SirenOrchestraPluginEditor::advanceWaves(double now)
{
    if (!waveTuning.enabled) {
        lastWaveFrame = 0.0;
        return;
    }
    // the frame rate cap: skip a display refresh only when it comes well
    // before the next frame is due (refreshes arrive late, then early)
    if (lastWaveFrame > 0.0 && now - lastWaveFrame < 1.0 / waveTuning.frameRate - 1.0 / 120.0)
        return;
    const double dt = lastWaveFrame > 0.0 ? juce::jlimit(0.0, 0.1, now - lastWaveFrame) : 0.0;
    lastWaveFrame = now;

    const bool moving = waveColumn.advance(dt, waveTuning);
    if (waveStats.on) {
        ++waveStats.frames;
        if (now - waveStats.since >= 1.0) {
            juce::String peaks;
            for (const auto& [id, db] : waveStats.peakDb)
                peaks << " " << sirenStrIdById.at(id) << " " << juce::String(db, 1);
            std::fprintf(stderr, "waves: %d frames %d paints, cells %.2f ms avg %.2f max, %s, peak dB:%s\n",
                         waveStats.frames, waveStats.paints,
                         waveStats.paints > 0 ? waveStats.paintMs / waveStats.paints : 0.0, waveStats.paintMaxMs,
                         waveColumn.lastPaintUsedOpenGL() ? "OpenGL" : "software", peaks.toRawUTF8());
            waveStats = { true, 0, 0, 0.0, 0.0, now, {} };
        }
    }
    if (moving || wavesMoving) { // one more frame to settle
        repaint(waveColumnBounds());
        if (wavesOverflow.isVisible())
            wavesOverflow.repaint();
    }
    wavesMoving = moving;
}

void SirenOrchestraPluginEditor::WavesOverflow::paint(juce::Graphics& g)
{
    if (quiet)
        return;
    editor.waveColumn.paintOverflow(g, editor.waveTuning, getPosition().toFloat());
}

void SirenOrchestraPluginEditor::WavesOverflow::silence()
{
    quiet = true;
    setVisible(false);
}
#endif
