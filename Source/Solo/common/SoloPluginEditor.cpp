/*
  ==============================================================================

    SoloPluginEditor.cpp
    ComposeSiren Solo Plugin (modèle paramétrable)

  ==============================================================================
*/

#include "SoloPluginProcessor.h"
#include "SoloPluginEditor.h"

namespace
{
    juce::Colour getGroupColour(const juce::String& groupName)
    {
        if (groupName == "Vibrato")   return juce::Colour(0xff4a90e2);
        if (groupName == "Tremolo")   return juce::Colour(0xffe28a4a);
        if (groupName == "Enveloppe") return juce::Colour(0xff8a4ae2);
        if (groupName == "Portamento")return juce::Colour(0xff4ae28a);
        return juce::Colour(0xff4a90e2);
    }
}

//==============================================================================
SoloPluginAudioProcessorEditor::SoloPluginAudioProcessorEditor (SoloPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      lastNoteOn(false), lastNote(0), lastVelocity(0)
{
    setSize (720, 580);

    // === Indicateurs MIDI ===
    noteLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    noteLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(noteLabel);

    velocityLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    velocityLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(velocityLabel);

    ambitusLabel.setFont(juce::Font(13.0f));
    ambitusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff888888));
    ambitusLabel.setJustificationType(juce::Justification::centred);
    ambitusLabel.setText(buildAmbitusText(), juce::dontSendNotification);
    addAndMakeVisible(ambitusLabel);

    // === Sections ===
    auto initGroup = [this](juce::GroupComponent& group, const juce::String& title)
    {
        group.setText(title);
        group.setTextLabelPosition(juce::Justification::centredTop);
        auto colour = getGroupColour(title);
        group.setColour(juce::GroupComponent::outlineColourId, colour);
        group.setColour(juce::GroupComponent::textColourId, colour);
        addAndMakeVisible(group);
    };

    initGroup(vibratoGroup,   "Vibrato");
    initGroup(tremoloGroup,   "Tremolo");
    initGroup(envelopeGroup,  "Enveloppe");
    initGroup(portamentoGroup,"Portamento");

    createKnob(vibratoDepthKnob,  vibratoDepthLabel,  "Depth",   1);
    createKnob(vibratoRateKnob,   vibratoRateLabel,   "Rate",    9);
    createKnob(vibratoAttackKnob, vibratoAttackLabel, "Attack", 11);

    createKnob(tremoloRateKnob,   tremoloRateLabel,   "Rate",   15);
    createKnob(tremoloDepthKnob,  tremoloDepthLabel,  "Depth",  92);

    createKnob(attackKnob,        attackLabel,        "Attack", 73);
    createKnob(releaseKnob,       releaseLabel,       "Release",72);

    createKnob(portamentoKnob,    portamentoLabel,    "Vitesse",5);

    startTimerHz(30);
}

SoloPluginAudioProcessorEditor::~SoloPluginAudioProcessorEditor()
{
    stopTimer();
}

void SoloPluginAudioProcessorEditor::createKnob(juce::Slider& slider, juce::Label& label,
                                                const juce::String& name, int ccNumber)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    slider.setRange(0.0, 127.0, 1.0);
    slider.setValue(0.0);
    slider.setTextValueSuffix(" CC" + juce::String(ccNumber));
    slider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    slider.addListener(this);
    addAndMakeVisible(slider);

    label.setText(name, juce::dontSendNotification);
    label.setFont(juce::Font(13.0f));
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(label);
}

//==============================================================================
void SoloPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2a2a2a));

    g.setColour(juce::Colour(0xff404040));
    g.fillRect(0, 0, getWidth(), 100);

    auto title = juce::String("ComposeSiren ") + SoloConfig::getDisplayName();
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(32.0f, juce::Font::bold));
    g.drawText(title, 20, 10, getWidth() - 40, 35, juce::Justification::centred);

    g.setFont(juce::Font(18.0f));
    g.setColour(juce::Colour(0xffaaaaaa));
    g.drawText(SoloConfig::getModel(), 20, 45, getWidth() - 40, 22, juce::Justification::centred);

    g.setColour(lastNoteOn ? juce::Colours::green : juce::Colour(0xff333333));
    g.fillEllipse(20, 80, 15, 15);
}

void SoloPluginAudioProcessorEditor::resized()
{
    const int margin = 15;
    const int headerHeight = 100;
    const int groupHeight = 220;
    const int groupSpacing = 10;
    const int groupWidth = (getWidth() - 2 * margin - groupSpacing) / 2;

    noteLabel.setBounds(50, 78, 150, 20);
    velocityLabel.setBounds(220, 78, 150, 20);
    ambitusLabel.setBounds(margin, 67, getWidth() - 2 * margin, 15);

    int yPos = headerHeight + 10;

    vibratoGroup.setBounds(margin, yPos, groupWidth, groupHeight);
    tremoloGroup.setBounds(margin + groupWidth + groupSpacing, yPos, groupWidth, groupHeight);

    auto layoutKnobsRow = [](juce::Rectangle<int> content, int numKnobs,
                             juce::Label* labels[], juce::Slider* sliders[])
    {
        const int spacing = (numKnobs > 1) ? 10 : 0;
        int knobWidth = (content.getWidth() - (numKnobs - 1) * spacing) / numKnobs;

        for (int i = 0; i < numKnobs; ++i)
        {
            auto knobArea = content.removeFromLeft(knobWidth);
            labels[i]->setBounds(knobArea.removeFromTop(20));
            sliders[i]->setBounds(knobArea.removeFromTop(100));
            if (numKnobs > 1 && i < numKnobs - 1)
                content.removeFromLeft(spacing);
        }
    };

    {
        juce::Label* labels[]  = { &vibratoDepthLabel, &vibratoRateLabel, &vibratoAttackLabel };
        juce::Slider* sliders[] = { &vibratoDepthKnob, &vibratoRateKnob, &vibratoAttackKnob };
        layoutKnobsRow(vibratoGroup.getBounds().reduced(10, 25), 3, labels, sliders);
    }

    {
        juce::Label* labels[]  = { &tremoloRateLabel, &tremoloDepthLabel };
        juce::Slider* sliders[] = { &tremoloRateKnob, &tremoloDepthKnob };
        layoutKnobsRow(tremoloGroup.getBounds().reduced(10, 25), 2, labels, sliders);
    }

    yPos += groupHeight + groupSpacing;

    envelopeGroup.setBounds(margin, yPos, groupWidth, groupHeight);
    portamentoGroup.setBounds(margin + groupWidth + groupSpacing, yPos, groupWidth, groupHeight);

    {
        juce::Label* labels[]  = { &attackLabel, &releaseLabel };
        juce::Slider* sliders[] = { &attackKnob, &releaseKnob };
        layoutKnobsRow(envelopeGroup.getBounds().reduced(10, 25), 2, labels, sliders);
    }

    auto portamentoBounds = portamentoGroup.getBounds().reduced(10, 25);
    portamentoLabel.setBounds(portamentoBounds.removeFromTop(20));
    portamentoKnob.setBounds(portamentoBounds.withSizeKeepingCentre(120, 120));
}

void SoloPluginAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    int ccNumber = -1;
    int ccValue = (int)slider->getValue();
    
    if (slider == &vibratoDepthKnob)      ccNumber = 1;
    else if (slider == &vibratoRateKnob)  ccNumber = 9;
    else if (slider == &vibratoAttackKnob)ccNumber = 11;
    else if (slider == &tremoloRateKnob)  ccNumber = 15;
    else if (slider == &tremoloDepthKnob) ccNumber = 92;
    else if (slider == &attackKnob)       ccNumber = 73;
    else if (slider == &releaseKnob)      ccNumber = 72;
    else if (slider == &portamentoKnob)   ccNumber = 5;
    
    if (ccNumber >= 0)
    {
        if (auto* midiHandler = audioProcessor.getMidiHandler())
            midiHandler->HandleControlChange(audioProcessor.getSoloChannel(), ccNumber, ccValue);
    }
}

void SoloPluginAudioProcessorEditor::timerCallback()
{
    auto midiState = audioProcessor.getMidiState();
    
    if (midiState.noteOn)
    {
        noteLabel.setText("Note: " + formatMidiNote(midiState.currentNote), juce::dontSendNotification);
        velocityLabel.setText("Vel: " + juce::String(midiState.currentVelocity), juce::dontSendNotification);
    }
    else
    {
        noteLabel.setText("Note: ---", juce::dontSendNotification);
        velocityLabel.setText("Vel: ---", juce::dontSendNotification);
    }
    
    const auto& activeCC = midiState.activeCC;
    if (! activeCC.empty())
    {
        for (const auto& pair : activeCC)
        {
            juce::Slider* targetKnob = nullptr;
            switch (pair.first)
            {
                case 1:  targetKnob = &vibratoDepthKnob;  break;
                case 9:  targetKnob = &vibratoRateKnob;   break;
                case 11: targetKnob = &vibratoAttackKnob; break;
                case 15: targetKnob = &tremoloRateKnob;   break;
                case 92: targetKnob = &tremoloDepthKnob;  break;
                case 73: targetKnob = &attackKnob;        break;
                case 72: targetKnob = &releaseKnob;       break;
                case 5:  targetKnob = &portamentoKnob;    break;
                default: break;
            }

            if (targetKnob != nullptr && (int)targetKnob->getValue() != pair.second)
                targetKnob->setValue(pair.second, juce::dontSendNotification);
        }
    }

    if (midiState.noteOn != lastNoteOn)
    {
        lastNoteOn = midiState.noteOn;
        repaint();
    }

    lastNote = midiState.currentNote;
    lastVelocity = midiState.currentVelocity;
    lastCC = midiState.activeCC;
}

juce::String SoloPluginAudioProcessorEditor::formatMidiNote (int midiNote) const
{
    return juce::String(midiNote) + " (" + juce::MidiMessage::getMidiNoteName(midiNote, true, true, 3) + ")";
}

juce::String SoloPluginAudioProcessorEditor::buildAmbitusText() const
{
    int noteMin = 24;
    int noteMax = 72;
    SoloConfig::getAmbitus(noteMin, noteMax);
    return "Ambitus: " + formatMidiNote(noteMin) + " - " + formatMidiNote(noteMax);
}
