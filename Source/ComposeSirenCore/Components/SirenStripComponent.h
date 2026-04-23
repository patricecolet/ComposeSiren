//
// Created by joseph larralde on 26/01/2026.
//

#ifndef COMPOSESIREN_SIRENSTRIPCOMPONENT_H
#define COMPOSESIREN_SIRENSTRIPCOMPONENT_H

#include <juce_gui_basics/juce_gui_basics.h>

#include "GuiCellGroup.h"
#include "SliderCell.h"
#include "apvtsUtilities.h"
#include "../lib/definitions/parameterDefinitions.h"

//==============================================================================
class SirenStripComponent : public juce::Component
{
    // give owning SirenTrackComponent access to all variables
    friend class SirenTrackComponent;

public:
    SirenStripComponent(juce::AudioProcessorValueTreeState& vts,
                        const parameterLayoutGroupData& layoutGroupData);
    ~SirenStripComponent() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

    float getMinWidth() const;
    void setShowGroupLabels(bool s);
    void setShowKnobLabels(bool s);
    void setShowTextBox(bool s);
    void setBackgroundColour(juce::Colour c);
    void setBackgroundStripColour(juce::Colour c);

private:
    std::string appendCCNumber(const std::string& s, const ParameterId id);
    void setSliderFillColour(SliderCell& sc, juce::Colour c);

    juce::AudioProcessorValueTreeState& apvts;
    const parameterLayoutGroupData& paramGroupData;

    bool showGroupLabels = true;
    bool showKnobLabels = true;
    bool showTextBox = true;

    IncDecLAF incDecLAF;
    CentredKnobLAF centredKnobLAF;
    NotchedKnobLAF notchedKnobLAF;

    juce::Colour backgroundColour{juce::Colours::black};
    juce::Colour backgroundStripColour{
        juce::Colour{mecaviv::Colours::backgroundStripGrey}
    };

    // Groups
    GuiCellGroup startGroup;
    GuiCellGroup vibGroup;
    GuiCellGroup tremGroup;
    GuiCellGroup envGroup;
    GuiCellGroup endGroup;

    // Spacers between groups
    Spacer spacer1;
    Spacer spacer2;
    Spacer spacer3;
    Spacer spacer4;
    Spacer spacer5;
    Spacer spacer6;

    // Sliders (cells)
    const float ksw = controlStripLayout::minKnobSliderWidth;
    const float idsw = controlStripLayout::minIncDecSliderWidth;
    const float sh = controlStripLayout::minSliderHeight;
    const float lh = controlStripLayout::sliderLabelHeight;

    SliderCell pitchBend        {ksw,  sh, lh};
    SliderCell pitchBendRange   {idsw, sh, lh};
    SliderCell transpose        {idsw, sh, lh};
    SliderCell portamento       {ksw,  sh, lh};

    SliderCell vibSpeed         {ksw,  sh, lh};
    SliderCell vibDepth         {ksw,  sh, lh};
    SliderCell vibAccel         {ksw,  sh, lh};

    SliderCell tremSpeed        {ksw,  sh, lh};
    SliderCell tremDepth        {ksw,  sh, lh};

    SliderCell envAttack        {ksw,  sh, lh};
    SliderCell envRelease       {ksw,  sh, lh};

    SliderCell timbre           {ksw,  sh, lh};
    SliderCell mute             {ksw,  sh, lh};
    SliderCell volume           {ksw,  sh, lh};

    SliderCell pan              {ksw, sh, lh};
    SliderCell outputGain       {ksw, sh, lh};
};

#endif //COMPOSESIREN_SIRENSTRIPCOMPONENT_H
