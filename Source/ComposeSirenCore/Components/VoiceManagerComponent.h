//
// Created by joseph larralde on 08/03/2026.
//

#ifndef COMPOSESIREN_VOICEMANAGERCOMPONENT_H
#define COMPOSESIREN_VOICEMANAGERCOMPONENT_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "LookAndFeels.h"
#include "../lib/definitions/palette.h"
#include "../lib/definitions/sirenProperties.h"
#include "../uiParameterUtilities.h"
#include "../atomicUtilities.h"
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
                              public juce::ComboBox::Listener,
                              public VoiceManagerState::Listener
{
    juce::ComboBox category;
    juce::Label categoryLabel;
    juce::ComboBox inputChannel;
    juce::Label inputChannelLabel;
    juce::ComboBox outputChannel;
    juce::Label outputChannelLabel;

    // for use with ScopedGuards to avoid update echo
    // from menus as we are a VoiceManagerState::Listener
    // (probably overkill but makes no harm,
    // can be safely removed if we don't care about
    // executing callbacks twice or if (when) we prove we
    // don't need to be a VoiceManagerState::Listener)
    std::atomic<bool> updatingCategoryFromMenu;
    std::atomic<bool> updatingInputChannelFromMenu;
    std::atomic<bool> updatingOutputChannelFromMenu;

    std::vector<juce::ComboBox*> menus{
        &category, &inputChannel, &outputChannel
    };

    LabelLAF labelLAF;
    ComboBoxLookAndFeel comboLAF{
        juce::Colour{mecaviv::Colours::darkTransparentBackground},
        juce::Colours::whitesmoke
    };

public:
    VoiceManagerComponent(VoiceManagerState& s) :
        voiceManagerState(s)
    {
        voiceManagerState.addListener(this);
        updatingCategoryFromMenu.store(false);
        updatingInputChannelFromMenu.store(false);
        updatingOutputChannelFromMenu.store(false);

        // categories ----------------------------------------------------------
        for (auto& p : sirenCategoryByMenuIndex) {
            category.addItem(sirenCategoriesData.at(p.second).name, p.first);
        }

        categoryLabel.setText("Siren type", juce::dontSendNotification);
        categoryLabel.setJustificationType(juce::Justification::left);
        categoryLabel.setLookAndFeel(&labelLAF);
        addAndMakeVisible(&categoryLabel);

        // input channel -------------------------------------------------------
        for (auto& p : menuIndexMidiChannelPairs) {
            if (p.second.isAny) {
                inputChannel.addItem("Any", p.first);
            } else {
                inputChannel.addItem(
                    juce::String(p.second.channel.oneBased),
                    p.first
                );
            }
        }

        inputChannelLabel.setText("MIDI In", juce::dontSendNotification);
        // inputChannelLabel.setJustificationType(juce::Justification::right);
        inputChannelLabel.setJustificationType(juce::Justification::left);
        inputChannelLabel.setLookAndFeel(&labelLAF);
        addAndMakeVisible(&inputChannelLabel);

        // output channel ------------------------------------------------------
        for (auto& p : menuIndexMidiChannelPairs) {
            if (p.second.isAny) {
                outputChannel.addItem("Thru", p.first);
            } else {
                outputChannel.addItem(
                    juce::String(p.second.channel.oneBased),
                    p.first
                );
            }
        }

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

        // init menus from state :
        VoiceManagerComponent::categoryChanged(
            voiceManagerState.getSirenCategory()
        );
        VoiceManagerComponent::midiInputChanged(
            voiceManagerState.getMidiInput()
        );
        VoiceManagerComponent::midiOutputChanged(
            voiceManagerState.getMidiOutput()
        );
    }

    ~VoiceManagerComponent() override
    {
        for (auto* menu : menus) {
            menu->setLookAndFeel(nullptr);
            menu->removeListener(this);
        }
        inputChannelLabel.setLookAndFeel(nullptr);
        outputChannelLabel.setLookAndFeel(nullptr);
        categoryLabel.setLookAndFeel(nullptr);
        voiceManagerState.removeListener(this);
    }

    // VoiceManagerState::Listener overrides -----------------------------------
    void categoryChanged(sirenCategory cat) override {
        if (updatingCategoryFromMenu.load()) { return; }
        category.setSelectedId(
            menuIndexBySirenCategory.at(cat),
            juce::dontSendNotification
        );
    }
    void midiInputChanged(AnyOrOneBasedMidiChannel ch) override {
        if (updatingInputChannelFromMenu.load()) { return; }
        inputChannel.setSelectedId(
            menuIndexByMidiChannel.at(ch),
            juce::dontSendNotification
        );
    }
    void midiOutputChanged(AnyOrOneBasedMidiChannel ch) override {
        if (updatingOutputChannelFromMenu.load()) { return; }
        outputChannel.setSelectedId(
            menuIndexByMidiChannel.at(ch),
            juce::dontSendNotification
        );
    }

    // juce::Component overrides -----------------------------------------------
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

    // juce::ComboBox::Listener ------------------------------------------------
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override {
        if (comboBoxThatHasChanged == &category) {
            auto c = sirenCategoryByMenuIndex.find(category.getSelectedId());
            if (c != sirenCategoryByMenuIndex.end()) {
                ScopedGuard s(updatingCategoryFromMenu);
                voiceManagerState.setSirenCategory(c->second, true);
            }
        } else if (comboBoxThatHasChanged == &inputChannel) {
            auto i = midiChannelByMenuIndex.find(inputChannel.getSelectedId());
            if (i != midiChannelByMenuIndex.end()) {
                ScopedGuard s(updatingInputChannelFromMenu);
                voiceManagerState.setMidiInput(i->second, true);
            }
        } else if (comboBoxThatHasChanged == &outputChannel) {
            auto o = midiChannelByMenuIndex.find(outputChannel.getSelectedId());
            if (o != midiChannelByMenuIndex.end()) {
                ScopedGuard s(updatingOutputChannelFromMenu);
                voiceManagerState.setMidiOutput(o->second, true);
            }
        }
    }
};


#endif //COMPOSESIREN_VOICEMANAGERCOMPONENT_H