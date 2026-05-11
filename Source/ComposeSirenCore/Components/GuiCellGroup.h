//
// Created by joseph larralde on 07/03/2026.
//

#ifndef COMPOSESIREN_GUICELLGROUP_H
#define COMPOSESIREN_GUICELLGROUP_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "GuiCell.h"

/**
 * Just a Spacer utility class. Is it really useful though ? We should be
 * able to use margins on flexboxes instead as in the SlicerCellGroup class
 * todo : remove in favor of using margins
 */
class Spacer : public juce::Component {
public:
    Spacer() { setInterceptsMouseClicks(false, false); }
    void paint(juce::Graphics& g) override {
        // g.setColour(Colours::white);
        // g.fillRect(getLocalBounds().toFloat());
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Spacer)
};

/**
 *  This is a horizontal Slider group. If ever needed, a vertical version would
 *  be better defined in its own separate class, as the slider's layouts would
 *  probably be different too.
 *  The slider group dimensions should be computed in the parent component,
 *  as the flexbox in juce is just a convenience to make layout creation code
 *  more elegant, and the spacers and dimensions are best defined statically.
 */
class GuiCellGroup : public juce::Component
{
public:
    explicit GuiCellGroup() :
        gap(0),
        wrap(false),
        showLabel(false),
        showTextBox(false)
    {
        groupLabel.setJustificationType(juce::Justification::centred);
        groupLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.85f));
        groupLabel.setFont(juce::FontOptions(controlStripLayout::groupLabelFontSize,
                                             juce::Font::bold));
        addAndMakeVisible(groupLabel);
    }

    void setTitleText(const juce::String& text) {
        groupLabel.setText(text, juce::dontSendNotification);
        resized();
    }

    const std::string getTitleText() const {
        return groupLabel.getText().toStdString();
    }

    void setGap(float g) { gap = g; resized(); }
    void setWrap(bool shouldWrap) { wrap = shouldWrap; resized(); }
    void setShowLabel(bool shouldShow) { showLabel = shouldShow; resized(); }
    void setShowTextBox(bool shouldShow) { showTextBox = shouldShow; resized(); }
    void setBackgroundColour(juce::Colour c) { backgroundColour = c; repaint(); }
    void setCellBackgroundColour(juce::Colour c) {
        for (auto* child : getChildren()) {
            if (auto* cell = dynamic_cast<GuiCell*>(child)) {
                cell->setBackgroundColour(c);
            }
        }
    }
    void setTextColour(juce::Colour c) {
        textColour = c;
        groupLabel.setColour(juce::Label::textColourId, c.withAlpha(0.95f));
    }

    float getMinWidth() const {
        int cnt = 0;
        float w = 0;
        for (auto* c : getChildren()) {
            // ignore group label component
            if (const auto* cell = dynamic_cast<GuiCell*>(c)) {
                w += cell->getMinWidth();
                cnt++;
            }
        }
        return w + static_cast<float>(cnt + 1) * gap;
    }

    void paint(juce::Graphics& g) override {
        g.setColour(backgroundColour);
        auto area = getLocalBounds();
        g.fillRoundedRectangle(area.toFloat(), controlStripLayout::cornerSize);
    }

    void resized() override {
        auto bounds = getLocalBounds();
        constexpr float titleAreaHeight = controlStripLayout::groupLabelHeight;

        if (showLabel) {
            auto groupTitleArea = bounds.removeFromTop(titleAreaHeight);
            groupLabel.setBounds(groupTitleArea);
        }

        juce::FlexBox fb;
        fb.flexWrap = wrap
                    ? juce::FlexBox::Wrap::wrap
                    : juce::FlexBox::Wrap::noWrap;
        fb.flexDirection = juce::FlexBox::Direction::row;
        fb.justifyContent = juce::FlexBox::JustifyContent::center; // <- this !!
        fb.alignContent   = juce::FlexBox::AlignContent::center;//flexStart;
        fb.alignItems     = juce::FlexBox::AlignItems::center;//flexStart;

        bool first = true;

        for (auto* c : getChildren()) {
            // ignore group label component
            if (auto* cell = dynamic_cast<GuiCell*>(c)) {
                float w = cell->getMinWidth();
                // float h = cell->getMinHeight();
                float h = bounds.getHeight() - 2 * gap;

                auto item = juce::FlexItem(*cell).withMinWidth(w)
                                                 .withMinHeight(h)
                                                 .withFlex(1);

                if (first) {
                    item.margin = juce::FlexItem::Margin(0.f, (float) gap, 0.f, (float) gap); // r/l gaps
                } else {
                    item.margin = juce::FlexItem::Margin(0.f, (float) gap, 0.f, 0.f); // r gap
                }

                first = false;
                fb.items.add(item);
            }
        }

        fb.performLayout(bounds);
    }

private:
    float gap;
    bool wrap;

    bool showLabel;
    bool showTextBox;
    juce::Label groupLabel;
    juce::Colour backgroundColour = juce::Colour(40, 40, 60);
    juce::Colour textColour = juce::Colours::white.withAlpha(0.95f);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuiCellGroup)
};

#endif //COMPOSESIREN_GUICELLGROUP_H
