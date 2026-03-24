//
// Created by joseph larralde on 08/03/2026.
//

#ifndef COMPOSESIREN_VOICEMANAGERCOMPONENT_H
#define COMPOSESIREN_VOICEMANAGERCOMPONENT_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "LookAndFeels.h"
#include "lib/definitions/palette.h"
#include "lib/definitions/sirenProperties.h"
#include "VoiceManagerState.h"

class LabelLAF : public juce::LookAndFeel_V2
{
public:
    LabelLAF()
    {
        setColour(juce::Label::textColourId, juce::Colours::whitesmoke);
        setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    }

    juce::Font getLabelFont(juce::Label &) override
    {
        return juce::FontOptions{12.0f, juce::Font::plain};
    }

    ~LabelLAF() override = default;
};


class VoiceManagerComponent : public juce::Component,
                              public juce::ComboBox::Listener
{
    juce::ComboBox category;
    juce::Label categoryLabel;
    juce::ComboBox inputChannel;
    juce::Label inputChannelLabel;
    juce::ComboBox outputChannel;
    juce::Label outputChannelLabel;

    std::vector<juce::ComboBox*> menus{
        &category, &inputChannel, &outputChannel
    };

    LabelLAF labelLAF;
    ComboBoxLookAndFeel comboLAF{
        juce::Colour{0xff283541}.withAlpha(0.95f),
        juce::Colours::whitesmoke
    };

public:
    VoiceManagerComponent(VoiceManagerState& s) :
        voiceManagerState(s)
    {
        // categories ----------------------------------------------------------
        category.addItem("Bass", 1);
        category.addItem("Tenor", 2);
        category.addItem("Alto", 3);
        category.addItem("Soprano", 4);
        category.addItem("Piccolo", 5);
        category.setSelectedId(3);

        categoryLabel.setText("Siren type", juce::dontSendNotification);
        categoryLabel.setJustificationType(juce::Justification::left);
        categoryLabel.setLookAndFeel(&labelLAF);
        addAndMakeVisible(&categoryLabel);

        // input channel -------------------------------------------------------
        inputChannel.addItem("Any", 8);
        for (int i = 1; i < 8; ++i) {
            inputChannel.addItem(juce::String(i), i);
        }
        inputChannel.setSelectedId(8);

        inputChannelLabel.setText("MIDI In", juce::dontSendNotification);
        // inputChannelLabel.setJustificationType(juce::Justification::right);
        inputChannelLabel.setJustificationType(juce::Justification::left);
        inputChannelLabel.setLookAndFeel(&labelLAF);
        addAndMakeVisible(&inputChannelLabel);

        // output channel ------------------------------------------------------
        outputChannel.addItem("Thru", 8);
        for (int i = 1; i < 8; ++i) {
            outputChannel.addItem(juce::String(i), i);
        }
        outputChannel.setSelectedId(8);

        outputChannelLabel.setText("MIDI Out", juce::dontSendNotification);
        // outputChannelLabel.setJustificationType(juce::Justification::right);
        outputChannelLabel.setJustificationType(juce::Justification::left);
        outputChannelLabel.setLookAndFeel(&labelLAF);
        addAndMakeVisible(&outputChannelLabel);

        for (auto* menu : menus) {
            menu->setLookAndFeel(&comboLAF);
            menu->addListener(this);
            addAndMakeVisible(menu);
        }
    }

    ~VoiceManagerComponent() override = default;

    void paint(juce::Graphics& g) override {
        // g.setColour(juce::Colour{0xff314159});
        // g.fillRect(getLocalBounds().toFloat());
    }

    void resized() override {
        const int margin = 5;

        auto bounds = getLocalBounds().reduced(margin);

        juce::FlexBox fb;

        fb.flexDirection = juce::FlexBox::Direction::row;
        fb.flexWrap = juce::FlexBox::Wrap::noWrap;
        // fb.alignItems = juce::FlexBox::AlignItems::center;
        fb.alignItems = juce::FlexBox::AlignItems::flexStart;
        fb.alignContent = juce::FlexBox::AlignContent::spaceBetween;

        const int menuHeight = bounds.getHeight();
        juce::FlexItem item;

        // // Left menus //////////////////////////////////////////////////////////
        // fb.alignContent = juce::FlexBox::AlignContent::flexStart;
        // fb.justifyContent = juce::FlexBox::JustifyContent::flexStart;
        // item = juce::FlexItem(category).withMinWidth(75)
        //                                .withMinHeight(menuHeight)
        //                                .withFlex(0,0);
        // fb.items.add(item);
        // item = juce::FlexItem(categoryLabel).withMinWidth(80)
        //                                     .withMinHeight(menuHeight)
        //                                     .withFlex(0,0);
        // item.margin = juce::FlexItem::Margin(0.f, 0.f, 0.f, (float) margin);
        // fb.items.add(item);
        //
        // fb.performLayout(bounds);
        // fb.items.clear();

        // Right menus /////////////////////////////////////////////////////////
        // fb.alignContent = juce::FlexBox::AlignContent::flexEnd;
        // fb.justifyContent = juce::FlexBox::JustifyContent::flexEnd;
        //
        // item = juce::FlexItem(inputChannelLabel).withMinWidth(60)
        //                        .withMinHeight(menuHeight)
        //                        .withFlex(0,1);
        // fb.items.add(item);
        // item = juce::FlexItem(inputChannel).withMinWidth(55)
        //                                     .withMinHeight(menuHeight)
        //                                     .withFlex(0,0);
        // item.margin = juce::FlexItem::Margin(0.f, 0.f, 0.f, (float) margin);
        // fb.items.add(item);
        // item = juce::FlexItem(outputChannelLabel).withMinWidth(60)
        //                        .withMinHeight(menuHeight)
        //                        .withFlex(0,1);
        // fb.items.add(item);
        // item = juce::FlexItem(outputChannel).withMinWidth(55)
        //                                     .withMinHeight(menuHeight)
        //                                     .withFlex(0,0);
        // item.margin = juce::FlexItem::Margin(0.f, 0.f, 0.f, (float) margin);
        // fb.items.add(item);

        // All left /////////////////////////////////////////////////////////
        fb.alignContent = juce::FlexBox::AlignContent::flexStart;
        fb.justifyContent = juce::FlexBox::JustifyContent::flexStart;

        item = juce::FlexItem(category).withMinWidth(75)
                                       .withMinHeight(menuHeight)
                                       .withFlex(0,0);
        fb.items.add(item);
        item = juce::FlexItem(categoryLabel).withMinWidth(60)
                                            .withMinHeight(menuHeight)
                                            .withFlex(0,1);
        item.margin = juce::FlexItem::Margin(0.f, 0.f, 0.f, (float) margin);
        fb.items.add(item);
        item = juce::FlexItem(inputChannel).withMinWidth(55)
                                            .withMinHeight(menuHeight)
                                            .withFlex(0,0);
        item.margin = juce::FlexItem::Margin(0.f, 0.f, 0.f, (float) margin);
        fb.items.add(item);
        item = juce::FlexItem(inputChannelLabel).withMinWidth(50)
                                                .withMinHeight(menuHeight)
                                                .withFlex(0,1);
        item.margin = juce::FlexItem::Margin(0.f, 0.f, 0.f, (float) margin);
        fb.items.add(item);
        item = juce::FlexItem(outputChannel).withMinWidth(55)
                                            .withMinHeight(menuHeight)
                                            .withFlex(0,0);
        item.margin = juce::FlexItem::Margin(0.f, 0.f, 0.f, (float) margin);
        fb.items.add(item);
        item = juce::FlexItem(outputChannelLabel).withMinWidth(50)
                               .withMinHeight(menuHeight)
                               .withFlex(0,1);
        item.margin = juce::FlexItem::Margin(0.f, 0.f, 0.f, (float) margin);
        fb.items.add(item);

        fb.performLayout(bounds);
    }

private:
    VoiceManagerState& voiceManagerState;

    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override {
        if (comboBoxThatHasChanged == &category) {
            switch (category.getSelectedId()) {
            case 1:
                voiceManagerState.setSirenCategory(sirenCategory::Bass);
                break;
            case 2:
                voiceManagerState.setSirenCategory(sirenCategory::Tenor);
                break;
            case 3:
                voiceManagerState.setSirenCategory(sirenCategory::Alto);
                break;
            case 4:
                voiceManagerState.setSirenCategory(sirenCategory::Soprano);
                break;
            case 5:
                voiceManagerState.setSirenCategory(sirenCategory::Piccolo);
                break;
            default:
                break;
            }
        } else if (comboBoxThatHasChanged == &inputChannel) {
            auto i = inputChannel.getSelectedId();
            if (i == 8) {
                voiceManagerState.setMidiInput(AnyMidiChannel{});
            } else if (i > 0 && i < 8) {
                voiceManagerState.setMidiInput(OneBasedMidiChannel{.oneBased=i});
            }
        } else if (comboBoxThatHasChanged == &outputChannel) {
            auto i = outputChannel.getSelectedId();
            if (i == 8) {
                voiceManagerState.setMidiOutput(AnyMidiChannel{});
            } else if (i > 0 && i < 8) {
                voiceManagerState.setMidiOutput(OneBasedMidiChannel{.oneBased=i});
            }
        }
    }
};


#endif //COMPOSESIREN_VOICEMANAGERCOMPONENT_H