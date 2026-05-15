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

// constexpr std::array<sirenId, 7> sirenOrder = { S3, S4, S1, S2, S5, S6, S7 };
constexpr std::array<sirenId, 7> sirenOrder = { S7, S6, S5, S2, S1, S4, S3 };

// SIREN STRIP MENU (SHOULD BE MOVED TO ITS OWN FILE ===========================

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
                                   public SirenStripMenu::Listener
{
public:
    SirenOrchestraPluginEditor(SirenOrchestraPluginProcessor&);
    ~SirenOrchestraPluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

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
    ReverbStripComponent rvbStrip;
    MasterVolumeComponent masterVolume;
    DbRangesMidiKeyboardComponent midiKeyboard;

    // uses sirenTracks as menu items and provides callback
    SirenStripMenu sirenStripMenu;
    juce::Colour bottomColour;
};


#endif //SIRENORCHESTRA_PLUGINEDITOR_H