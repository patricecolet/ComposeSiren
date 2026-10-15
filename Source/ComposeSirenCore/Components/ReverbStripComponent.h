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
#ifndef COMPOSESIREN_CLIC
#define COMPOSESIREN_CLIC 0
#endif
#if COMPOSESIREN_CLIC
#include "EngravedTitle.h"
#endif
#include "LookAndFeels.h"
#include "../apvtsUtilities.h"
#include "../lib/definitions/parameterDefinitions.h"

class ReverbStripComponent : public juce::Component
{
    juce::AudioProcessorValueTreeState& apvts;

#if COMPOSESIREN_CLIC
    // clic build: engraved title + header enable (Enable cell lives in the header)
    EngravedTitle titleLabel;
    juce::ToggleButton titleSwitch;
    HeaderSwitchLAF headerSwitchLAF;
    // the switch follows ReverbEnable both ways: shows the restored state when the editor
    // opens, and any later change (a host automation, setStateInformation, MCP)
    std::unique_ptr<juce::ButtonParameterAttachment> titleSwitchAttachment;
#else
    // master layout: plain label (title is optional); Enable Reverb is a cell
    juce::Label titleLabel;
#endif
    bool showTitle = true;
    bool showGroupLabels = true;
    bool showKnobLabels = true;
    bool showTextBox = true;

    juce::Colour backgroundStripColour{
        juce::Colour{mecaviv::Colours::backgroundStripGrey}
    };

#if !COMPOSESIREN_CLIC
    GuiCellGroup enableGroup;
#endif
    GuiCellGroup reverbGroup;
    GuiCellGroup filterGroup;

#if COMPOSESIREN_CLIC
    // denser / shorter knobs when CLIC borrows width; labels sit lower
    const float ksw = controlStripLayout::minKnobSliderWidth * 0.82f;
    const float sh = controlStripLayout::minSliderHeight * 0.72f;
    const float lh = controlStripLayout::sliderLabelHeight * 0.5f;
#else
    const float ksw = controlStripLayout::minKnobSliderWidth;
    const float sh = controlStripLayout::minSliderHeight * 0.95f;
    const float lh = controlStripLayout::sliderLabelHeight * 0.75f;
#endif
    const float idsw = controlStripLayout::minIncDecSliderWidth;

#if !COMPOSESIREN_CLIC
    ToggleCell enable     {ksw, sh, lh};
#endif

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

    void setTitle(const juce::String& t)
    {
#if COMPOSESIREN_CLIC
        titleLabel.setText(t);
#else
        titleLabel.setText(t, juce::dontSendNotification);
#endif
    }
    void setShowTitle(bool s);
    void setShowGroupLabels(bool s);
    void setShowKnobLabels(bool s);
    void setShowTextBox(bool s);

    void setBackgroundColour(juce::Colour c);
    void setCellBackgroundColour(juce::Colour c);
    void setBackgroundStripColour(juce::Colour c);
};

#endif //COMPOSESIREN_REVERBSTRIPCOMPONENT_H
