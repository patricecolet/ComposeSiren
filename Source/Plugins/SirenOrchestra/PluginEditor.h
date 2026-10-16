//
// Created by joseph larralde on 20/02/2026.
//

#ifndef SIRENORCHESTRA_PLUGINEDITOR_H
#define SIRENORCHESTRA_PLUGINEDITOR_H

#include <juce_audio_processors/juce_audio_processors.h>
#include <Components/MainButtonsComponent.h>
#include <Components/SirenTrackComponent.h>
#include <Components/DbRangesMidiKeyboardComponent.h>
#include <Components/ReverbStripComponent.h>
#include <Components/MasterVolumeComponent.h>
#if COMPOSESIREN_CLIC
#include <Components/ClicPane.h>
#endif
#include "PluginProcessor.h"
#include <StandaloneAudio.h>
#include <TouchInput.h>
#if COMPOSESIREN_SIREN_WAVES
#include <juce_opengl/juce_opengl.h>
#include <Components/waves/SirenWaveColumn.h>
#endif

constexpr std::array<sirenId, 7> sirenOrder = { S7, S6, S5, S2, S1, S4, S3 };

#if COMPOSESIREN_PARK_BRIDGE
// LED D'ÉTAT ST (sirène physique, lue sur le variateur KEB) ===================

class StLedComponent : public juce::Component
{
public:
    StLedComponent()
    {
        // simple témoin visuel : ne doit pas gêner le clic sur le titre
        setInterceptsMouseClicks(false, false);
    }

    void setState(SirenUdpBridge::StState s)
    {
        if (s != state) {
            state = s;
            repaint();
        }
    }

    void paint(juce::Graphics& g) override
    {
        const auto area = getLocalBounds().toFloat().reduced(1.f);
        switch (state) {
            case SirenUdpBridge::StState::on:
                g.setColour(juce::Colours::limegreen);
                g.fillEllipse(area);
                break;
            case SirenUdpBridge::StState::off:
                g.setColour(juce::Colours::darkred);
                g.fillEllipse(area);
                break;
            case SirenUdpBridge::StState::unknown:
            default:
                // injoignable : juste un contour grisé
                g.setColour(juce::Colours::grey.withAlpha(0.6f));
                g.drawEllipse(area, 1.f);
                break;
        }
    }

private:
    SirenUdpBridge::StState state { SirenUdpBridge::StState::unknown };
};
#endif

// SIREN STRIP MENU (MIGHT BE MOVED TO ITS OWN FILE) ===========================

class SirenStripMenu : public SirenTrackComponent::Listener
{
    std::map<sirenId, std::unique_ptr<SirenTrackComponent>>& tracks;

public:
    class Listener
    {
    public:
        Listener() = default;
        virtual ~Listener() = default;
        virtual void sirenStripMenuItemSelected(std::optional<sirenId>) = 0;
    };

    SirenStripMenu(std::map<sirenId, std::unique_ptr<SirenTrackComponent>>& s)
    : tracks(s) {
        for (auto& [id, track] : tracks) {
            track->addListener(this);
        }
    }

    ~SirenStripMenu() override {
        for (auto& [id, track] : tracks) {
            track->removeListener(this);
        }
    }

    void setListener(Listener* l) { listener = l; }
    void removeListener() { listener = nullptr; }

    // SirenTrackComponent::Listener callback
    void sirenTrackClicked(sirenId sid) override {
        sirenTrackSelected(sid, true);
   }

    void setSelectedSirenTrack(std::optional<sirenId> sid, bool notify = true) {
        sirenTrackSelected(sid, notify);
    }

private:
    Listener* listener{nullptr};

    void sirenTrackSelected(std::optional<sirenId> sid, bool notify) const {
        if (!sid.has_value()) {
            if (notify) { listener->sirenStripMenuItemSelected(std::nullopt); }
            for (const auto& track : tracks | std::views::values) {
                track->setSelected(false);
            }
        } else {
            for (const auto& [id, track] : tracks) {
                if (id == sid.value()) {
                    if (notify) { listener->sirenStripMenuItemSelected(id); }
                    track->setSelected(true);

                    // use below code if we want to implement an unselected state :
                    // (fiddling with the MIDI keyboard looks tedious if we want to
                    // keep the mouse interaction but disable sending MIDI messages,
                    // alternative is to disable all mouse interaction if no track
                    // is selected)

                    // bool v = track->getSelected();
                    // track->setSelected(!v);
                    // if (!v) {
                    //     listener->sirenStripMenuItemSelected(id);
                    // } else {
                    //     listener->sirenStripMenuItemSelected(std::nullopt);
                    // }
                } else {
                    track->setSelected(false);
                }
            }
        }
    }
};

// In the top bar while the standalone has no audio device: no sound, but the MIDI in,
// the keyboard and the MIDI out keep working (StandaloneAudio.h).
class NoAudioNotice : public juce::Component, private juce::Timer
{
public:
    NoAudioNotice()
    {
        setInterceptsMouseClicks(false, false);
        startTimerHz(4);
    }

    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat().reduced(0.0f, 5.0f);
        g.setColour(juce::Colour(0xffe3a21a).withAlpha(0.18f));
        g.fillRoundedRectangle(area, 4.0f);
        g.setColour(juce::Colour(0xffe3a21a));
        g.drawRoundedRectangle(area, 4.0f, 1.0f);
        g.setColour(juce::Colours::whitesmoke);
        g.setFont(juce::FontOptions(12.0f));
        g.drawFittedText("No audio output: no sound. MIDI in, keyboard and MIDI out still work.",
                         area.reduced(8.0f, 0.0f).toNearestInt(),
                         juce::Justification::centredLeft, 1);
    }

private:
    void timerCallback() override
    {
        setVisible(cs::standalone::audioState() == cs::standalone::AudioState::noDevice);
    }
};

class SirenOrchestraPluginEditor : public juce::AudioProcessorEditor,
                                   public VoiceManagerState::Listener,
                                   public SirenStripMenu::Listener
#if COMPOSESIREN_PARK_BRIDGE
                                   , private juce::Timer
#endif
#if COMPOSESIREN_SIREN_WAVES
                                   , private SirenStateMonitor::Listener
                                   , private cs::Settings::Listener
#endif
{
public:
    SirenOrchestraPluginEditor(SirenOrchestraPluginProcessor&);
    ~SirenOrchestraPluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

#if COMPOSESIREN_PARK_BRIDGE
    // rafraîchit les LEDs d'état ST depuis le bridge UDP
    void timerCallback() override;
#endif

    // VoiceManagerState::Listener
    //--------------------------------------------------------------------------
    void midiInputChanged(AnyOrOneBasedMidiChannel inch) override;

    // SirenStripMenu::Listener
    //--------------------------------------------------------------------------
    void sirenStripMenuItemSelected(std::optional<sirenId> s) override;

private:
    // Declared first so it is destroyed last: resumes MCP only after every
    // other member — including WavesOverflow — is gone. The dtor body calls
    // beginEditorTeardown() at the top; this closes the window at the end.
    struct EndTeardown
    {
        SirenOrchestraPluginProcessor* processor = nullptr;
        ~EndTeardown()
        {
            if (processor != nullptr)
                processor->endEditorTeardown();
        }
    } endTeardown;

    SirenOrchestraPluginProcessor& audioProcessor;

    MainButtonsComponent mainButtons;
    NoAudioNotice noAudioNotice;
    // Standalone: runs the processor while no audio device does (null in a plugin).
    cs::standalone::Keeper audioKeeper;
    juce::TooltipWindow tooltipWindow { this };
    std::map<sirenId, std::unique_ptr<SirenTrackComponent>> sirenTracks;
#if COMPOSESIREN_PARK_BRIDGE
    std::map<sirenId, std::unique_ptr<StLedComponent>> stLeds;
#endif
    ReverbStripComponent rvbStrip;
#if COMPOSESIREN_CLIC
    ClicPane clicPane;
#endif
    MasterVolumeComponent masterVolume;
    DbRangesMidiKeyboardComponent midiKeyboard;

    // uses sirenTracks as menu items and provides callback
    SirenStripMenu sirenStripMenu;
    juce::Colour bottomColour;

#if COMPOSESIREN_SIREN_WAVES
    // The waves in the title cells (see SirenWaveColumn). The cells are
    // painted by paint(), beneath the tracks, which then draw the names, the
    // LEDs and the selection on top. Overflow paints what goes past the
    // cells, over the strips.
    class WavesOverflow : public juce::Component
    {
    public:
        explicit WavesOverflow(SirenOrchestraPluginEditor& e) : editor(e)
        {
            setInterceptsMouseClicks(false, false);
        }
        void paint(juce::Graphics& g) override;
        // Stop painting and let go of the editor before either is torn down.
        void silence();
    private:
        SirenOrchestraPluginEditor& editor;
        bool quiet = false;
    };

    // SirenStateMonitor::Listener
    void currentSirenState(const sirenId, const SirenVoice::State&) override;
    // cs::Settings::Listener
    void settingChanged(cs::Settings::Id) override;

    void updateWavesSetup();
    void updateWaveCells();
    void advanceWaves(double now);
    juce::Rectangle<int> waveColumnBounds() const;

    juce::SharedResourcePointer<cs::Settings> settings;
    cs::waves::Tuning waveTuning;
    cs::waves::SirenWaveColumn waveColumn;
    WavesOverflow wavesOverflow { *this };
    juce::OpenGLContext glContext;
    std::unique_ptr<juce::VBlankAttachment> vblank;
    double lastWaveFrame = 0.0;
    bool wavesMoving = false;

    // COMPOSESIREN_WAVES_STATS=1 in the environment: once a second on stderr,
    // the animation and paint rates, the cells' paint time, each siren's peak level
    struct WaveStats {
        bool on = false;
        int frames = 0, paints = 0;
        double paintMs = 0.0, paintMaxMs = 0.0, since = 0.0;
        std::map<sirenId, float> peakDb;
    } waveStats;
#endif

    // Last: stops reading touches before anything else goes.
    cs::TouchInput touchInput{*this};
};


#endif //SIRENORCHESTRA_PLUGINEDITOR_H