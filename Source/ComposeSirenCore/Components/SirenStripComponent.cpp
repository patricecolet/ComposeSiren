//
// Created by joseph larralde on 26/01/2026.
//

#include "SirenStripComponent.h"

//==============================================================================
// ReverbComponent - Section de reverb
SirenStripComponent::SirenStripComponent(juce::AudioProcessorValueTreeState& vts,
                                         const std::string& paramGroupId) :
    apvts(vts),
    currentCategory(Alto)
{
    auto appendCCNumber = [&](const std::string& s, const ParameterId id) -> std::string {
        if (auto cc = std::get_if<CCParam>(&parameterDefinitionById.at(id)->data)) {
            return s + std::string("\ncc") + std::to_string(cc->midiCCNumber) + "";
        }
        return s;
    };

    // Titre
    // titleLabel.setText("Siren", juce::dontSendNotification);
    // titleLabel.setJustificationType(juce::Justification::centred);
    // titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    // titleLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    // addAndMakeVisible(titleLabel);

    const float gap = controlStripLayout::spacerSize;

    // Groups + spacers
    startGroup.setTitleText("Pitch");
    // startGroup.setTitleText(" ");
    startGroup.setGap(gap);
    startGroup.setWrap(false);
    addAndMakeVisible(startGroup);

    vibGroup.setTitleText("Vibrato");
    vibGroup.setGap(gap);
    vibGroup.setWrap(false);
    addAndMakeVisible(vibGroup);

    tremGroup.setTitleText("Tremolo");
    tremGroup.setGap(gap);
    tremGroup.setWrap(false);
    addAndMakeVisible(tremGroup);

    envGroup.setTitleText("Envelope");
    envGroup.setGap(gap);
    envGroup.setWrap(false);
    addAndMakeVisible(envGroup);

    endGroup.setTitleText(" ");
    endGroup.setGap(gap);
    endGroup.setWrap(false);
    addAndMakeVisible(endGroup);

    // masterGroup.setTitleText("Master");
    // masterGroup.setGap(NBGAP);
    // masterGroup.setWrap(false);
    // addAndMakeVisible(masterGroup);

    addAndMakeVisible(spacer1);
    addAndMakeVisible(spacer2);
    addAndMakeVisible(spacer3);
    addAndMakeVisible(spacer4);
    addAndMakeVisible(spacer5);
    addAndMakeVisible(spacer6);
    // addAndMakeVisible(spacer7);

    juce::Colour c1 = juce::Colours::whitesmoke;//juce::Colours::lightgoldenrodyellow;
    juce::Colour c2 = juce::Colours::whitesmoke;
    juce::Colour c3 = c1;
    juce::Colour c4 = c2;
    juce::Colour c5 = c3;

    auto setSliderFillColour = [&](SliderCell& sc, juce::Colour c) {
        sc.getSlider().setColour(juce::Slider::ColourIds::rotarySliderFillColourId, c);
    };

    // start group knobs =======================================================

    // Pitch Bend --------------------------------------------------------------
    pitchBend.setNameText("Bend");
    pitchBend.setSliderAttachment(vts,
                                  ParameterId::PitchBend,
                                  paramGroupId);
    juce::Slider& pbs = pitchBend.getSlider();
    pbs.setLookAndFeel(&knobLAF3);
    // pbs.addListener(this);
    setSliderFillColour(pitchBend, c1);
    startGroup.addAndMakeVisible(pitchBend);

    // Pitch Bend Range --------------------------------------------------------
    pitchBendRange.setNameText(
        appendCCNumber("Bend Range", ParameterId::PitchBendRange)
    );
    // pitchBendRange.setNameText("Bend Range");
    // pitchBendRange.setRange(1, 36, 1);
    pitchBendRange.setSliderAttachment(vts,
                                       ParameterId::PitchBendRange,
                                       paramGroupId);
    juce::Slider& pbrs = pitchBendRange.getSlider();
    pbrs.setSliderStyle(juce::Slider::IncDecButtons);
    pbrs.setIncDecButtonsMode(juce::Slider::incDecButtonsNotDraggable);
    pbrs.setTextBoxStyle(juce::Slider::TextBoxRight, true, 50, 20);
    pbrs.setNumDecimalPlacesToDisplay(0);
    pbrs.setTextBoxIsEditable(true);
    pbrs.setLookAndFeel(&incDecLAF);
    // pbrs.addListener(this);
    setSliderFillColour(pitchBendRange, c1);
    startGroup.addAndMakeVisible(pitchBendRange);

    // Transpose ---------------------------------------------------------------
    transpose.setNameText("Transpose");
    // transpose.setRange(-24, 24, 1);
    transpose.setSliderAttachment(vts,
                                  ParameterId::Transpose,
                                  paramGroupId);
    juce::Slider& ts = transpose.getSlider();
    ts.setSliderStyle(juce::Slider::IncDecButtons);
    ts.setIncDecButtonsMode(juce::Slider::incDecButtonsNotDraggable);
    ts.setTextBoxStyle(juce::Slider::TextBoxRight, true, 50, 20);
    ts.setNumDecimalPlacesToDisplay(0);
    ts.setTextBoxIsEditable(true);
    ts.setLookAndFeel(&incDecLAF);
    ts.addListener(this);
    setSliderFillColour(transpose, c1);
    startGroup.addAndMakeVisible(transpose);

    // Portamento --------------------------------------------------------------
    portamento.setNameText(
        appendCCNumber("Porta", ParameterId::Portamento)
    );
    // portamento.setNameText("Porta");
    portamento.setSliderAttachment(vts,
                                   ParameterId::Portamento,
                                   paramGroupId);
    // portamento.setRange(0.0, 1.0, 0.01);
    // portamento.getSlider().addListener(this);
    setSliderFillColour(portamento, c1);
    startGroup.addAndMakeVisible(portamento);

    // Vib group knobs =========================================================

    // Speed -------------------------------------------------------------------
    vibSpeed.setNameText(
        appendCCNumber("Speed", ParameterId::VibratoFrequency)
    );
    // vibSpeed.setNameText("Speed");
    vibSpeed.setSliderAttachment(vts,
                                 ParameterId::VibratoFrequency,
                                 paramGroupId);
    // vibSpeed.setRange(0.0, 1.0, 0.01);
    // vibSpeed.getSlider().addListener(this);
    setSliderFillColour(vibSpeed, c2);
    vibGroup.addAndMakeVisible(vibSpeed);

    // Depth -------------------------------------------------------------------
    vibDepth.setNameText(
       appendCCNumber("Depth", ParameterId::VibratoAmplitude)
    );
    // vibDepth.setNameText("Depth");
    vibDepth.setSliderAttachment(vts,
                                 ParameterId::VibratoAmplitude,
                                 paramGroupId);
    // vibDepth.setRange(0.0, 1.0, 0.01);
    // vibDepth.getSlider().addListener(this);
    setSliderFillColour(vibDepth, c2);
    vibGroup.addAndMakeVisible(vibDepth);

    // Evolve ------------------------------------------------------------------
    // Propagate ? or Evolve ? Drift ? Envelope ? Progress ? Rise ?
    vibAccel.setNameText(
       appendCCNumber("Evolve", ParameterId::VibratoAcceleration)
    );
    // vibAccel.setNameText("Evolve");
    vibAccel.setSliderAttachment(vts,
                                 ParameterId::VibratoAcceleration,
                                 paramGroupId);
    // vibAccel.setRange(0.0, 1.0, 0.01);
    // vibAccel.getSlider().addListener(this);
    setSliderFillColour(vibAccel, c2);
    vibGroup.addAndMakeVisible(vibAccel);

    // Trem group knobs ========================================================

    // Speed -------------------------------------------------------------------
    tremSpeed.setNameText(
       appendCCNumber("Speed", ParameterId::TremoloFrequency)
    );
    // tremSpeed.setNameText("Speed");
    tremSpeed.setSliderAttachment(vts,
                                  ParameterId::TremoloFrequency,
                                  paramGroupId);
    // tremSpeed.setRange(0.0, 1.0, 0.01);
    // tremSpeed.getSlider().addListener(this);
    setSliderFillColour(tremSpeed, c3);
    tremGroup.addAndMakeVisible(tremSpeed);

    // Depth -------------------------------------------------------------------
    tremDepth.setNameText(
       appendCCNumber("Depth", ParameterId::TremoloAmplitude)
    );
    // tremDepth.setNameText("Depth");
    tremDepth.setSliderAttachment(vts,
                                  ParameterId::TremoloAmplitude,
                                  paramGroupId);
    // tremDepth.setRange(0.0, 1.0, 0.01);
    // tremDepth.getSlider().addListener(this);
    setSliderFillColour(tremDepth, c3);
    tremGroup.addAndMakeVisible(tremDepth);

    // Env group knobs =========================================================

    // Attack ------------------------------------------------------------------
    envAttack.setNameText(
       appendCCNumber("Attack", ParameterId::AttackDuration)
    );
    // envAttack.setNameText("Attack");
    envAttack.setSliderAttachment(vts,
                                  ParameterId::AttackDuration,
                                  paramGroupId);
    // envAttack.setRange(0.0, 1.0, 0.01);
    // envAttack.getSlider().addListener(this);
    setSliderFillColour(envAttack, c4);
    envGroup.addAndMakeVisible(envAttack);

    // Release -----------------------------------------------------------------
    envRelease.setNameText(
       appendCCNumber("Release", ParameterId::ReleaseDuration)
    );
    // envRelease.setNameText("Release");
    envRelease.setSliderAttachment(vts,
                                   ParameterId::ReleaseDuration,
                                   paramGroupId);
    // envRelease.setRange(0.0, 1.0, 0.01);
    // envRelease.getSlider().addListener(this);
    setSliderFillColour(envRelease, c4);
    envGroup.addAndMakeVisible(envRelease);

    // End group knobs =========================================================

    // Timbre ------------------------------------------------------------------
    timbre.setNameText(
       appendCCNumber("Timbre", ParameterId::Timbre)
    );
    // timbre.setNameText("Timbre");
    timbre.setSliderAttachment(vts,
                               ParameterId::Timbre,
                               paramGroupId);
    // timbre.setRange(0.0, 1.0, 0.01);
    // timbre.getSlider().addListener(this);
    setSliderFillColour(timbre, c5);
    endGroup.addAndMakeVisible(timbre);

    // Mute --------------------------------------------------------------------
    mute.setNameText(
       appendCCNumber("Mute", ParameterId::Mute)
    );
    // mute.setNameText("Mute");
    mute.setSliderAttachment(vts,
                             ParameterId::Mute,
                             paramGroupId);
    // mute.setRange(0.0, 1.0, 0.01);
    // mute.getSlider().addListener(this);
    setSliderFillColour(mute, c5);
    endGroup.addAndMakeVisible(mute);

    // Volume ------------------------------------------------------------------
    volume.setNameText(
       appendCCNumber("Volume", ParameterId::Volume)
    );
    // volume.setNameText("Volume");
    volume.setSliderAttachment(vts,
                               ParameterId::Volume,
                               paramGroupId);
    // volume.setRange(0.0, 1.0, 0.01);
    // volume.getSlider().addListener(this);
    setSliderFillColour(volume, c5);
    endGroup.addAndMakeVisible(volume);

    // Démarrer le timer pour synchroniser l'UI avec les changements MIDI
    // startTimer(50); // 50ms = 20 Hz
}

SirenStripComponent::~SirenStripComponent()
{
    stopTimer();
}

void SirenStripComponent::setCurrentSiren(sirenCategory c)
{
    currentCategory = c;
}

void SirenStripComponent::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().reduced(2).toFloat();

    // g.setColour(backgroundColour);
    g.setColour(juce::Colour{0xff314159});
    g.fillRoundedRectangle(area, 12);

    // This draws some kind of minimalistic 19" rack ears ----------------------

    /*
    float holeRadius = 5;
    float holeDiameter = 2 * holeRadius;
    float holeDistance = 7;
    float wholeDistance = holeDistance + holeRadius;

    juce::Point tl = {
        area.getX() + wholeDistance,
        area.getY() + wholeDistance
    };
    juce::Point tr = {
        area.getRight() - wholeDistance,
        area.getY() + wholeDistance
    };
    juce::Point bl = {
        area.getX() + wholeDistance,
        area.getBottom() - wholeDistance
    };
    juce::Point br = {
        area.getRight() - wholeDistance,
        area.getBottom() - wholeDistance
    };

    g.setColour(juce::Colour{0xff222f3e});
    g.fillEllipse(tl.x - holeRadius, tl.y - holeRadius, holeDiameter, holeDiameter);
    g.fillEllipse(tr.x - holeRadius, tr.y - holeRadius, holeDiameter, holeDiameter);
    g.fillEllipse(bl.x - holeRadius, bl.y - holeRadius, holeDiameter, holeDiameter);
    g.fillEllipse(br.x - holeRadius, br.y - holeRadius, holeDiameter, holeDiameter);
    //*/
}

void SirenStripComponent::resized()
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
    root.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    root.alignItems = juce::FlexBox::AlignItems::center;

    const float spacerW = 2.0f;
    const float gap = controlStripLayout::spacerSize;

    // NB : withMinWidth is 52 for 1, 103 for 2, 154 for 3 etc
    root.items.add(juce::FlexItem(spacer1).withFlex(0,0).withWidth(gap).withHeight((float) area_height));
    root.items.add(juce::FlexItem(startGroup).withFlex(0,0)
                                             .withMinWidth(startGroup.getMinWidth())
                                             .withHeight((float) area_height));
    root.items.add(juce::FlexItem(spacer2).withFlex(0,0).withWidth(gap).withHeight((float) area_height));
    // root.items.add(juce::FlexItem(vibGroup).withFlex(0,0).withMinWidth(160.0f).withHeight((float) area_height));
    root.items.add(juce::FlexItem(vibGroup).withFlex(0,0)
                                             .withMinWidth(vibGroup.getMinWidth())
                                             .withHeight((float) area_height));
    root.items.add(juce::FlexItem(spacer3).withFlex(0,0).withWidth(gap).withHeight((float) area_height));
    // root.items.add(juce::FlexItem(tremGroup).withFlex(0,0).withMinWidth(108.0f).withHeight((float) area_height));
    root.items.add(juce::FlexItem(tremGroup).withFlex(0,0)
                                             .withMinWidth(tremGroup.getMinWidth())
                                             .withHeight((float) area_height));
    root.items.add(juce::FlexItem(spacer4).withFlex(0,0).withWidth(gap).withHeight((float) area_height));
    // root.items.add(juce::FlexItem(envGroup).withFlex(0,0).withMinWidth(108.0f).withHeight((float) area_height));
    root.items.add(juce::FlexItem(envGroup).withFlex(0,0)
                                             .withMinWidth(envGroup.getMinWidth())
                                             .withHeight((float) area_height));
    root.items.add(juce::FlexItem(spacer5).withFlex(0,0).withWidth(gap).withHeight((float) area_height));
    // root.items.add(juce::FlexItem(endGroup).withFlex(0,0).withMinWidth(160.0f).withHeight((float) area_height));
    root.items.add(juce::FlexItem(endGroup).withFlex(0,0)
                                             .withMinWidth(endGroup.getMinWidth())
                                             .withHeight((float) area_height));
    root.items.add(juce::FlexItem(spacer6).withFlex(0,0).withWidth(gap).withHeight((float) area_height));
    // root.items.add(juce::FlexItem(masterGroup).withFlex(0).withMinWidth(103.0f).withHeight((float) area_height));
    // root.items.add(juce::FlexItem(spacer7).withFlex(0,0).withWidth(spacerW).withHeight((float) area_height));

    root.performLayout(area.toFloat());

    startGroup.resized();
    vibGroup.resized();
    tremGroup.resized();
    envGroup.resized();
    endGroup.resized();
    // masterGroup.resized();
}

void SirenStripComponent::sliderValueChanged(juce::Slider* slider)
{
    if (portamento.compareSlider(slider))
    {
        //std::cout << "coucou !" <<std::endl;
    }
    else if (vibSpeed.compareSlider(slider))
    {
        //std::cout << "yeeeha !" <<std::endl;
    }
    /*
    if (slider == &roomSizeSlider)
    {
        audioProcessor.mySynth->reverb.setroomsize((float)roomSizeSlider.getValue());
    }
    else if (slider == &wetSlider)
    {
        // Dry/Wet : 0=100% dry, 0.5=50/50, 1=100% wet
        float dryWetValue = (float)wetSlider.getValue();
        audioProcessor.mySynth->reverb.setwet(dryWetValue);
        audioProcessor.mySynth->reverb.setdry(1.0f - dryWetValue);
    }
    else if (slider == &dampSlider)
    {
        audioProcessor.mySynth->reverb.setdamp((float)dampSlider.getValue());
    }
    else if (slider == &widthSlider)
    {
        audioProcessor.mySynth->reverb.setwidth((float)widthSlider.getValue());
    }
    else if (slider == &highpassSlider)
    {
        audioProcessor.mySynth->setReverbHighpass((float)highpassSlider.getValue());
    }
    else if (slider == &lowpassSlider)
    {
        audioProcessor.mySynth->setReverbLowpass((float)lowpassSlider.getValue());
    }
    //*/
}

void SirenStripComponent::buttonClicked(juce::Button* button)
{
    /*
    if (button == &enableButton)
    {
        audioProcessor.mySynth->setReverbEnabled(enableButton.getToggleState());
    }
    //*/
}

float SirenStripComponent::getMinWidth()
{
    float mw = 0.0f;

    for (auto* c : getChildren()) {
        if (auto* group = dynamic_cast<SliderCellGroup*>(c)) {
            mw += group->getMinWidth();
        } else if (auto* spacer = dynamic_cast<Spacer*>(c)) {
            mw += controlStripLayout::spacerSize;
        }
    }

    return mw;
}

void SirenStripComponent::setShowTitle(bool s)
{
    showTitle = s;
    titleLabel.setVisible(s);
    resized();
}

void SirenStripComponent::setShowGroupLabels(bool s)
{
    showGroupLabels = s;
    startGroup.setShowLabel(s);
    vibGroup.setShowLabel(s);
    tremGroup.setShowLabel(s);
    envGroup.setShowLabel(s);
    endGroup.setShowLabel(s);
    // masterGroup.setShowLabel(s);
    resized();
}

void SirenStripComponent::setShowKnobLabels(bool s)
{
    showKnobLabels = s;
    pitchBend.setShowLabel(s);
    pitchBendRange.setShowLabel(s);
    transpose.setShowLabel(s);
    portamento.setShowLabel(s);
    vibSpeed.setShowLabel(s);
    vibDepth.setShowLabel(s);
    vibAccel.setShowLabel(s);
    tremSpeed.setShowLabel(s);
    tremDepth.setShowLabel(s);
    envAttack.setShowLabel(s);
    envRelease.setShowLabel(s);
    timbre.setShowLabel(s);
    volume.setShowLabel(s);
    mute.setShowLabel(s);
    // masterVolume.setShowLabel(s);
    // pan.setShowLabel(s);
    resized();
}

void SirenStripComponent::setShowTextBox(bool s)
{
    pitchBend.setShowLabel(s);
    // always show values for inc dec sliders
    pitchBendRange.setShowLabel(true);
    transpose.setShowLabel(true);
    portamento.setShowTextBox(s);
    vibSpeed.setShowTextBox(s);
    vibDepth.setShowTextBox(s);
    vibAccel.setShowTextBox(s);
    tremSpeed.setShowTextBox(s);
    tremDepth.setShowTextBox(s);
    envAttack.setShowTextBox(s);
    envRelease.setShowTextBox(s);
    timbre.setShowTextBox(s);
    volume.setShowTextBox(s);
    mute.setShowTextBox(s);

    // masterVolume.setShowTextBox(false);
    // pan.setShowTextBox(s);
    resized();
}

void SirenStripComponent::setBackgroundColour(juce::Colour c)
{
    backgroundColour = c;
    startGroup.setBackgroundColour(c);
    vibGroup.setBackgroundColour(c);
    tremGroup.setBackgroundColour(c);
    envGroup.setBackgroundColour(c);
    endGroup.setBackgroundColour(c);
    // masterGroup.setBackgroundColour(c);
    repaint();
}

void SirenStripComponent::timerCallback()
{
    /*
    // Synchroniser l'état Enable
    bool currentEnabled = audioProcessor.mySynth->isReverbEnabled();
    if (enableButton.getToggleState() != currentEnabled)
    {
        enableButton.setToggleState(currentEnabled, juce::dontSendNotification);
    }

    // Synchroniser Room Size (CC65)
    float currentRoomSize = audioProcessor.mySynth->reverb.getroomsize();
    if (std::abs(roomSizeSlider.getValue() - currentRoomSize) > 0.01)
    {
        roomSizeSlider.setValue(currentRoomSize, juce::dontSendNotification);
    }

    // Synchroniser Dry/Wet (CC66)
    // On synchronise directement avec wet (qui varie de 0 à 1)
    float currentWet = audioProcessor.mySynth->reverb.getwet();
    if (std::abs(wetSlider.getValue() - currentWet) > 0.02)  // Seuil augmenté pour éviter l'oscillation
    {
        wetSlider.setValue(currentWet, juce::dontSendNotification);
    }

    // Synchroniser Damp (CC67)
    float currentDamp = audioProcessor.mySynth->reverb.getdamp();
    if (std::abs(dampSlider.getValue() - currentDamp) > 0.01)
    {
        dampSlider.setValue(currentDamp, juce::dontSendNotification);
    }

    // Synchroniser Width (CC70)
    float currentWidth = audioProcessor.mySynth->reverb.getwidth();
    if (std::abs(widthSlider.getValue() - currentWidth) > 0.01)
    {
        widthSlider.setValue(currentWidth, juce::dontSendNotification);
    }

    // Synchroniser Highpass (CC68)
    float currentHighpass = audioProcessor.mySynth->getReverbHighpass();
    if (std::abs(highpassSlider.getValue() - currentHighpass) > 1.0)
    {
        highpassSlider.setValue(currentHighpass, juce::dontSendNotification);
    }

    // Synchroniser Lowpass (CC69)
    float currentLowpass = audioProcessor.mySynth->getReverbLowpass();
    if (std::abs(lowpassSlider.getValue() - currentLowpass) > 1.0)
    {
        lowpassSlider.setValue(currentLowpass, juce::dontSendNotification);
    }
    //*/
}
