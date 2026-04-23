//
// Created by joseph larralde on 12/04/2026.
//

#ifndef COMPOSESIREN_MASTERVOLUME_COMPONENT_H
#define COMPOSESIREN_MASTERVOLUME_COMPONENT_H

#include <juce_gui_basics/juce_gui_basics.h>

#include "GuiCellGroup.h"
#include "SliderCell.h"
#include "LookAndFeels.h"
#include "apvtsUtilities.h"
#include "../lib/definitions/parameterDefinitions.h"

//==============================================================================
class MasterVolumeComponent : public juce::Component
{
public:
    MasterVolumeComponent(juce::AudioProcessorValueTreeState& vts,
                          const std::string& paramGroupId);
    ~MasterVolumeComponent() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

    void setShowTitle(bool s);
    void setShowGroupLabels(bool s);
    void setShowKnobLabels(bool s);
    void setShowTextBox(bool s);

    void setBackgroundColour(juce::Colour c);
    void setCellBackgroundColour(juce::Colour c);

protected:
    juce::AudioProcessorValueTreeState& apvts;
    juce::Label titleLabel;
    NotchedKnobLAF notchedKnobLAF;

    // Groups
    GuiCellGroup masterGroup;
    // Spacers between groups
    Spacer spacer0;
    Spacer spacer1;
    // Sliders (cells)
    const float ksw = controlStripLayout::minKnobSliderWidth * 2;
    const float sh = controlStripLayout::minSliderHeight * 2;
    const float lh = controlStripLayout::sliderLabelHeight * 0.75f;
    SliderCell volume {ksw,  sh, lh};
};

#endif //COMPOSESIREN_MASTERVOLUME_COMPONENT_H