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
#include "PluginProcessor.h"

constexpr std::array<sirenId, 7> sirenOrder = { S7, S6, S5, S2, S1, S4, S3 };

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

class SirenOrchestraPluginEditor : public juce::AudioProcessorEditor,
                                   public VoiceManagerState::Listener,
                                   public SirenStripMenu::Listener,
                                   private juce::Timer
{
public:
    SirenOrchestraPluginEditor(SirenOrchestraPluginProcessor&);
    ~SirenOrchestraPluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // rafraîchit les LEDs d'état ST depuis le bridge UDP
    void timerCallback() override;

    // VoiceManagerState::Listener
    //--------------------------------------------------------------------------
    void midiInputChanged(AnyOrOneBasedMidiChannel inch) override;

    // SirenStripMenu::Listener
    //--------------------------------------------------------------------------
    void sirenStripMenuItemSelected(std::optional<sirenId> s) override;

private:
    SirenOrchestraPluginProcessor& audioProcessor;

    MainButtonsComponent mainButtons;
    std::map<sirenId, std::unique_ptr<SirenTrackComponent>> sirenTracks;
    std::map<sirenId, std::unique_ptr<StLedComponent>> stLeds;
    ReverbStripComponent rvbStrip;
    MasterVolumeComponent masterVolume;
    DbRangesMidiKeyboardComponent midiKeyboard;

    // uses sirenTracks as menu items and provides callback
    SirenStripMenu sirenStripMenu;
    juce::Colour bottomColour;
};


#endif //SIRENORCHESTRA_PLUGINEDITOR_H