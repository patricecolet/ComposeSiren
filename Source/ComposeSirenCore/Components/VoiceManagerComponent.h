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

    const std::map<int, sirenCategory> categoryByIndex{
        { 1, sirenCategory::Bass    },
        { 2, sirenCategory::Tenor   },
        { 3, sirenCategory::Alto    },
        { 4, sirenCategory::Soprano },
        { 5, sirenCategory::Piccolo }
    };

    const std::map<sirenCategory, int> indexByCategory = [this]()
    {
        std::map<sirenCategory, int> res;
        for (auto& p : categoryByIndex) {
            res[p.second] = p.first;
        }
        return res;
    }();

    const std::map<int, AnyOrOneBasedMidiChannel> channelByIndex{
        { 1, OneBasedMidiChannel{.oneBased = 1} },
        { 2, OneBasedMidiChannel{.oneBased = 2} },
        { 3, OneBasedMidiChannel{.oneBased = 3} },
        { 4, OneBasedMidiChannel{.oneBased = 4} },
        { 5, OneBasedMidiChannel{.oneBased = 5} },
        { 6, OneBasedMidiChannel{.oneBased = 6} },
        { 7, OneBasedMidiChannel{.oneBased = 7} },
        { 8, AnyMidiChannel{}                   }
    };

    const std::map<AnyOrOneBasedMidiChannel, int> indexByChannel = [this]()
    {
        std::map<AnyOrOneBasedMidiChannel, int> res;
        for (auto& p : channelByIndex) {
            if (auto* c = std::get_if<OneBasedMidiChannel>(&p.second)) {
                res[OneBasedMidiChannel{c->oneBased}] = p.first;
            } else {
                res[AnyMidiChannel{}] = p.first;
            }
        }
        return res;
    }();

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
        for (auto& p : categoryByIndex) {
            category.addItem(sirenCategoriesData.at(p.second).name, p.first);
        }

        categoryLabel.setText("Siren type", juce::dontSendNotification);
        categoryLabel.setJustificationType(juce::Justification::left);
        categoryLabel.setLookAndFeel(&labelLAF);
        addAndMakeVisible(&categoryLabel);

        // input channel -------------------------------------------------------
        inputChannel.addItem("Any", 8);
        for (int i = 1; i < 8; ++i) {
            inputChannel.addItem(juce::String(i), i);
        }
        // inputChannel.setSelectedId(8);

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
        // outputChannel.setSelectedId(8);

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

        category.setSelectedId(
            indexByCategory.at(voiceManagerState.getSirenCategory()),
            juce::dontSendNotification
        );

        AnyOrOneBasedMidiChannel inChannel = voiceManagerState.getMidiInput();
        if (auto* ic = std::get_if<OneBasedMidiChannel>(&inChannel)) {
            inputChannel.setSelectedId(
                ic->oneBased,
                juce::dontSendNotification
            );
        } else {
            inputChannel.setSelectedId(8);
        }

        AnyOrOneBasedMidiChannel outChannel = voiceManagerState.getMidiOutput();
        if (auto* oc = std::get_if<OneBasedMidiChannel>(&outChannel)) {
            outputChannel.setSelectedId(
                oc->oneBased,
                juce::dontSendNotification
            );
        } else {
            outputChannel.setSelectedId(8);
        }
    }

    ~VoiceManagerComponent() override
    {
        voiceManagerState.removeListener(this);
        for (auto* menu : menus) {
            menu->setLookAndFeel(nullptr);
            menu->removeListener(this);
        }
        inputChannelLabel.setLookAndFeel(nullptr);
        outputChannelLabel.setLookAndFeel(nullptr);
        categoryLabel.setLookAndFeel(nullptr);
    }

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
            auto c = categoryByIndex.find(category.getSelectedId());
            if (c != categoryByIndex.end()) {
                voiceManagerState.setSirenCategory(c->second);
            }
        } else if (comboBoxThatHasChanged == &inputChannel) {
            auto i = channelByIndex.find(inputChannel.getSelectedId());
            if (i != channelByIndex.end()) {
                voiceManagerState.setMidiInput(i->second);
            }
        } else if (comboBoxThatHasChanged == &outputChannel) {
            auto o = channelByIndex.find(outputChannel.getSelectedId());
            if (o != channelByIndex.end()) {
                voiceManagerState.setMidiOutput(o->second);
            }
        }
    }
};


#endif //COMPOSESIREN_VOICEMANAGERCOMPONENT_H