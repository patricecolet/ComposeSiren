//
// Created by joseph larralde on 13/04/2026.
//

#ifndef COMPOSESIREN_GUICELL_H
#define COMPOSESIREN_GUICELL_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "LookAndFeels.h"

class GuiCell : public juce::Component
{
public:
    GuiCell(float minw, float minh, float labelh) :
        showLabel(true),
        minWidth(minw),
        minHeight(minh),
        labelHeight(labelh)
    {
        nameLabel.setJustificationType(juce::Justification::centred);
        nameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        nameLabel.setFont(juce::FontOptions(controlStripLayout::sliderLabelFontSize,
                                            juce::Font::plain));
        addAndMakeVisible(nameLabel);
    }

    ~GuiCell() override = default;

    void setNameText(const juce::String& text) {
        nameLabel.setText(text, juce::dontSendNotification);
    }

    // we need to define these because we override juce::Component
    void paint(juce::Graphics& g) override {
        g.setColour(backgroundColour);
        auto area = getLocalBounds();
        g.fillRoundedRectangle(area.toFloat(), controlStripLayout::cornerSize);
    }

    void resized() override {} // no shared code here

    // used by CellGroup
    float getMinWidth() const { return minWidth; }
    void setMinWidth(float w) { minWidth = w; }

    float getMinHeight() const { return minHeight; }
    void setMinHeight(float h) { minHeight = h; }

    void setBackgroundColour(juce::Colour c) { backgroundColour = c; }

protected:
    juce::Label nameLabel;
    bool showLabel;

    // needed for flexbox style UI integration
    float minWidth;
    float minHeight;
    float labelHeight;

    juce::Colour backgroundColour{0x22ffffff};
};

#endif //COMPOSESIREN_GUICELL_H