//
// Created by joseph larralde on 12/04/2026.
//

#include "MasterVolumeComponent.h"

MasterVolumeComponent::MasterVolumeComponent(juce::AudioProcessorValueTreeState& vts,
                                             const std::string& paramGroupId) :
    apvts(vts)
{
    const float gap = controlStripLayout::spacerSize;

    masterGroup.setTitleText("Master Volume");
    masterGroup.setGap(gap);
    masterGroup.setWrap(false);
    addAndMakeVisible(masterGroup);

    addAndMakeVisible(spacer0);
    addAndMakeVisible(spacer1);

    volume.setNameText("Master Volume");
    volume.setSliderAttachment(vts,
                               ParameterId::MasterVolume,
                               paramGroupId);
    juce::Slider& dws = volume.getSlider();
    // dws.setSliderStyle(juce::Slider::LinearVertical);
    dws.setLookAndFeel(&notchedKnobLAF);
    masterGroup.addAndMakeVisible(volume);
}

MasterVolumeComponent::~MasterVolumeComponent()
{
    // why is this not required although it is in other places ?
    // volume.getSlider().setLookAndFeel(nullptr);
}

void MasterVolumeComponent::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();

    // g.setColour(juce::Colour{0xff314159});
    g.setColour(backgroundStripColour);
    g.fillRoundedRectangle(area, controlStripLayout::cornerSize);
    // todo : draw tick bars for dBs here
}

void MasterVolumeComponent::resized()
{
    auto area = getLocalBounds();//.reduced(0);
    const int area_height = area.getHeight();

    // Parent FlexBox: [ vibGroup ] [ spacer ] [ tremGroup ] [ spacer ] [ envGroup ]
    juce::FlexBox root;
    root.flexDirection = juce::FlexBox::Direction::row;
    root.flexWrap = juce::FlexBox::Wrap::noWrap;
    root.justifyContent = juce::FlexBox::JustifyContent::flexEnd;
    // root.justifyContent = juce::FlexBox::JustifyContent::center;
    root.alignItems = juce::FlexBox::AlignItems::center;

    const float gap = controlStripLayout::spacerSize;

    // root.items.add(juce::FlexItem(spacer0).withFlex(0,0).withWidth(gap).withHeight((float) area_height));
    root.items.add(juce::FlexItem(masterGroup).withFlex(1,0)
                                              .withMinWidth(masterGroup.getMinWidth())
                                              .withHeight((float) area_height));
    // root.items.add(juce::FlexItem(spacer1).withFlex(0,0).withWidth(gap).withHeight((float) area_height));

    root.performLayout(area.toFloat());
    masterGroup.resized();
}

void MasterVolumeComponent::setShowTitle(bool s)
{
    titleLabel.setVisible(s);
    resized();
}

void MasterVolumeComponent::setShowGroupLabels(bool s)
{
    masterGroup.setShowLabel(s);
    resized();
}

void MasterVolumeComponent::setShowKnobLabels(bool s)
{
    volume.setShowLabel(s);
    resized();
}

void MasterVolumeComponent::setShowTextBox(bool s)
{
    volume.setShowTextBox(s);
    resized();
}

void MasterVolumeComponent::setBackgroundColour(juce::Colour c)
{
    masterGroup.setBackgroundColour(c);
    repaint();
}

void MasterVolumeComponent::setCellBackgroundColour(juce::Colour c)
{
    masterGroup.setCellBackgroundColour(c);
    repaint();
}

void MasterVolumeComponent::setBackgroundStripColour(juce::Colour c)
{
    backgroundStripColour = c;
    repaint();
}