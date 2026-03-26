//
// Created by joseph larralde on 21/03/2026.
//

#ifndef COMPOSESIREN_RESETCOMPONENT_H
#define COMPOSESIREN_RESETCOMPONENT_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../lib/definitions/palette.h"

class MainButtonsComponent : public juce::Component,
                             public juce::TextButton::Listener
{
public:
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void resetSiren() = 0;
        virtual void selectedNewResourcesPath(const std::string&) = 0;
        virtual std::string getResourcesPath() = 0;
    };

    MainButtonsComponent(Listener& l) : listener(l)
    {

        resetButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred);
        resetButton.setColour(juce::TextButton::textColourOffId , juce::Colours::whitesmoke);
        resetButton.setButtonText ("Reset");
        resetButton.addListener(this);
        addAndMakeVisible(resetButton);

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
    }

    ~MainButtonsComponent() override = default;

    void paint(juce::Graphics& g) override
    {
        // g.setColour(juce::Colour{0xff314159});
        // g.fillRect(getLocalBounds().toFloat());
    }

    void resized() override
    {
        const int margin = 5;

        auto bounds = getLocalBounds();
        bounds.removeFromTop(margin);
        bounds.removeFromRight(margin);
        bounds.removeFromLeft(margin);

        juce::FlexBox fb;

        fb.flexDirection = juce::FlexBox::Direction::row;
        fb.flexWrap = juce::FlexBox::Wrap::noWrap;
        // fb.alignItems = juce::FlexBox::AlignItems::center;
        fb.alignItems = juce::FlexBox::AlignItems::flexEnd;
        fb.alignContent = juce::FlexBox::AlignContent::spaceBetween;

        const int menuHeight = bounds.getHeight();
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
                                                    .withMinHeight(menuHeight)
                                                    .withFlex(0,1);
        fb.items.add(item);
        item = juce::FlexItem(resetButton).withMinWidth(75)
                                          .withMinHeight(menuHeight)
                                          .withFlex(0,0);
        item.margin = juce::FlexItem::Margin(0.f, 0.f, 0.f, (float) margin);
        fb.items.add(item);
        fb.performLayout(bounds);
    }

    void buttonClicked(juce::Button* btn) override
    {
        if (btn == &resetButton) {
            listener.resetSiren();
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

            fileChooser->launchAsync(flags, [this](const juce::FileChooser& chooser) {
                // get the result to update resourcesPath
                juce::File newResourcesPath = chooser.getResult();
                listener.selectedNewResourcesPath(
                    newResourcesPath.getFullPathName().toStdString()
                );
            });
        }
    }

private:
    Listener& listener;

    juce::TextButton resetButton;
    juce::TextButton selectResourcesButton;

    std::unique_ptr<juce::FileChooser> fileChooser;
};


#endif //COMPOSESIREN_RESETCOMPONENT_H