//
// Created by joseph larralde on 21/04/2026.
//

#include "SirenTrackComponent.h"

SirenTrackComponent::SirenTrackComponent(sirenId sid,
                                         juce::AudioProcessorValueTreeState& vts,
                                         const parameterLayoutGroupData& layoutGroupData) :
    sirenControls(vts, layoutGroupData)
{
    sirenControls.setBackgroundColour(juce::Colours::transparentBlack);

    addAndMakeVisible(spacer1);
    std::string title = sirenTitleById.at(sid)
                        + "\nch"
                        + std::to_string(sirenPropertiesById.at(sid)->oneBasedMidiChannel.oneBased);
    titleLabel.setText(title, juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel.setFont(juce::FontOptions(controlStripLayout::titleFontSize,
                       juce::Font::bold));
    // only listen to mouse events if we have a title area :
    titleLabel.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible(titleLabel);

    addAndMakeVisible(sirenControls);

    // Panning -------------------------------------------------------------
    // pan.setNameText(
    //    sirenControls.appendCCNumber("Pan", ParameterId::TrackPanning)
    // );
    pan.setNameText("Pan");
    juce::Slider& pans = pan.getSlider();
    pans.setLookAndFeel(&sirenControls.centredKnobLAF);
    pan.setSliderAttachment(vts,
                            ParameterId::TrackPanning,
                            layoutGroupData.id);
    sirenControls.setSliderFillColour(pan, juce::Colours::whitesmoke);
    trackGroup.addAndMakeVisible(pan);

    // Output gain ---------------------------------------------------------
    // outputGain.setNameText(
    //    appendCCNumber("Gain", ParameterId::TrackOutputGain)
    // );
    outputGain.setNameText("Gain");
    juce::Slider& outgains = outputGain.getSlider();
    outputGain.setSliderAttachment(vts,
                                   ParameterId::TrackOutputGain,
                                   layoutGroupData.id);
    sirenControls.setSliderFillColour(outputGain, juce::Colours::whitesmoke);
    trackGroup.addAndMakeVisible(outputGain);

    trackGroup.setTitleText("Master");
    trackGroup.setGap(controlStripLayout::spacerSize);
    trackGroup.setWrap(false);
    addAndMakeVisible(trackGroup);

    addAndMakeVisible(spacer2);
    addAndMakeVisible(selection);
}

SirenTrackComponent::~SirenTrackComponent()
{
    listeners.clear();
}

void SirenTrackComponent::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();

    area = area.removeFromRight(controlStripLayout::titleAreaWidth);
    area -= { controlStripLayout::spacerSize, 0 };
    g.setColour(backgroundColour);
    g.fillRoundedRectangle(area, controlStripLayout::cornerSize);

    // draw midi note playing monitor led
    float ledAreaWidth = 16;
    auto tr = area.getTopRight();
    auto ledBounds = juce::Rectangle<float>(
        tr.getX() - ledAreaWidth,
        tr.getY(),
        ledAreaWidth,
        ledAreaWidth
    );

    g.setColour(juce::Colours::darkgreen);
    g.fillEllipse(ledBounds.withSizeKeepingCentre(9, 9));

    if (isPlayingNote) {
        g.setColour(juce::Colours::lightgreen);
        g.fillEllipse(ledBounds.withSizeKeepingCentre(6,6));
    }
}

void SirenTrackComponent::resized()
{
    auto area = getLocalBounds().reduced(0);
    const int area_height = area.getHeight();

    juce::FlexBox root;
    root.flexDirection = juce::FlexBox::Direction::row;
    root.flexWrap = juce::FlexBox::Wrap::noWrap;
    root.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    root.alignItems = juce::FlexBox::AlignItems::center;

    const float gap = controlStripLayout::spacerSize;

    root.items.add(juce::FlexItem(sirenControls).withFlex(0,0)
                                            .withMinWidth(sirenControls.getMinWidth())
                                            .withHeight((float) area_height));

    root.items.add(juce::FlexItem(trackGroup).withFlex(0,0)
                                             .withMinWidth(trackGroup.getMinWidth())
                                             .withHeight((float) area_height));
    root.items.add(juce::FlexItem(spacer2).withFlex(0,0).withWidth(gap).withHeight((float) area_height));

    root.items.add(juce::FlexItem(titleLabel).withFlex(0,0)
                                             .withWidth(controlStripLayout::titleAreaWidth)
                                             .withHeight((float) area_height));
    root.items.add(juce::FlexItem(spacer1).withFlex(0,0).withWidth(gap).withHeight((float) area_height));

    root.performLayout(area.toFloat());

    trackGroup.resized();
    selection.setBounds(getLocalBounds());
    selection.resized();
}

void SirenTrackComponent::addListener(Listener* l)
{
    listeners.emplace(l);
}

void SirenTrackComponent::removeListener(Listener* l)
{
    if (listeners.contains(l)) { listeners.erase(l); }
}


float SirenTrackComponent::getMinWidth() const
{
    return getTitleWidth() +
           getTrackControlsWidth() +
           getSirenControlsWidth();
}

float SirenTrackComponent::getTitleWidth() const
{
    return controlStripLayout::spacerSize + controlStripLayout::titleAreaWidth;
}

float SirenTrackComponent::getTrackControlsWidth() const
{
    for (auto* c : getChildren()) {
        if (auto* group = dynamic_cast<GuiCellGroup*>(c)) {
            return group->getMinWidth() + controlStripLayout::spacerSize;
        }
    }

    return 0;
}

float SirenTrackComponent::getSirenControlsWidth() const
{
    return sirenControls.getMinWidth();
}

void SirenTrackComponent::setShowGroupLabels(bool s)
{
    trackGroup.setShowLabel(s);
    sirenControls.setShowGroupLabels(s);
    resized();
}

void SirenTrackComponent::setShowKnobLabels(bool s)
{
    outputGain.setShowLabel(s);
    pan.setShowLabel(s);
    sirenControls.setShowKnobLabels(s);
    resized();
}

void SirenTrackComponent::setShowTextBox(bool s)
{
    outputGain.setShowTextBox(s);
    pan.setShowTextBox(s);
    sirenControls.setShowTextBox(s);
    resized();
}

void SirenTrackComponent::setBackgroundColour(juce::Colour c)
{
    backgroundColour = c;
    trackGroup.setBackgroundColour(c);
    sirenControls.setBackgroundColour(c);
}

void SirenTrackComponent::setBackgroundStripColour(juce::Colour c)
{
    backgroundStripColour = c;
    repaint();
}

bool SirenTrackComponent::getSelected() const
{
    return selected;
}

void SirenTrackComponent::setSelected(bool s)
{
    selected = s;
    selection.enable(s);
    resized();
}

void SirenTrackComponent::onTitleLabelClicked()
{
    for (auto* l : listeners) { l->sirenTrackSelected(this); }
}

void SirenTrackComponent::setIsPlayingNote(bool isPlaying)
{
    isPlayingNote = isPlaying;
    repaint();
}
