//
// Created by joseph larralde on 13/04/2026.
//

#ifndef COMPOSESIREN_TOGGLECELL_H
#define COMPOSESIREN_TOGGLECELL_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "lib/definitions/parameterDefinitions.h"
#include "GuiCell.h"

class ToggleCell : public GuiCell
{
    using juceButtonAttachment
        = juce::AudioProcessorValueTreeState::ButtonAttachment;

public:
    ToggleCell(float minw, float minh, float labelh) :
        GuiCell(minw, minh, labelh)
    {
        // toggle.setClickingTogglesState(true);
        toggle.setLookAndFeel(&toggleLAF);
        addAndMakeVisible(toggle);
    }

    ~ToggleCell() override
    {
        attachment.reset();
        toggle.setLookAndFeel(nullptr);
    }

    void setState(bool e) {
        toggle.setToggleState(e, juce::dontSendNotification);
    }

    void setToggleLookAndFeel(juce::LookAndFeel* laf) {
        toggle.setLookAndFeel(laf);
    }

    void setToggleAttachment(juce::AudioProcessorValueTreeState& vts,
                             ParameterId paramId, const std::string& groupId) {
        attachment = std::make_unique<juceButtonAttachment>(
            vts, ParameterIdGet::toJuceParameterId(groupId, paramId), toggle
        );
    }

    juce::ToggleButton& getToggle() { return toggle; }
    const juce::ToggleButton& getToggle() const { return toggle; }
    bool compareToggle(juce::ToggleButton* other) const { return &toggle == other; }

    void setShowLabel(bool s) {
        nameLabel.setVisible(s);
        showLabel = s;
        resized();
    }

    void resized() override {
        auto bounds = getLocalBounds();

        if (nameLabel.isVisible()) {
            auto labelArea = bounds.removeFromTop(labelHeight);
            nameLabel.setBounds(labelArea);
        }

        juce::FlexBox fb;
        fb.flexDirection = juce::FlexBox::Direction::column;
        fb.justifyContent = juce::FlexBox::JustifyContent::center;
        fb.alignContent = juce::FlexBox::AlignContent::center;
        fb.alignItems = juce::FlexBox::AlignItems::center;

        fb.items.add(juce::FlexItem(toggle).withMinWidth(minWidth)
                                           .withMinHeight(minHeight)
                                           .withFlex(1));

        fb.performLayout(bounds.toFloat());
    }

private:
    juce::ToggleButton toggle;
    std::unique_ptr<juceButtonAttachment> attachment;

    ToggleLAF toggleLAF;
};

#endif //COMPOSESIREN_TOGGLECELL_H