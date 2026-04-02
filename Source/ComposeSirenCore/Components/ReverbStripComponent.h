//
// Created by joseph larralde on 09/03/2026.
//

#ifndef COMPOSESIREN_REVERBSTRIPCOMPONENT_H
#define COMPOSESIREN_REVERBSTRIPCOMPONENT_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "SliderCellGroup.h"
#include "LookAndFeels.h"

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

    juce::Colour backgroundColour = juce::Colour(40, 40, 60);
    juce::ToggleButton reverbEnableButton;

    SliderCellGroup reverbGroup;
    SliderCellGroup filterGroup;

    const float ksw = controlStripLayout::minKnobSliderWidth;
    const float idsw = controlStripLayout::minIncDecSliderWidth;
    const float sh = controlStripLayout::minSliderHeight;
    const float lh = controlStripLayout::sliderLabelHeight;

    SliderCell dryWet     {ksw, sh, lh};
    SliderCell damping    {ksw, sh, lh};
    SliderCell roomSize   {ksw, sh, lh};
    SliderCell width      {ksw, sh, lh};;

    SliderCell lowCut     {ksw, sh, lh};
    SliderCell highCut    {ksw, sh, lh};

    Spacer spacer1;
    Spacer spacer2;
    Spacer spacer3;
    Spacer spacer4;

public:
    ReverbStripComponent(juce::AudioProcessorValueTreeState& vts);
    ~ReverbStripComponent() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;

    void setShowTitle(bool s);
    void setShowGroupLabels(bool s);
    void setShowKnobLabels(bool s);
    void setShowTextBox(bool s);

    void setBackgroundColour(juce::Colour c);
};

#endif //COMPOSESIREN_REVERBSTRIPCOMPONENT_H
