//
// Created by joseph larralde on 09/03/2026.
//

#ifndef COMPOSESIREN_REVERBSTRIPCOMPONENT_H
#define COMPOSESIREN_REVERBSTRIPCOMPONENT_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "GuiCellGroup.h"
#include "SliderCell.h"
#include "ToggleCell.h"
#include "LookAndFeels.h"
#include "../apvtsUtilities.h"
#include "../lib/definitions/parameterDefinitions.h"

class ReverbStripComponent : public juce::Component//,
                             // public juce::Slider::Listener,
                             // public juce::Button::Listener,
                             // private juce::Timer
{
    juce::AudioProcessorValueTreeState& apvts;

    juce::Label titleLabel;
    bool showTitle = true;
    bool showGroupLabels = true;
    bool showKnobLabels = true;
    bool showTextBox = true;

    juce::Colour backgroundStripColour{
        juce::Colour{mecaviv::Colours::backgroundStripGrey}
    };

    juce::ToggleButton reverbEnableButton;

    GuiCellGroup enableGroup;
    GuiCellGroup reverbGroup;
    GuiCellGroup filterGroup;

    const float ksw = controlStripLayout::minKnobSliderWidth;
    const float idsw = controlStripLayout::minIncDecSliderWidth;
    const float sh = controlStripLayout::minSliderHeight * 0.95f;
    const float lh = controlStripLayout::sliderLabelHeight * 0.75f;

    ToggleCell enable     {ksw, sh, lh};

    SliderCell dryWet     {ksw, sh, lh};
    SliderCell damping    {ksw, sh, lh};
    SliderCell roomSize   {ksw, sh, lh};
    SliderCell width      {ksw, sh, lh};

    SliderCell lowCut     {ksw, sh, lh};
    SliderCell highCut    {ksw, sh, lh};

    Spacer spacer1;
    Spacer spacer2;
    Spacer spacer3;
    Spacer spacer4;

public:
    ReverbStripComponent(juce::AudioProcessorValueTreeState& vts,
                         const std::string& paramGroupId);
    ~ReverbStripComponent() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

    void setShowTitle(bool s);
    void setShowGroupLabels(bool s);
    void setShowKnobLabels(bool s);
    void setShowTextBox(bool s);

    void setBackgroundColour(juce::Colour c);
    void setCellBackgroundColour(juce::Colour c);
};

#endif //COMPOSESIREN_REVERBSTRIPCOMPONENT_H
