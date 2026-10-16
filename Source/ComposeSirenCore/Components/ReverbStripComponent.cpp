//
// Created by joseph larralde on 09/03/2026.
//

#include "ReverbStripComponent.h"

ReverbStripComponent::ReverbStripComponent(
    juce::AudioProcessorValueTreeState& vts,
    const std::string& paramGroupId
) :
    apvts(vts)
{
    const float gap = 0;//controlStripLayout::spacerSize;

#if COMPOSESIREN_CLIC
    addAndMakeVisible(titleSwitch);
    addAndMakeVisible(titleLabel);
    titleLabel.setText("Reverb");
    // header enable: ToggleLAF button with LED on the left
    titleSwitch.setButtonText({});
    titleSwitch.setLookAndFeel(&headerSwitchLAF);
    if (auto* p = vts.getParameter(
            ParameterIdGet::toJuceParameterId(paramGroupId, ParameterId::ReverbEnable)))
        titleSwitchAttachment = std::make_unique<juce::ButtonParameterAttachment>(*p, titleSwitch);
#else
    // master layout
    addAndMakeVisible(titleLabel);
    enableGroup.setTitleText("Enable");
    enableGroup.setGap(gap);
    enableGroup.setWrap(false);
    addAndMakeVisible(enableGroup);
#endif

    // Groups + spacers
    reverbGroup.setTitleText("Reverb");
    reverbGroup.setGap(gap);
    reverbGroup.setWrap(false);
    addAndMakeVisible(reverbGroup);

    filterGroup.setTitleText("Filter");
    filterGroup.setGap(gap);
    filterGroup.setWrap(false);
    addAndMakeVisible(filterGroup);

    addAndMakeVisible(spacer2);
    addAndMakeVisible(spacer3);

    juce::Colour c1 = juce::Colours::whitesmoke;

    auto setSliderFillColour = [&](SliderCell& sc, juce::Colour c) {
        sc.getSlider().setColour(juce::Slider::ColourIds::rotarySliderFillColourId, c);
    };

#if !COMPOSESIREN_CLIC
    // --- reverb enable switch (master: dedicated cell) -----------------------
    enable.setNameText(midiCcKnobLabel("Enable Reverb", ParameterId::ReverbEnable));
    enable.setToggleAttachment(vts,
                               ParameterId::ReverbEnable,
                               paramGroupId);
    enableGroup.addAndMakeVisible(enable);
#endif

    // --- reverb group knobs (horizontal) -------------------------------------
    dryWet.setNameText(midiCcKnobLabel("DryWet", ParameterId::ReverbDryWet));
    dryWet.setSliderAttachment(vts, ParameterId::ReverbDryWet, paramGroupId);
    setSliderFillColour(dryWet, c1);
    reverbGroup.addAndMakeVisible(dryWet);

    damping.setNameText(midiCcKnobLabel("Damp", ParameterId::ReverbDamping));
    damping.setSliderAttachment(vts, ParameterId::ReverbDamping, paramGroupId);
    setSliderFillColour(damping, c1);
    reverbGroup.addAndMakeVisible(damping);

    roomSize.setNameText(midiCcKnobLabel("Size", ParameterId::ReverbRoomSize));
    roomSize.setSliderAttachment(vts, ParameterId::ReverbRoomSize, paramGroupId);
    setSliderFillColour(roomSize, c1);
    reverbGroup.addAndMakeVisible(roomSize);

    width.setNameText(midiCcKnobLabel("Width", ParameterId::ReverbWidth));
    width.setSliderAttachment(vts, ParameterId::ReverbWidth, paramGroupId);
    setSliderFillColour(width, c1);
    reverbGroup.addAndMakeVisible(width);

    // --- filter group knobs --------------------------------------------------
    lowCut.setNameText(midiCcKnobLabel("LowCut", ParameterId::ReverbLowCut));
    lowCut.setSliderAttachment(vts, ParameterId::ReverbLowCut, paramGroupId);
    setSliderFillColour(lowCut, c1);
    filterGroup.addAndMakeVisible(lowCut);

    highCut.setNameText(midiCcKnobLabel("HighCut", ParameterId::ReverbHighCut));
    highCut.setSliderAttachment(vts, ParameterId::ReverbHighCut, paramGroupId);
    setSliderFillColour(highCut, c1);
    filterGroup.addAndMakeVisible(highCut);
}

void ReverbStripComponent::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    g.setColour(backgroundStripColour);
    g.fillRoundedRectangle(area, controlStripLayout::cornerSize);
}

void ReverbStripComponent::resized()
{
    auto area = getLocalBounds().reduced(controlStripLayout::spacerSize);

#if COMPOSESIREN_CLIC
    if (showTitle) {
        auto header = area.removeFromTop(20);
        titleSwitch.setBounds(header.removeFromLeft(44));
        titleLabel.setBounds(header);
    }
#endif

    const int area_height = area.getHeight();

    juce::FlexBox root;
    root.flexDirection = juce::FlexBox::Direction::row;
    root.flexWrap = juce::FlexBox::Wrap::noWrap;
    root.justifyContent = juce::FlexBox::JustifyContent::center;
    root.alignItems = juce::FlexBox::AlignItems::center;

    const float gap = controlStripLayout::spacerSize;

#if COMPOSESIREN_CLIC
    // no Enable cell — 6 knobs take the width
    root.items.add(juce::FlexItem(reverbGroup).withFlex(4/6.f, 0)
                                              .withMinWidth(reverbGroup.getMinWidth())
                                              .withHeight((float) area_height));
    root.items.add(juce::FlexItem(spacer3).withFlex(0, 0).withWidth(gap).withHeight((float) area_height));
    root.items.add(juce::FlexItem(filterGroup).withFlex(2/6.f, 0)
                                              .withMinWidth(filterGroup.getMinWidth())
                                              .withHeight((float) area_height));

    root.performLayout(area.toFloat());
    reverbGroup.resized();
    filterGroup.resized();
#else
    // master layout: Enable | Reverb (4) | Filter (2)
    root.items.add(juce::FlexItem(enableGroup).withFlex(1/7.f, 0)
                                              .withWidth(enableGroup.getMinWidth())
                                              .withHeight((float) area_height));
    root.items.add(juce::FlexItem(spacer2).withFlex(0, 0).withWidth(gap).withHeight((float) area_height));
    root.items.add(juce::FlexItem(reverbGroup).withFlex(4/7.f, 0)
                                              .withMinWidth(reverbGroup.getMinWidth())
                                              .withHeight((float) area_height));
    root.items.add(juce::FlexItem(spacer3).withFlex(0, 0).withWidth(gap).withHeight((float) area_height));
    root.items.add(juce::FlexItem(filterGroup).withFlex(2/7.f, 0)
                                              .withMinWidth(filterGroup.getMinWidth())
                                              .withHeight((float) area_height));

    root.performLayout(area.toFloat());
    enableGroup.resized();
    reverbGroup.resized();
    filterGroup.resized();
#endif
}

void ReverbStripComponent::setShowTitle(bool s)
{
    titleLabel.setVisible(s);
    resized();
}

void ReverbStripComponent::setShowGroupLabels(bool s)
{
#if !COMPOSESIREN_CLIC
    enableGroup.setShowLabel(s);
#endif
    reverbGroup.setShowLabel(s);
    filterGroup.setShowLabel(s);
    resized();
}

void ReverbStripComponent::setShowKnobLabels(bool s)
{
#if !COMPOSESIREN_CLIC
    enable.setShowLabel(s);
#endif
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
    dryWet.setShowTextBox(s);
    damping.setShowTextBox(s);
    roomSize.setShowTextBox(s);
    width.setShowTextBox(s);
    lowCut.setShowTextBox(s);
    highCut.setShowTextBox(s);
    resized();
}

void ReverbStripComponent::setBackgroundColour(juce::Colour c)
{
#if !COMPOSESIREN_CLIC
    enableGroup.setBackgroundColour(c);
#endif
    reverbGroup.setBackgroundColour(c);
    filterGroup.setBackgroundColour(c);
    repaint();
}

void ReverbStripComponent::setCellBackgroundColour(juce::Colour c)
{
#if !COMPOSESIREN_CLIC
    enableGroup.setCellBackgroundColour(c);
#endif
    reverbGroup.setCellBackgroundColour(c);
    filterGroup.setCellBackgroundColour(c);
    repaint();
}

void ReverbStripComponent::setBackgroundStripColour(juce::Colour c)
{
    backgroundStripColour = c;
    repaint();
}
