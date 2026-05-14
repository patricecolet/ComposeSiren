/*
  ==============================================================================

    CMSMidiKeyboardComponent.h
    Created: 16 Aug 2021 11:03:34pm
    Author:  Joseph Larralde

  ==============================================================================
*/

#ifndef COMPOSESIREN_DBRANGES_MIDIKEYBOARDCOMPONENT_H
#define COMPOSESIREN_DBRANGES_MIDIKEYBOARDCOMPONENT_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../lib/definitions/sirenProperties.h"
#include "../lib/definitions/palette.h"
#include "../colourUtilities.h"
#include "LookAndFeels.h"
#include "VoiceManagerState.h"
#include "lib/wrappers/SirenEnsemble.h"
#include "lib/wrappers/SirenStateMonitor.h"

class MidiKeyboardLabelLookAndFeel : public juce::LookAndFeel_V3
{
public:
    MidiKeyboardLabelLookAndFeel()
    {
        setColour(juce::Label::textColourId, juce::Colours::whitesmoke);
    }
};

class SirenPitchesDisplayComponent : public juce::Component,
                                     public juce::Timer,
                                     public SirenStateMonitor::Listener
{
    std::vector<sirenId> sirenStateIds;
    std::optional<sirenId> fgSirenId{std::nullopt};
    std::vector<sirenId> sortedSirenStateIds;
    std::map<sirenId, SirenVoice::State> sirenStates{};

    juce::MidiKeyboardComponent& keyboardComponent;
    SirenStateMonitor& sirenStateMonitor;

public:
    SirenPitchesDisplayComponent(SirenStateMonitor& ssm,
                                 juce::MidiKeyboardComponent& kc) :
        keyboardComponent(kc),
        sirenStateMonitor(ssm)
    {
        sirenStateMonitor.addListener(this);
        const std::vector<sirenId>& ids = sirenStateMonitor.getActiveSirenIds();
        SirenPitchesDisplayComponent::activeSirenIds(ids);
        startTimer(33);
    }

    ~SirenPitchesDisplayComponent() override
    {
        sirenStateMonitor.removeListener(this);
        stopTimer();
    };

    void setForegroundSirenId(std::optional<sirenId> id = std::nullopt) {
        fgSirenId = id;
        sortedSirenStateIds = sirenStateIds;
        if (fgSirenId.has_value()) {
            auto last = sortedSirenStateIds.size() - 1;
            for (auto i = 0; i <= last; ++i) {
                if (sortedSirenStateIds[i] == fgSirenId.value() && i < last) {
                    std::swap(sortedSirenStateIds[i],
                              sortedSirenStateIds[last]);
                }
            }
        }
    }

    // SirenStateMonitor::Listener callbacks
    //--------------------------------------------------------------------------
    void activeSirenIds(const std::vector<sirenId>& ids) override {
        sirenStates.clear();
        sirenStateIds = ids;
        for (const auto id : ids) { sirenStates[id] = {}; }
        setForegroundSirenId(fgSirenId);
    }

    void currentSirenState(const sirenId id,
                           const SirenVoice::State& state) override {
        sirenStates[id] = state;
    }

    // juce::Timer callback ----------------------------------------------------
    void timerCallback() override {
        repaint();
    }

    // juce::Component methods
    //--------------------------------------------------------------------------
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();

        auto bg = bounds;
        bg.removeFromLeft(1); // align to midi keyboard
        g.setColour(juce::Colours::whitesmoke);
        g.fillRect(bg);

        float s = bounds.getHeight();
        float y = s * 0.5f;
        juce::Rectangle<float> area;

        bool hasFgSiren = fgSirenId.has_value();

        for (auto i = 0; i < sirenStates.size(); ++i) {
            const auto& id = sortedSirenStateIds[i];
            const auto& [ isNoteOn, currentPitch ] = sirenStates[id];
            float pos = getPositionFromFloatNote(currentPitch);
            area = area.withCentre({pos, y})
                       .withSizeKeepingCentre(s, s)
                       .reduced((hasFgSiren && i < sirenStates.size() - 1) ? 1 : 0);
            float alpha = isNoteOn ? 0.12f : 0.5f;
            g.setColour(sirenColourById.at(id));
            g.fillEllipse(area);
            g.setColour(juce::Colours::white.withAlpha(alpha));
            g.fillEllipse(area.reduced(2));
        }
    }

    void resized() override {}

private:
     static bool isWhiteNote(int midiNoteNumber) {
        switch (midiNoteNumber % 12) {
        case 0:
        case 2:
        case 4:
        case 5:
        case 7:
        case 9:
        case 11:
            return true;
        default:
            return false;
        }
    }

    float getPositionFromNote(int midiNoteNumber) const {
        float s = keyboardComponent.getKeyStartPosition(midiNoteNumber);
        float w = isWhiteNote(midiNoteNumber) ?
                  keyboardComponent.getKeyWidth() :
                  keyboardComponent.getBlackNoteWidth();
        return s + w * 0.5f;
    }

    float getPositionFromFloatNote(float midiNoteNumber) const {
         midiNoteNumber += 12;
         float lower = getPositionFromNote(std::floor(midiNoteNumber));
         float upper = getPositionFromNote(std::ceil(midiNoteNumber));
         float frac = midiNoteNumber - std::floor(midiNoteNumber);
         return lower + frac * (upper - lower);
    }
};

//==============================================================================
// customized MidiKeyboardComponent class
// owned by DbRangesMidiKeyboardComponent
//==============================================================================

class CustomMidiKeyboardComponent : public juce::MidiKeyboardComponent
{
public:
    CustomMidiKeyboardComponent(juce::MidiKeyboardState& s,
                                juce::MidiKeyboardComponent::Orientation o) :
        juce::MidiKeyboardComponent(s, o)
    {
        // setBlackNoteLengthProportion(1.0f);
        // setOctaveForMiddleC(4);
        setWantsKeyboardFocus(false);
    }

    ~CustomMidiKeyboardComponent() override = default;

    // TODO : override paint method for more control over final look and feel

    void drawBlackNote(int midiNoteNumber,
                       juce::Graphics& g,
                       juce::Rectangle<float> area,
                       bool isDown,
                       bool isOver,
                       const juce::Colour noteFillColour) override
    {
        juce::Colour c(noteFillColour);

        if (isDown)  c = c.overlaidWith(findColour(keyDownOverlayColourId));
        if (isOver)  c = c.overlaidWith(findColour(mouseOverKeyOverlayColourId));

        g.setColour(c);
        g.fillRect(area);

        g.setColour(noteFillColour);
        g.drawRect(area);
    }

    juce::Range<float> getKeyPosition(int midiNoteNumber) const
    {
        return juce::MidiKeyboardComponent::getKeyPosition(
            midiNoteNumber,
            getKeyWidth()
        );
    }

    juce::String getWhiteNoteText(int midiNoteNumber) override
    {
        if (midiNoteNumber % 12 == 0)
            return juce::MidiMessage::getMidiNoteName(
                midiNoteNumber,
                true,
                true,
                getOctaveForMiddleC() + 1
            );

        return {};
    }
};

//==============================================================================
// our DbRangesMidiKeyboardComponent class
//==============================================================================

class DbRangesMidiKeyboardComponent : public juce::Component,
                                      public VoiceManagerState::Listener
{
private:
    MidiKeyboardLookAndFeel mklaf;
    MidiKeyboardLabelLookAndFeel mkllaf;

    CustomMidiKeyboardComponent keyboard;
    SirenPitchesDisplayComponent sirenPitchesDisplay;
    VoiceManagerState& voiceManagerState;

    juce::Rectangle<int> dbRangesBounds;

    juce::Rectangle<int> lowDbRange;
    juce::Label lowDbRangeLabel;
    juce::Rectangle<int> midDbRange;
    juce::Label midDbRangeLabel;
    juce::Rectangle<int> highDbRange;
    juce::Label highDbRangeLabel;

    std::shared_ptr<sirenData> data;

public:
    DbRangesMidiKeyboardComponent(juce::MidiKeyboardState& s,
                                  VoiceManagerState& vms,
                                  SirenStateMonitor& ssm,
                                  const juce::String& name = "") :
        Component(name),
        keyboard(s, juce::MidiKeyboardComponent::horizontalKeyboard),
        sirenPitchesDisplay(ssm, keyboard),
        voiceManagerState(vms)
    {
        voiceManagerState.addListener(
            VoiceManagerState::Listener::Key::category,
            this
        );

        // keyboard.setEnabled(false);
        keyboard.setAvailableRange(24, 95);
        // keyboard.setAvailableRange(36, 107);
        keyboard.setBlackNoteLengthProportion(0.5f);
        keyboard.setScrollButtonsVisible(false);
        // keyboard.setScrollButtonWidth(0);
        keyboard.setLookAndFeel(&mklaf);
        addAndMakeVisible(keyboard);

        addAndMakeVisible(sirenPitchesDisplay);

        const juce::FontOptions f{10.0f, juce::Font::plain};

        lowDbRangeLabel.setText("VOL 1", juce::dontSendNotification);
        lowDbRangeLabel.setJustificationType(juce::Justification::centred);
        lowDbRangeLabel.setFont(f);
        lowDbRangeLabel.setLookAndFeel(&mkllaf);
        addAndMakeVisible(lowDbRangeLabel);

        midDbRangeLabel.setText("VOL 2", juce::dontSendNotification);
        midDbRangeLabel.setJustificationType(juce::Justification::centred);
        midDbRangeLabel.setFont(f);
        midDbRangeLabel.setLookAndFeel(&mkllaf);
        addAndMakeVisible(midDbRangeLabel);

        highDbRangeLabel.setText("VOL 3", juce::dontSendNotification);
        highDbRangeLabel.setJustificationType(juce::Justification::centred);
        highDbRangeLabel.setFont(f);
        highDbRangeLabel.setLookAndFeel(&mkllaf);
        addAndMakeVisible(highDbRangeLabel);

        setCurrentSirenCategory(voiceManagerState.getSirenCategory());
    }

    ~DbRangesMidiKeyboardComponent() override
    {
        voiceManagerState.removeListener(
            VoiceManagerState::Listener::Key::category,
            this
        );

        keyboard.setLookAndFeel(nullptr);
        lowDbRangeLabel.setLookAndFeel(nullptr);
        midDbRangeLabel.setLookAndFeel(nullptr);
        highDbRangeLabel.setLookAndFeel(nullptr);
    }

    // VoiceManagerState callback
    void categoryChanged(sirenCategory category) override
    {
        setCurrentSirenCategory(category);
    }

    void setCurrentSirenCategory(sirenCategory c)
    {
        data = sirenPropertiesByCategory.at(c);
        resized();
        repaint();
    }

    void setCurrentChannel(OneBasedMidiChannel midiChannel)
    {
        // careful, could throw an out_of_bounds exception if midiChannel is invalid
        if (sirenPropertiesByChannel.find(midiChannel) !=
            sirenPropertiesByChannel.end())
        {
            data = sirenPropertiesByChannel.at(midiChannel);
            sirenPitchesDisplay.setForegroundSirenId(data->id);
            keyboard.setMidiChannel(midiChannel.oneBased);
            resized();
            repaint();
        } else {
            // paint using default sirenId order;
            sirenPitchesDisplay.setForegroundSirenId();
        }
    }

    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colour(0x77ffffff));
        g.drawRect(dbRangesBounds.toFloat(), 1.0f);

        g.setColour(juce::Colour(mecaviv::Colours::MidiKeyboard::lowLevelRed));
        g.fillRect(lowDbRange);

        g.setColour(juce::Colour(mecaviv::Colours::MidiKeyboard::midLevelRed));
        g.fillRect(midDbRange);

        g.setColour(juce::Colour(mecaviv::Colours::MidiKeyboard::highLevelRed));
        g.fillRect(highDbRange);
    }

    void paintOverChildren(juce::Graphics& g) override
    {
        const juce::Rectangle<float>
        low = lowDbRange.toFloat(),
        mid = midDbRange.toFloat(),
        high = highDbRange.toFloat();

        g.setColour(juce::Colour(mecaviv::Colours::MidiKeyboard::dbRangeSeparatorBlue));
        const float top = static_cast<float>(keyboard.getBounds().getCentreY());
        g.drawLine(juce::Line<float>(juce::Point<float>(low.getX(), top), low.getBottomLeft()), 2.0f);
        g.drawLine(juce::Line<float>(juce::Point<float>(mid.getX(), top), mid.getBottomLeft()), 2.0f);
        g.drawLine(juce::Line<float>(juce::Point<float>(high.getX(), top), high.getBottomLeft()), 2.0f);
        g.drawLine(juce::Line<float>(juce::Point<float>(high.getRight(), top), high.getBottomRight()), 2.0f);
    }

    void resized() override
    {
        int xspacer = 2;
        int yspacer = 1;
        auto bounds = getLocalBounds().reduced(xspacer, yspacer);
        float sirenPitchesHeightRatio = 0.18f;
        float keyboardHeightRatio = 0.57f;//0.7f;
        float dbRangesRatio = 0.25f;
        int sirenPitchesHeight = bounds.getHeight() * sirenPitchesHeightRatio - yspacer;
        int keyboardHeight = bounds.getHeight() * keyboardHeightRatio - yspacer;
        int dbRangesHeight = bounds.getHeight() * dbRangesRatio - yspacer;

        auto sirenPitchesBounds = bounds;
        sirenPitchesBounds.setHeight(sirenPitchesHeight);
        sirenPitchesBounds.setLeft(sirenPitchesBounds.getX() - 1);
        sirenPitchesDisplay.setBounds(sirenPitchesBounds);

        auto keyboardBounds = bounds.withTop(sirenPitchesBounds.getBottom() + 2 * yspacer);
        keyboardBounds.setHeight(keyboardHeight);
        keyboardBounds.setLeft(keyboardBounds.getX() - 1);
        keyboardBounds.setBottom(keyboardBounds.getBottom() + 1);
        keyboard.setBounds(keyboardBounds);
        keyboard.setKeyWidth(keyboardBounds.getWidth() / 42.0f);

        dbRangesBounds = bounds;
        dbRangesBounds.setHeight(dbRangesHeight);
        dbRangesBounds = dbRangesBounds.withBottomY(bounds.getBottom());

        juce::Rectangle<int>* ranges[] = { &lowDbRange, &midDbRange, &highDbRange };
        juce::Label* labels[] = { &lowDbRangeLabel, &midDbRangeLabel, &highDbRangeLabel };

        for (auto i = 0; i < 3; ++i)
        {
            *(ranges[i]) = dbRangesBounds;
            auto minMaxNote = data->velocityRanges[i];
            auto [ min, max ] = minMaxNote;

            ranges[i]->setLeft(keyboard.getKeyPosition(min - 12).getStart() + xspacer);
            ranges[i]->setRight(keyboard.getKeyPosition(max - 12).getEnd() + xspacer);
            ranges[i]->reduce(0,1);
            labels[i]->setBounds(*ranges[i]);
        }
    }
};

#endif  // COMPOSESIREN_DBRANGES_MIDIKEYBOARDCOMPONENT_H
