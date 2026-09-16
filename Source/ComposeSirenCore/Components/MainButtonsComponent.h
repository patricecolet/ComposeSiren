//
// Created by joseph larralde on 21/03/2026.
//

#ifndef COMPOSESIREN_RESETCOMPONENT_H
#define COMPOSESIREN_RESETCOMPONENT_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../lib/definitions/palette.h"
#include "../lib/definitions/sirenProperties.h"

class MainButtonsComponent : public juce::Component,
                             public juce::TextButton::Listener
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
        virtual void stAllSwitched(bool) {}
    };

    MainButtonsComponent(Listener& l, bool hasResetAll = false,
                         bool hasStAll = false) :
        listener(l),
        hasResetAllButton(hasResetAll),
        hasStAllSwitch(hasStAll)
    {
        selectResourcesButton.setColour(
            juce::TextButton::buttonColourId,
            juce::Colour{mecaviv::Colours::darkTransparentBackground}
        );
        selectResourcesButton.setColour(juce::TextButton::textColourOffId , juce::Colours::whitesmoke);
        selectResourcesButton.setButtonText("Set resources directory");
        selectResourcesButton.addListener(this);
#if COMPOSESIREN_DEV_BUILD
        addAndMakeVisible(selectResourcesButton);
#endif

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

        if (hasStAllSwitch) {
            physicalButton.setButtonText("Sirenes physiques");
            physicalButton.setColour(juce::ToggleButton::textColourId, juce::Colours::whitesmoke);
            physicalButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::whitesmoke);
            physicalButton.setToggleState(listener.physicalSirensEnabled(),
                                          juce::dontSendNotification);
            physicalButton.addListener(this);
            addAndMakeVisible(physicalButton);

            stAllButton.setButtonText("ST");
            stAllButton.setEnabled(physicalButton.getToggleState());
            stAllButton.setColour(juce::ToggleButton::textColourId, juce::Colours::whitesmoke);
            stAllButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::whitesmoke);
            stAllButton.addListener(this);
            addAndMakeVisible(stAllButton);
        }
    }

    ~MainButtonsComponent() override = default;

    void paint(juce::Graphics& g) override
    {
        // g.setColour(juce::Colour{0xff314159});
        // g.fillRect(getLocalBounds().toFloat());
    }

    void resized() override
    {
        constexpr float margin = 5;
        auto bounds = getLocalBounds().reduced(static_cast<int>(margin));

        juce::FlexBox fb;

        fb.flexDirection = juce::FlexBox::Direction::row;
        fb.flexWrap = juce::FlexBox::Wrap::noWrap;
        // fb.alignItems = juce::FlexBox::AlignItems::center;
        fb.alignItems = juce::FlexBox::AlignItems::flexEnd;
        fb.alignContent = juce::FlexBox::AlignContent::spaceBetween;

        const float btnsHeight = static_cast<float>(bounds.getHeight());
        juce::FlexItem item;

        // Left button /////////////////////////////////////////////////////////
        // fb.alignContent = juce::FlexBox::AlignContent::flexStart;
        // fb.justifyContent = juce::FlexBox::JustifyContent::flexStart;
        //
        // item = juce::FlexItem(resetButton).withMinWidth(75)
        //                                   .withMinHeight(menuHeight)
        //                                   .withFlex(0,0);
        // fb.items.add(item);
        // fb.performLayout(bounds);
        // fb.items.clear();

        // Right button ////////////////////////////////////////////////////////
        // fb.alignContent = juce::FlexBox::AlignContent::flexEnd;
        // fb.justifyContent = juce::FlexBox::JustifyContent::flexEnd;
        //
        // item = juce::FlexItem(selectResourcesButton).withMinWidth(230)
        //                                             .withMinHeight(menuHeight)
        //                                             .withFlex(0,1);
        // fb.items.add(item);
        // fb.performLayout(bounds);

        // All buttons right ///////////////////////////////////////////////////
        fb.alignContent = juce::FlexBox::AlignContent::flexEnd;
        fb.justifyContent = juce::FlexBox::JustifyContent::flexEnd;

        item = juce::FlexItem(selectResourcesButton).withMinWidth(230)
                                                    .withMinHeight(btnsHeight)
                                                    .withFlex(0,1);
        fb.items.add(item);
        item = juce::FlexItem(resetButton).withMinWidth(75)
                                          .withMinHeight(btnsHeight)
                                          .withFlex(0,0);
        item.margin = juce::FlexItem::Margin(0.f, 0.f, 0.f, margin);
        fb.items.add(item);

        if (hasResetAllButton) {
            item = juce::FlexItem(resetAllButton).withMinWidth(150)
                                                 .withMinHeight(btnsHeight)
                                                 .withFlex(0,0);
            item.margin = juce::FlexItem::Margin(0.f, 0.f, 0.f, margin);
            fb.items.add(item);
        }

        if (hasStAllSwitch) {
            item = juce::FlexItem(physicalButton).withMinWidth(150)
                                                 .withMinHeight(btnsHeight)
                                                 .withFlex(0,0);
            item.margin = juce::FlexItem::Margin(0.f, 0.f, 0.f, margin);
            fb.items.add(item);
            item = juce::FlexItem(stAllButton).withMinWidth(55)
                                              .withMinHeight(btnsHeight)
                                              .withFlex(0,0);
            item.margin = juce::FlexItem::Margin(0.f, 0.f, 0.f, margin);
            fb.items.add(item);
        }

        fb.performLayout(bounds);
    }

    void setSirenIdToReset(std::optional<sirenId> id)
    {
        currentSirenId = id;
    }

    void buttonClicked(juce::Button* btn) override
    {
        if (btn == &resetButton) {
            listener.resetSiren(currentSirenId);
            return;
        }

        if (btn == &physicalButton) {
            const bool on = physicalButton.getToggleState();
            stAllButton.setEnabled(on);
            listener.physicalSirensSwitched(on);
            return;
        }

        if (btn == &stAllButton) {
            listener.stAllSwitched(stAllButton.getToggleState());
            return;
        }

        if (btn == &resetAllButton) {
            listener.resetSiren(std::nullopt);
            return;
        }

        if (btn == &selectResourcesButton) {
            const std::string resourcesPath = listener.getResourcesPath();
            fileChooser = std::make_unique<juce::FileChooser>(
                "Select a file", juce::File(resourcesPath), ""
            );

            auto flags =
                juce::FileBrowserComponent::openMode
                | juce::FileBrowserComponent::canSelectDirectories;

            fileChooser->launchAsync(flags,[this](const juce::FileChooser& chooser) {
                // get the result to update resourcesPath
                juce::File newResourcesPath = chooser.getResult();
                listener.selectedNewResourcesPath(
                    juce::File::addTrailingSeparator(
                        newResourcesPath.getFullPathName()
                    ).toStdString()
                );
            });
        }
    }

private:
    Listener& listener;

    std::optional<sirenId> currentSirenId{std::nullopt};
    bool hasResetAllButton{false};
    bool hasStAllSwitch{false};

    juce::TextButton resetButton;
    juce::TextButton resetAllButton;
    juce::TextButton selectResourcesButton;
    juce::ToggleButton physicalButton;
    juce::ToggleButton stAllButton;

    std::unique_ptr<juce::FileChooser> fileChooser;
};

#endif //COMPOSESIREN_RESETCOMPONENT_H
