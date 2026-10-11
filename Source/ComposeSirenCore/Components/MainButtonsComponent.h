//
// Created by joseph larralde on 21/03/2026.
//

#ifndef COMPOSESIREN_RESETCOMPONENT_H
#define COMPOSESIREN_RESETCOMPONENT_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../lib/definitions/palette.h"
#include "../lib/definitions/sirenProperties.h"

#ifndef COMPOSESIREN_PARK_BRIDGE
#define COMPOSESIREN_PARK_BRIDGE 0
#endif
#ifndef COMPOSESIREN_SETTINGS
#define COMPOSESIREN_SETTINGS 0
#endif
#ifndef COMPOSESIREN_SONG_TITLE
#define COMPOSESIREN_SONG_TITLE 0
#endif
#include "../lib/net/mcp/McpControl.h"
#include "../lib/utilities/recorder/RecordDialog.h"
#if COMPOSESIREN_SETTINGS
#include "../lib/settings/SettingsDialog.h"
#endif

class MainButtonsComponent : public juce::Component,
                             public juce::TextButton::Listener
#if COMPOSESIREN_SONG_TITLE
                           , private juce::Timer
                           , private juce::ChangeListener
#endif
{
public:
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void resetSiren(std::optional<sirenId>) = 0;
        virtual void selectedNewResourcesPath(const std::string&) = 0;
        virtual std::string getResourcesPath() = 0;
        // optionnel : pilotage des sirènes physiques — no-op par défaut.
        // physicalSirensEnabled() donne l'état courant pour initialiser la case
        // quand l'éditeur s'ouvre (l'état vit dans le processor, pas dans l'UI).
        virtual void physicalSirensSwitched(bool) {}
        virtual bool physicalSirensEnabled() { return false; }
        virtual juce::String physicalSirensTooltip() { return {}; }
        virtual void stAllSwitched(bool) {}
        // optionnel : un bouton About... ouvre la fenêtre que showAbout() ouvre.
        virtual bool hasAbout() { return false; }
        virtual void showAbout(juce::Component* /*parent*/) {}
        // le serveur MCP (About, and the song title bar when COMPOSESIREN_SONG_TITLE)
        virtual McpControl& getMcp() = 0;
#if COMPOSESIREN_RECORD
        // optionnel : l'enregistreur de la sortie audio ; un bouton Record...
        // ouvre son dialogue quand il y en a un.
        virtual Recorder* getRecorder() { return nullptr; }
#endif
    };

    MainButtonsComponent(Listener& l, bool hasResetAll = false,
                         bool hasStAll = false) :
        listener(l),
        hasResetAllButton(hasResetAll)
#if COMPOSESIREN_PARK_BRIDGE
        , hasStAllSwitch(hasStAll)
#endif
    {
#if !COMPOSESIREN_PARK_BRIDGE
        juce::ignoreUnused(hasStAll);
#endif
        // Secondary actions live in one hamburger so the 30px top bar stays readable
        // when Settings, Record, About and Reset All are all present.
        menuButton.setTooltip("Menu");
        menuButton.onClick = [this] { showMenu(); };
        addAndMakeVisible(menuButton);

        resetButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred);
        resetButton.setColour(juce::TextButton::textColourOffId , juce::Colours::whitesmoke);
        resetButton.setButtonText ("Reset");
        resetButton.addListener(this);
        addAndMakeVisible(resetButton);

        if (hasResetAllButton) {
            resetAllButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred);
            resetAllButton.setColour(juce::TextButton::textColourOffId , juce::Colours::whitesmoke);
            resetAllButton.setButtonText ("Reset All");
            resetAllButton.addListener(this);
            addAndMakeVisible(resetAllButton);
        }

#if COMPOSESIREN_PARK_BRIDGE
        if (hasStAllSwitch) {
            physicalButton.setButtonText("Sirenes physiques");
            physicalButton.setColour(juce::ToggleButton::textColourId, juce::Colours::whitesmoke);
            physicalButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::whitesmoke);
            physicalButton.setToggleState(listener.physicalSirensEnabled(),
                                          juce::dontSendNotification);
            physicalButton.setTooltip(listener.physicalSirensTooltip());
            physicalButton.addListener(this);
            addAndMakeVisible(physicalButton);

            stAllButton.setButtonText("ST");
            stAllButton.setEnabled(physicalButton.getToggleState());
            stAllButton.setColour(juce::ToggleButton::textColourId, juce::Colours::whitesmoke);
            stAllButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::whitesmoke);
            stAllButton.addListener(this);
            addAndMakeVisible(stAllButton);
        }
#endif

#if COMPOSESIREN_SONG_TITLE
        // The song tap-viewer is playing (set_song_title), on the left of the
        // top row and in the window's title bar. Hidden again by clear_song_title.
        songTitle.setJustificationType(juce::Justification::centredLeft);
        songTitle.setColour(juce::Label::textColourId, juce::Colours::whitesmoke);
        songTitle.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        songTitle.setInterceptsMouseClicks(false, false);
        addChildComponent(songTitle);

        auto& mcp = listener.getMcp();
        mcp.addChangeListener(this);
        applySongDisplay(mcp.getSongDisplay(), false);
#endif
    }

    ~MainButtonsComponent() override
    {
#if COMPOSESIREN_SONG_TITLE
        listener.getMcp().removeChangeListener(this);
#endif
    }

    void paint(juce::Graphics& g) override
    {
        juce::ignoreUnused(g);
    }

    void resized() override
    {
        constexpr float margin = 5;
        auto bounds = getLocalBounds().reduced(static_cast<int>(margin));

        juce::FlexBox fb;
        fb.flexDirection = juce::FlexBox::Direction::row;
        fb.flexWrap = juce::FlexBox::Wrap::noWrap;
        fb.alignItems = juce::FlexBox::AlignItems::flexEnd;
        fb.alignContent = juce::FlexBox::AlignContent::flexEnd;
        fb.justifyContent = juce::FlexBox::JustifyContent::flexEnd;

        const float btnsHeight = static_cast<float>(bounds.getHeight());

#if COMPOSESIREN_SONG_TITLE
        // The song title, when one is playing.
        if (songTitle.isVisible())
            songTitle.setBounds(bounds.removeFromLeft(
                static_cast<int>(juce::jmax(160.0f, bounds.getWidth() * 0.42f))));
#endif

        auto add = [&](juce::Button& b, float minW) {
            juce::FlexItem item = juce::FlexItem(b).withMinWidth(minW)
                                                   .withMinHeight(btnsHeight)
                                                   .withFlex(0, 0);
            item.margin = juce::FlexItem::Margin(0.f, 0.f, 0.f, margin);
            fb.items.add(item);
        };

#if COMPOSESIREN_PARK_BRIDGE
        if (hasStAllSwitch) {
            add(physicalButton, 130.f);
            add(stAllButton, 44.f);
        }
#endif
        if (hasResetAllButton)
            add(resetAllButton, 88.f);
        add(resetButton, 70.f);
        add(menuButton, btnsHeight + 10.f);

        fb.performLayout(bounds);
    }

    void setSirenIdToReset(std::optional<sirenId> id)
    {
        currentSirenId = id;
    }

#if COMPOSESIREN_SONG_TITLE
private:
    // The song display: title on the left of the top row, and progress in the
    // window's title bar (`set_song_progress`, and the clock between calls).
    // `clear_song_title` puts both back.
    void changeListenerCallback(juce::ChangeBroadcaster*) override
    {
        applySongDisplay(listener.getMcp().getSongDisplay(), true);
    }

    void timerCallback() override
    {
        if (!song.active)
            return;
        updateWindowTitle(formatSongLine(song));
    }

    void applySongDisplay(const SongDisplay& next, bool relayout)
    {
        const bool wasActive = song.active;
        song = next;
        songTitle.setVisible(song.active);
        updateSongTitle();

        if (song.active != wasActive || relayout)
            resized();

        if (song.active)
            startTimerHz(4);
        else
            stopTimer();
    }

    // Progress is only in the window title: Title [=======|-------] 1:23
    static juce::String formatSongLine(const SongDisplay& song)
    {
        if (!song.active || song.title.isEmpty())
            return {};

        juce::String line = song.title;
        if (song.durationSeconds > 0.0) {
            constexpr int cells = 14;
            const double fraction =
                juce::jlimit(0.0, 1.0, song.currentPosition() / song.durationSeconds);
            const int filled = juce::roundToInt(fraction * cells);

            line << "  [";
            for (int i = 0; i < cells; ++i) {
                if (i < filled)
                    line << '=';
                else if (i == filled)
                    line << '|';
                else
                    line << '-';
            }
            line << "]";

            const int total = juce::roundToInt(song.durationSeconds);
            const int at = juce::roundToInt(juce::jmin(song.currentPosition(),
                                                       song.durationSeconds));
            line << "  " << (at / 60) << ":"
                 << juce::String(at % 60).paddedLeft('0', 2) << "/"
                 << (total / 60) << ":"
                 << juce::String(total % 60).paddedLeft('0', 2);
        }
        return line;
    }

    void updateSongTitle()
    {
        const auto line = song.active ? song.title : juce::String();
        songTitle.setText(line, juce::dontSendNotification);
        updateWindowTitle(formatSongLine(song));
    }

    void updateWindowTitle(const juce::String& line)
    {
        auto* top = getTopLevelComponent();
        if (top == nullptr)
            return;
        if (defaultWindowTitle.isEmpty())
            defaultWindowTitle = top->getName();
        top->setName(song.active && line.isNotEmpty() ? line : defaultWindowTitle);
    }

public:
#endif // COMPOSESIREN_SONG_TITLE

#if COMPOSESIREN_PARK_BRIDGE
    void refreshPhysicalSirensTooltip()
    {
        if (hasStAllSwitch)
            physicalButton.setTooltip(listener.physicalSirensTooltip());
    }
#endif

    void buttonClicked(juce::Button* btn) override
    {
        if (btn == &resetButton) {
            listener.resetSiren(currentSirenId);
            return;
        }

#if COMPOSESIREN_PARK_BRIDGE
        if (btn == &physicalButton) {
            const bool on = physicalButton.getToggleState();
            stAllButton.setEnabled(on);
            listener.physicalSirensSwitched(on);
            refreshPhysicalSirensTooltip();
            return;
        }

        if (btn == &stAllButton) {
            listener.stAllSwitched(stAllButton.getToggleState());
            return;
        }
#endif

        if (btn == &resetAllButton) {
            listener.resetSiren(std::nullopt);
            return;
        }
    }

    void showMenu()
    {
        juce::PopupMenu menu;
#if COMPOSESIREN_SETTINGS
        menu.addItem("Settings...", [this] { cs::SettingsDialog::show(getTopLevelComponent()); });
#endif
#if COMPOSESIREN_RECORD
        if (auto* recorder = listener.getRecorder())
            menu.addItem("Record...", [this, recorder] { RecordDialog::show(*recorder, getTopLevelComponent()); });
#endif
#if COMPOSESIREN_DEV_BUILD
        menu.addItem("Set resources directory...", [this] { chooseResourcesDirectory(); });
#endif
        if (listener.hasAbout()) {
            menu.addSeparator();
            menu.addItem("About...", [this] { listener.showAbout(getTopLevelComponent()); });
        }
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&menuButton));
    }

    void chooseResourcesDirectory()
    {
        const std::string resourcesPath = listener.getResourcesPath();
        fileChooser = std::make_unique<juce::FileChooser>(
            "Select a file", juce::File(resourcesPath), ""
        );

        auto flags =
            juce::FileBrowserComponent::openMode
            | juce::FileBrowserComponent::canSelectDirectories;

        fileChooser->launchAsync(flags,[this](const juce::FileChooser& chooser) {
            juce::File newResourcesPath = chooser.getResult();
            listener.selectedNewResourcesPath(
                juce::File::addTrailingSeparator(
                    newResourcesPath.getFullPathName()
                ).toStdString()
            );
        });
    }

private:
    class HamburgerButton : public juce::Button
    {
    public:
        HamburgerButton() : juce::Button("Menu") {}
        void paintButton(juce::Graphics& g, bool highlighted, bool down) override
        {
            auto area = getLocalBounds().toFloat().reduced(0.5f);
            g.setColour(juce::Colour { 0xff37474f }.brighter(down ? 0.3f : highlighted ? 0.15f : 0.0f));
            g.fillRoundedRectangle(area, 4.0f);
            g.setColour(juce::Colours::whitesmoke);
            const float w = juce::jmin(16.0f, area.getWidth() - 10.0f);
            const float gap = juce::jmin(5.0f, area.getHeight() / 5.0f);
            const auto c = area.getCentre();
            for (int i = -1; i <= 1; ++i)
                g.fillRoundedRectangle(c.x - w / 2, c.y + float(i) * gap - 1.0f, w, 2.0f, 1.0f);
        }
    };

    Listener& listener;

    std::optional<sirenId> currentSirenId{std::nullopt};
    bool hasResetAllButton{false};
#if COMPOSESIREN_PARK_BRIDGE
    bool hasStAllSwitch{false};
#endif

    HamburgerButton menuButton;
    juce::TextButton resetButton;
    juce::TextButton resetAllButton;
#if COMPOSESIREN_PARK_BRIDGE
    juce::ToggleButton physicalButton;
    juce::ToggleButton stAllButton;
#endif

#if COMPOSESIREN_SONG_TITLE
    // the song title in the top bar and window title (set_song_title / clear_song_title)
    SongDisplay song;
    juce::String defaultWindowTitle;
    juce::Label songTitle;
#endif

    std::unique_ptr<juce::FileChooser> fileChooser;
};

#endif //COMPOSESIREN_RESETCOMPONENT_H
