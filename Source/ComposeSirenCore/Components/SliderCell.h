//
// Created by joseph larralde on 27/01/2026.
//

#ifndef COMPOSESIREN_SLIDERCELL_H
#define COMPOSESIREN_SLIDERCELL_H

#include "LookAndFeels.h"
#include "GuiCell.h"
#include "../lib/definitions/parameterDefinitions.h"

class SliderCell : public GuiCell
{
    using juceSliderAttachment
        = juce::AudioProcessorValueTreeState::SliderAttachment;

public:
    SliderCell(float minw, float minh, float labelh) :
        GuiCell(minw, minh, labelh),
        showTextBox(true)
    {
        slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 42, 16); // value at bottom
        slider.setNumDecimalPlacesToDisplay(2);
        slider.setLookAndFeel(&knobLAF);
        addAndMakeVisible(slider);
    }

    ~SliderCell() override
    {
        attachment.reset();
        slider.setLookAndFeel(nullptr);
    }

    void setRange(double min, double max, double step = 0.0) {
        slider.setRange(min, max, step);
    }

    void setValue(double v) {
        slider.setValue(v, juce::dontSendNotification);
    }

    void setSuffix(const juce::String& s) {
        slider.setTextValueSuffix(s);
    }

    void setSliderLookAndFeel(juce::LookAndFeel* laf) {
        slider.setLookAndFeel(laf);
    }

    void setSliderAttachment(juce::AudioProcessorValueTreeState& vts,
                             ParameterId paramId, const std::string& groupId) {
        attachment = std::make_unique<juceSliderAttachment>(
            vts, ParameterIdGet::toJuceParameterId(groupId, paramId), slider
        );
    }

    juce::Slider& getSlider() { return slider; }
    const juce::Slider& getSlider() const { return slider; }
    bool compareSlider(juce::Slider* other) const { return &slider == other; }

    void setShowLabel(bool s) {
        slider.getProperties().set("compactTextBox", !s);
        nameLabel.setVisible(s);
        showLabel = s;
        resized();
    }

    void setShowTextBox(bool s) {
        if (s) {
            slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 42, 16);
        } else {
            slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        }
        showTextBox = s;
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
        fb.alignContent = juce::FlexBox::AlignContent::flexEnd;
        fb.alignItems = juce::FlexBox::AlignItems::center;

        fb.items.add(juce::FlexItem(slider).withMinWidth(minWidth)
                                           .withMinHeight(minHeight)
                                           .withFlex(1));

        fb.performLayout(bounds.toFloat());
    }

private:
    juce::Slider slider;
    std::unique_ptr<juceSliderAttachment> attachment;

    // default look and feel
    KnobLAF knobLAF;
    bool showTextBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SliderCell)
};

#endif //COMPOSESIREN_SLIDERCELL_H
