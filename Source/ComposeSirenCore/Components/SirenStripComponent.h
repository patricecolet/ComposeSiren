//
// Created by joseph larralde on 26/01/2026.
//

#ifndef COMPOSESIREN_SIRENSTRIPCOMPONENT_H
#define COMPOSESIREN_SIRENSTRIPCOMPONENT_H

#include <juce_gui_basics/juce_gui_basics.h>

#include "SliderCellGroup.h"
#include "apvtsUtilities.h"
#include "../lib/definitions/parameterDefinitions.h"

//==============================================================================
class SirenStripComponent : public juce::Component,
                            private juce::Timer
{
public:
    SirenStripComponent(juce::AudioProcessorValueTreeState& vts,
                        const parameterLayoutGroupData& paramGroupData,
                        bool hasTitleArea = false);
    ~SirenStripComponent() override;

    void setCurrentSiren(sirenCategory c);

    void paint(juce::Graphics&) override;
    void resized() override;

    float getMinWidth();
    float getTitleWidth() const;
    float getParametersWidth(bool sirenParametersOnly = false);
    void setShowTitle(bool s);
    void setShowGroupLabels(bool s);
    void setShowKnobLabels(bool s);
    void setShowTextBox(bool s);

    void setBackgroundColour(juce::Colour c);

protected:
    void timerCallback() override;

    juce::AudioProcessorValueTreeState& apvts;

    sirenCategory currentCategory;
    juce::Label titleLabel;
    bool showTitle = true;
    bool showGroupLabels = true;
    bool showKnobLabels = true;
    bool showTextBox = true;

    bool hasTrackControls = false;
    bool hasTitle = false;

    IncDecLAF incDecLAF;
    KnobLAF2 knobLAF2;
    KnobLAF3 knobLAF3;

    juce::Colour backgroundColour = juce::Colour(40, 40, 60);

    // Groups
    SliderCellGroup startGroup;
    SliderCellGroup vibGroup;
    SliderCellGroup tremGroup;
    SliderCellGroup envGroup;
    SliderCellGroup endGroup;
    SliderCellGroup trackGroup;

    // Spacers between groups
    Spacer spacer0;
    Spacer spacer1;
    Spacer spacer2;
    Spacer spacer3;
    Spacer spacer4;
    Spacer spacer5;
    Spacer spacer6;
    Spacer spacer7;

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

    SliderCell outputGain       {ksw * 1.2f, sh, lh};
    SliderCell pan              {ksw, sh, lh};
};

#endif //COMPOSESIREN_SIRENSTRIPCOMPONENT_H
