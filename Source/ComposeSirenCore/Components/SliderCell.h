//
// Created by joseph larralde on 27/01/2026.
//

#ifndef COMPOSESIREN_SLIDERCELL_H
#define COMPOSESIREN_SLIDERCELL_H

#include "LookAndFeels.h"
#include "../lib/definitions/parameterDefinitions.h"

class SliderCell : public juce::Component
{
    using juceSliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
public:
    SliderCell(float minw, float minh, float labelh) :
        showLabel(true),
        showTextBox(true),
        minWidth(minw),
        minHeight(minh),
        labelHeight(labelh)
    {
        nameLabel.setJustificationType(juce::Justification::centred);
        nameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        nameLabel.setFont(juce::FontOptions(controlStripLayout::sliderLabelFontSize,
                                            juce::Font::plain));
        addAndMakeVisible(nameLabel);

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

    void setNameText(const juce::String& text) {
        nameLabel.setText(text, juce::dontSendNotification);
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

    float getMinWidth() const { return minWidth; }
    void setMinWidth(float w) { minWidth = w; }

    float getMinHeight() const { return minHeight; }
    void setMinHeight(float h) { minHeight = h; }

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

    void paint(juce::Graphics& g) override {
        g.setColour(juce::Colour{0x22ffffff});
        auto area = getLocalBounds();
        g.fillRoundedRectangle(area.toFloat(), controlStripLayout::cornerSize);
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

        fb.items.add(juce::FlexItem(slider).withMinWidth(minWidth)
                                           .withMinHeight(minHeight)
                                           .withFlex(1));

        fb.performLayout(bounds.toFloat());

        // auto area = getLocalBounds();
        // const int labelH = 14;
        // if (nameLabel.isVisible()) {
        //     auto labelArea = area.removeFromTop(labelH);
        //     nameLabel.setBounds(labelArea);
        // } else {
        //     nameLabel.setBounds(0, 0, 0, 0);
        // }
        // slider.setBounds(area.reduced(0,0));
    }

private:
    juce::Label nameLabel;
    juce::Slider slider;
    std::unique_ptr<juceSliderAttachment> attachment;

    bool showLabel;
    bool showTextBox;

    // needed for flexbox style UI integration
    float minWidth;
    float minHeight;
    float labelHeight;

    // default look and feel
    KnobLAF knobLAF;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SliderCell)
};

#endif //COMPOSESIREN_SLIDERCELL_H
