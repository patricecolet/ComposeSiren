//
// Created by joseph larralde on 09/03/2026.
//

#include "ReverbStripComponent.h"

ReverbStripComponent::ReverbStripComponent(juce::AudioProcessorValueTreeState& vts) :
    apvts(vts)
{
    const float gap = controlStripLayout::spacerSize;

    enableGroup.setTitleText("Enable");
    enableGroup.setGap(gap);
    enableGroup.setWrap(false);
    addAndMakeVisible(&enableGroup);

    // Groups + spacers
    reverbGroup.setTitleText("Reverb");
    reverbGroup.setGap(gap);
    reverbGroup.setWrap(false);
    addAndMakeVisible(reverbGroup);

    filterGroup.setTitleText("Filter");
    filterGroup.setGap(gap);
    filterGroup.setWrap(false);
    addAndMakeVisible(filterGroup);

    addAndMakeVisible(spacer1);
    addAndMakeVisible(spacer2);
    addAndMakeVisible(spacer3);
    addAndMakeVisible(spacer4);

    juce::Colour c1 = juce::Colours::whitesmoke;//juce::Colours::lightgoldenrodyellow;
    juce::Colour c2 = juce::Colours::whitesmoke;
    juce::Colour c3 = c1;

    auto setSliderFillColour = [&](SliderCell& sc, juce::Colour c) {
        sc.getSlider().setColour(juce::Slider::ColourIds::rotarySliderFillColourId, c);
    };

    // --- reverb enable switch (horizontal) -----------------------------------

    enable.setNameText("Enable");
    enable.setRange(0.0, 1.0, 1.0);
    juce::Slider& es = enable.getSlider();
    // es.addListener(this);
    setSliderFillColour(enable, c1);
    enableGroup.addAndMakeVisible(enable);

    // --- reverb group knobs (horizontal) -------------------------------------

    dryWet.setNameText("DryWet");
    dryWet.setRange(0.0, 1.0, 0.01);
    juce::Slider& dws = dryWet.getSlider();
    // dws.addListener(this);
    setSliderFillColour(dryWet, c1);
    reverbGroup.addAndMakeVisible(dryWet);

    damping.setNameText("Damp");
    damping.setRange(0.0, 1.0, 0.01);
    juce::Slider& ds = damping.getSlider();
    // ds.addListener(this);
    setSliderFillColour(damping, c1);
    reverbGroup.addAndMakeVisible(damping);

    roomSize.setNameText("Size");
    roomSize.setRange(0.0, 1.0, 0.01);
    juce::Slider& rss = roomSize.getSlider();
    // rss.addListener(this);
    setSliderFillColour(roomSize, c1);
    reverbGroup.addAndMakeVisible(roomSize);

    width.setNameText("Width");
    width.setRange(0.0, 1.0, 0.01);
    juce::Slider& ws = width.getSlider();
    // ws.addListener(this);
    setSliderFillColour(width, c1);
    reverbGroup.addAndMakeVisible(width);

    // --- reverb group knobs (horizontal) -------------------------------------

    lowCut.setNameText("LowCut");
    lowCut.setRange(0.0, 1.0, 0.01);
    juce::Slider& lcs = lowCut.getSlider();
    // lcs.addListener(this);
    setSliderFillColour(lowCut, c1);
    filterGroup.addAndMakeVisible(lowCut);

    highCut.setNameText("HighCut");
    highCut.setRange(0.0, 1.0, 0.01);
    juce::Slider& hcs = highCut.getSlider();
    // hcs.addListener(this);
    setSliderFillColour(highCut, c1);
    filterGroup.addAndMakeVisible(highCut);
}

// ReverbStripComponent::~ReverbStripComponent() {}

void ReverbStripComponent::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().reduced(2).toFloat();

    g.setColour(juce::Colour{0xff314159});
    g.fillRoundedRectangle(area, 12);
}

void ReverbStripComponent::resized()
{
    auto area = getLocalBounds().reduced(0);

    // int area_height;

    // if (showKnobLabels) {
    //     area_height = 88;
    // } else {
    //     area_height = 80;
    // }
    //
    // if (!showGroupLabels) {
    //     area_height -= 16;
    // }
    //
    // if (!showTextBox) {
    //     area_height -= 16;
    // }

    const int area_height = area.getHeight();

    // Parent FlexBox: [ vibGroup ] [ spacer ] [ tremGroup ] [ spacer ] [ envGroup ]
    juce::FlexBox root;
    root.flexDirection = juce::FlexBox::Direction::row;
    root.flexWrap = juce::FlexBox::Wrap::noWrap;
    root.justifyContent = juce::FlexBox::JustifyContent::flexEnd;
    // root.justifyContent = juce::FlexBox::JustifyContent::center;
    root.alignItems = juce::FlexBox::AlignItems::center;

    const float spacerW = 2.0f;
    const float gap = controlStripLayout::spacerSize;

    // NB : withMinWidth is 52 for 1, 103 for 2, 154 for 3 etc
    // root.items.add(juce::FlexItem(spacer1).withFlex(0,0).withWidth(gap).withHeight((float) area_height));
    root.items.add(juce::FlexItem(spacer2).withFlex(0,0).withWidth(gap).withHeight((float) area_height));
    root.items.add(juce::FlexItem(reverbGroup).withFlex(0,0)
                                             .withMinWidth(reverbGroup.getMinWidth())
                                             .withHeight((float) area_height));
    root.items.add(juce::FlexItem(spacer3).withFlex(0,0).withWidth(gap).withHeight((float) area_height));
    root.items.add(juce::FlexItem(filterGroup).withFlex(0,0)
                                             .withMinWidth(filterGroup.getMinWidth())
                                             .withHeight((float) area_height));
    root.items.add(juce::FlexItem(spacer4).withFlex(0,0).withWidth(gap).withHeight((float) area_height));

    root.performLayout(area.toFloat());

    reverbGroup.resized();
    filterGroup.resized();
}

void ReverbStripComponent::setShowTitle(bool s)
{
    showTitle = s;
    titleLabel.setVisible(s);
    resized();
}

void ReverbStripComponent::setShowGroupLabels(bool s)
{
    showGroupLabels = s;
    reverbGroup.setShowLabel(s);
    filterGroup.setShowLabel(s);

    resized();
}

void ReverbStripComponent::setShowKnobLabels(bool s)
{
    showKnobLabels = s;
    dryWet.setShowLabel(s);
    damping.setShowLabel(s);
    roomSize.setShowLabel(s);
    width.setShowLabel(s);
    lowCut.setShowLabel(s);
    highCut.setShowLabel(s);

    resized();
}

void ReverbStripComponent::setShowTextBox(bool s)
{
    showTextBox = s;
    dryWet.setShowTextBox(s);
    damping.setShowTextBox(s);
    roomSize.setShowTextBox(s);
    width.setShowTextBox(s);
    lowCut.setShowTextBox(s);
    highCut.setShowTextBox(s);
    // always show values for inc dec sliders
    // pitchBendRange.setShowTextBox(true);
    // transpose.setShowTextBox(true);

    resized();
}

void ReverbStripComponent::setBackgroundColour(juce::Colour c)
{
    backgroundColour = c;
    reverbGroup.setBackgroundColour(c);
    filterGroup.setBackgroundColour(c);
    repaint();
}
