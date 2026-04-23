//
// Created by joseph larralde on 26/01/2026.
//

#ifndef COMPOSESIREN_LOOKANDFEELS_H
#define COMPOSESIREN_LOOKANDFEELS_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "BinaryData.h"

// LAYOUT VARS /////////////////////////////////////////////////////////////////

namespace controlStripLayout
{
constexpr float titleAreaWidth{70};
constexpr float titleFontSize{13};
constexpr float cornerSize{10};
constexpr float spacerSize{2};
constexpr float groupLabelHeight{16};
constexpr float groupLabelFontSize{12};
// constexpr float sliderLabelHeight{14};
// constexpr float sliderLabelFontSize{12};
// constexpr float minSliderHeight{70};
// constexpr float minKnobSliderWidth{47};
constexpr float sliderLabelHeight{28};
constexpr float sliderLabelFontSize{11.5};
constexpr float minSliderHeight{62};
constexpr float minKnobSliderWidth{47};
constexpr float minIncDecSliderWidth{70};

constexpr float knobIndicatorOffThickness{2};
constexpr float knobIndicatorOnThickness{4};
}

// MIDI KEYBOARD ///////////////////////////////////////////////////////////////

class MidiKeyboardLookAndFeel : public juce::LookAndFeel_V3
{
public:
    MidiKeyboardLookAndFeel()
    {
        setColour(juce::MidiKeyboardComponent::whiteNoteColourId,
                  juce::Colour(255,255,255));
        setColour(juce::MidiKeyboardComponent::blackNoteColourId,
                  juce::Colours::grey);
        setColour(juce::MidiKeyboardComponent::shadowColourId,
                  juce::Colours::transparentBlack);
        setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId,
                  juce::Colours::black);
    }

    ~MidiKeyboardLookAndFeel() override = default;
};

// COMBO BOX ///////////////////////////////////////////////////////////////////

class ComboBoxLookAndFeel : public juce::LookAndFeel_V3
{
public:
    ComboBoxLookAndFeel(juce::Colour bg = juce::Colours::darkgrey,
                        juce::Colour txt = juce::Colours::whitesmoke)
    {
        // see line 99 of juce_LookAndFeel_V2.cpp
        setColour(juce::PopupMenu::backgroundColourId, bg);
        setColour(juce::PopupMenu::textColourId, txt);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, txt);
        setColour(juce::PopupMenu::highlightedTextColourId, bg);
        setColour(juce::ComboBox::backgroundColourId, bg);
        setColour(juce::ComboBox::textColourId, txt);
        setColour(juce::ComboBox::arrowColourId, txt);
        setColour(juce::ComboBox::outlineColourId, txt);
        // setColour(juce::ComboBox::)
    }

    ~ComboBoxLookAndFeel() override = default;

    juce::Font getComboBoxFont(juce::ComboBox& box) override {
        //return { jmin (16.0f, box.getHeight() * 0.85f) };
        return juce::FontOptions(12.0f);
    }

    juce::Font getPopupMenuFont() override
    {
        return juce::FontOptions(12.0f);
    }

    void drawComboBox(juce::Graphics& g, int width, int height,
                      const bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override
    {
        // draw background :

        g.setColour(findColour(juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle(1, 1, width - 2, height - 2, 6);
        g.setColour(findColour(juce::ComboBox::outlineColourId).withAlpha(0.5f));
        g.drawRoundedRectangle(1, 1, width - 2, height - 2, 6, 1);

        // draw arrow :

        if (box.isEnabled()) {
            const float arrowX = 0.35f;
            const float arrowH = 0.35f;
            const float lineH = 0.6f;

            int bx = 1;
            int by = 1;
            // int bw = 20;
            int bw = box.getHeight() + 2;
            int bh = box.getHeight();

            juce::Path p;
            p.addTriangle(bx + bw * 0.5f,
                          by + bh * ((1.0f - arrowH) * 0.5f + arrowH),
                          bx + bw * (1.0f - arrowX),
                          by + bh * ((1.0f - arrowH) * 0.5f),
                          bx + bw * arrowX,
                          by + bh * ((1.0f - arrowH) * 0.5f));

            g.setColour(juce::Colours::white);
            g.fillPath (p);
            g.setColour(juce::Colours::black);
            g.strokePath(p, juce::PathStrokeType(1, juce::PathStrokeType::JointStyle::curved));

            // p.clear();
            // p.startNewSubPath(bx + bw, by + bh * ((1.0f - lineH) * 0.5f));
            // p.lineTo(bx + bw, by + bh * ((1.0f - lineH) * 0.5f + lineH));
            // g.setColour(juce::Colours::white);
            // g.strokePath(p, juce::PathStrokeType(1, juce::PathStrokeType::JointStyle::curved));
        }
    }

    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds(
            box.getHeight() - 2, // left arrow offset
            1,
            box.getWidth() - box.getHeight() - 1, // - (box.getHeight() - 3) - 1
            //box.getWidth() - 20,
            box.getHeight() - 2
        );

        label.setFont(getComboBoxFont(box));
        // for testing purpose :
        /*
        label.setColour(
            juce::Label::backgroundColourId,
            findColour(juce::Label::backgroundColourId)
        );
        label.setBorderSize(BorderSize<int>(2));
        //*/
    }
};

// ARROW BUTTON FOR INC DEC SLIDER /////////////////////////////////////////////

class IncDecArrowButton : public juce::Button
{
public:
    IncDecArrowButton
    (
      const juce::String& name,
      float arrowDirection,
      juce::Colour arrowColour,
      const float arrowPadding = 3.0f,
      const float arrowStrokeWidth = 1.0f
    ) :
        Button(name),
        colour(arrowColour),
        padding(arrowPadding),
        strokeWidth(arrowStrokeWidth)
    {
        path.addTriangle(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.5f);
        path.applyTransform
        (
          juce::AffineTransform::rotation(juce::float_Pi * 2.0f * arrowDirection, 0.5f, 0.5f)
        );
    }

    ~IncDecArrowButton() override = default;

    /** @internal */
    int getWidth()
    {
        return juce::Button::getWidth() - (strokeWidth + padding) * 2;
    }

    int getHeight()
    {
        return Button::getHeight() - (strokeWidth + padding) * 2;
    }

    void paintButton(juce::Graphics& g,
                     bool isMouseOverButton,
                     bool isButtonDown) override
    {
        const float offset = (isButtonDown ? 1.0f : 0.0f) + strokeWidth + padding;

        juce::Path p(path);

        p.applyTransform(
          path.getTransformToScaleToFit(
            offset,
            offset,
            getWidth(),
            getHeight(),
            false
          )
        );

        g.setColour(colour);
        g.fillPath(p);

        g.setColour(juce::Colours::black);
        g.strokePath(
            p, juce::PathStrokeType(
                strokeWidth,
                juce::PathStrokeType::JointStyle::curved
            )
        );

        // shadows :
        juce::Path btnPath = p;

        juce::Path maskPath;
        maskPath.addRectangle(btnPath.getBounds().expanded(5.0f, 5.0f));
        maskPath.addPath(btnPath);

        juce::Path shadowPath;
        shadowPath.addPath(btnPath);

        // looks like we have to choose between drawing inner and outer shadows
        // whatever order we use, only first DS is rendered
        // todo : find out why !
        maskPath.setUsingNonZeroWinding(false);
        shadowPath.setUsingNonZeroWinding(true);
        juce::DropShadow outerDs(juce::Colour{0x99000000}, 10, {3, 2});
        g.reduceClipRegion(maskPath);
        outerDs.drawForPath(g, shadowPath);

        // this one will not be drawn as it comes last :(
        // maskPath.setUsingNonZeroWinding(true);
        // shadowPath.setUsingNonZeroWinding(false);
        // juce::DropShadow innerDs(juce::Colour{0xff000000}, 2, {0, 0});
        // g.reduceClipRegion(shadowPath);
        // innerDs.drawForPath(g, maskPath);
    }

private:
    juce::Colour colour;
    float padding;
    float strokeWidth;
    juce::Path path;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IncDecArrowButton)
};

// INC DEC SLIDER //////////////////////////////////////////////////////////////

class IncDecLAF : public juce::LookAndFeel_V4
{
private:
    bool displayKeys;
    int keyOffset;
    float btnPadding;
    float fontSize;
    juce::Colour btnColour;
    juce::Colour labelBgColour;
    juce::Colour txtColour;

public:
    IncDecLAF(bool dk = false, int ko = 0) :
        displayKeys(dk),
        keyOffset(ko),
        btnPadding(3.f),
        fontSize(12.f),
        btnColour(juce::Colours::whitesmoke),
        labelBgColour(juce::Colours::whitesmoke),
        txtColour(juce::Colours::black)
    {}

    ~IncDecLAF() override = default;

    juce::Slider::SliderLayout getSliderLayout(juce::Slider& s) override
    {
        juce::Slider::SliderLayout layout;
        auto bounds = s.getLocalBounds();
        layout.sliderBounds = bounds.removeFromLeft(0.5f * bounds.getWidth())
                                    .withSizeKeepingCentre(bounds.getWidth(),
                                                           juce::jmin(bounds.getHeight()*1.f, bounds.getWidth()*1.6f));
        bounds.removeFromRight(btnPadding);
        layout.textBoxBounds = bounds.withSizeKeepingCentre(bounds.getWidth(), fontSize * 1.66f);
        return layout;
    }

    juce::Button* createSliderButton(juce::Slider& s, const bool isIncrement) override
    {
        // as in juceLookAndFeel_V2
        return new IncDecArrowButton(
          isIncrement ? "u" : "d",
          isIncrement ? 0.75f : 0.25f,
          btnColour,
          btnPadding, // padding
          1.0f  // strokeWidth
        );
    }

    // Here we can replace the actual value by a note name (e.g. SOL# etc) :

    void drawLabel(juce::Graphics& g, juce::Label& l) override
    {
        juce::Rectangle<float> labelRect(
            1, 1, l.getWidth() - 2, l.getHeight() - 2
        );
        g.setColour(labelBgColour);
        g.fillRoundedRectangle(labelRect, 5);

        juce::FontOptions f = {12.0f, juce::Font::plain};
        g.setFont(f);
        g.setColour(txtColour);
        juce::Rectangle<int> txtr(
            l.getBorderSize().subtractedFrom(l.getLocalBounds())
        );

        juce::String txt = l.getText();
        if (displayKeys)
        {
            const juce::String noteNames[] = {
                "DO",
                "DO#",
                "RE",
                "RE#",
                "MI",
                "FA",
                "FA#",
                "SOL",
                "SOL#",
                "LA",
                "LA#",
                "SI"
              };

            int noteIndex = (txt.getIntValue() + keyOffset) % 12;
            txt = noteNames[noteIndex];
        }

        g.drawFittedText(
            txt,
            txtr,
            l.getJustificationType(),
            juce::jmax(1, (int) (txtr.getHeight() / f.getHeight())),
            l.getMinimumHorizontalScale()
        );
        g.drawRoundedRectangle(labelRect, 5, 1);
    }
};

class ToggleLAF : public juce::LookAndFeel_V4
{
public:
    ToggleLAF()
    {
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colour{0x00000000});
    }

    ~ToggleLAF() override = default;

    void drawToggleButton(juce::Graphics& g,
                          juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override
    {
        // auto fontSize = juce::jmin(15.0f, (float) button.getHeight() * 0.75f);
        auto size = std::min<float>(button.getWidth(), button.getHeight() * 0.66f);
        auto btnWidth = size * 0.5f; //* controlStripLayout::spacerSize
        auto btnHeight = btnWidth * 1.5f;

        drawTickBox(g, button,
                    ((float) button.getWidth() - btnWidth) * 0.5f,
                    ((float) button.getHeight() - btnHeight) * 0.5f,
                    btnWidth, btnHeight,
                    button.getToggleState(),
                    button.isEnabled(),
                    shouldDrawButtonAsHighlighted,
                    shouldDrawButtonAsDown);
    }

    void drawTickBox(juce::Graphics& g, juce::Component& component,
                     float x, float y, float w, float h,
                     const bool ticked,
                     [[maybe_unused]] const bool isEnabled,
                     [[maybe_unused]] const bool shouldDrawButtonAsHighlighted,
                     [[maybe_unused]] const bool shouldDrawButtonAsDown) override
    {
        juce::Rectangle<float> tickBounds;
        juce::Rectangle<float> ledBounds;
        juce::Path tickShape;
        juce::Path tickMask;

        if (h > w) {
            tickBounds = juce::Rectangle<float>(x, y + h - w, w, w);
            ledBounds = juce::Rectangle<float>(x, y, w, h - w);
        } else {
            tickBounds = juce::Rectangle<float>(x, y, h, h);
            ledBounds = juce::Rectangle<float>(x + w, y, h, w - h);
        }

        ledBounds = ledBounds.withCentre({ledBounds.getCentreX(), ledBounds.getY()});

        g.setColour(juce::Colours::darkgreen);
        g.fillEllipse(ledBounds.withSizeKeepingCentre(9, 9));

        if (!ticked)
        {
            tickShape.addRoundedRectangle(tickBounds, 4.0f, 2.0f);
            tickMask.addRoundedRectangle(tickBounds, 4.0f, 2.0f);
            tickMask.setUsingNonZeroWinding(false);
            juce::DropShadow ds(juce::Colours::black, 3, {2, 2});
            ds.drawForPath(g, tickMask);

            g.setColour(juce::Colours::whitesmoke);
            g.fillRoundedRectangle(tickBounds, 4.0f);
            g.setColour(juce::Colours::black);
            g.drawRoundedRectangle(tickBounds, 4.0f, 2.0f);
        }
        else
        {
            g.setColour(juce::Colours::lightgreen);
            g.fillEllipse(ledBounds.withSizeKeepingCentre(6,6));

            g.setColour(juce::Colours::black);
            g.fillRoundedRectangle(tickBounds, 4.0f);
            g.setColour(juce::Colours::black);
            g.drawRoundedRectangle(tickBounds, 4.0f, 2.0f);

            juce::Rectangle<float> tickBoundsExpanded = tickBounds.expanded(3, 3);
            tickShape.addRoundedRectangle(tickBoundsExpanded, 4.0f, 2.0f);
            tickMask.addRoundedRectangle(tickBounds, 4.0f, 2.0f);
            tickMask.setUsingNonZeroWinding(true);
            g.reduceClipRegion(juce::Rectangle<int>(x, y, w, h));
            juce::DropShadow ds(juce::Colours::whitesmoke.withAlpha(0.9f), 2, {2, 2});
            ds.drawForPath(g, tickMask);
        }
    }
};

///////////////////////////////////////////////////////////////////////////////
/// SIMPLER LOOK AND FEELS (tintable procedural knob UIs with discrete shadows)
///
class KnobLAF : public juce::LookAndFeel_V4
{
public:
    KnobLAF()
    {
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colour{0x00000000});
    }

    ~KnobLAF() override = default;

    juce::Font getLabelFont(juce::Label &) override
    {
        return juce::FontOptions{12.0f, juce::Font::plain};
    }

    juce::Slider::SliderLayout getSliderLayout(juce::Slider& s) override
    {
        auto layout = LookAndFeel_V4::getSliderLayout(s);
        if (s.getTextBoxPosition() == juce::Slider::NoTextBox)
            return layout;

        const bool compact = s.getProperties().getWithDefault("compactTextBox", false);
        const int tbH = layout.textBoxBounds.getHeight();

        const auto bounds
            = compact
                ? s.getLocalBounds().withTrimmedTop(6)
                : s.getLocalBounds();
        const int gap = compact ? -2 : -4; // <-- tweak this (0..4)


        layout.textBoxBounds = layout.textBoxBounds
            .withX(bounds.getX())
            .withWidth(bounds.getWidth())
            .withHeight(tbH)
            .withY(bounds.getBottom() - tbH + gap);

        layout.sliderBounds = bounds.withTrimmedBottom(tbH + gap);

        return layout;
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos,
                          const float rotaryStartAngle,
                          const float rotaryEndAngle,
                          juce::Slider& slider) override

    {
        auto outline = slider.findColour(juce::Slider::rotarySliderOutlineColourId);
        auto fill = slider.findColour(juce::Slider::rotarySliderFillColourId);

        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced(2);

        auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
        // clip max knob size (but textbox below is still annoying) :
        radius = juce::jmin(radius, 18.f);
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto lineW = juce::jmin (8.0f, radius * 0.5f);
        auto arcRadius = 0.75f * radius;
        // auto arcRadius = radius - lineW * 0.5f;

        // auto c = juce::Colours::lightgoldenrodyellow;
        // auto c = juce::Colours::gold;
        // auto c = juce::Colours::orangered;
        // auto c = juce::Colours::hotpink;
        // auto c = juce::Colours::limegreen;
        // auto c = juce::Colour{255, 153, 0}; // orange mecanique
        auto c = fill;

        juce::Rectangle r{2 * arcRadius, 2 * arcRadius};

        juce::Path backgroundArc;
        backgroundArc.addCentredArc(bounds.getCentreX(),
                                    bounds.getCentreY(),
                                    arcRadius * 1.33f,
                                    arcRadius * 1.33f,
                                    0.0f,
                                    rotaryStartAngle,
                                    rotaryEndAngle,
                                    true);

        g.setColour(c.withAlpha(0.6f));
        // g.setColour(juce::Colour{0x99ffffff});
        g.strokePath(
            backgroundArc,
            juce::PathStrokeType(
                controlStripLayout::knobIndicatorOffThickness,
                juce::PathStrokeType::curved,
                juce::PathStrokeType::rounded
            )
        );

        if (slider.isEnabled() && slider.getValue() > 0)
        {
            juce::Path valueArc;
            valueArc.addCentredArc(bounds.getCentreX(),
                                   bounds.getCentreY(),
                                   arcRadius * 1.33f,
                                   arcRadius * 1.33f,
                                   0.0f,
                                   rotaryStartAngle,
                                   toAngle,
                                   true);

            // g.setColour(fill);
            // g.setColour(juce::Colour{0xeeffffff});
            g.setColour(c.withAlpha(0.8f));
            g.strokePath(
                valueArc,
                juce::PathStrokeType(
                    controlStripLayout::knobIndicatorOnThickness,
                    juce::PathStrokeType::curved,
                    juce::PathStrokeType::rounded
                )
            );
        }

        // DRAW SHADOWS ---------------------------------------
        juce::Path ellipsis;
        ellipsis.addEllipse(r.withCentre(bounds.getCentre()));

        // invert the path's fill shape and enlarge it,
        // so it casts a shadow
        juce::Path shadowPath(ellipsis);
        // shadowPath.addRectangle(shadowPath.getBounds().expanded(10));
        shadowPath.setUsingNonZeroWinding(false);
        // reduce clip region to avoid the shadow
        // being drawn outside of the shape to cast the shadow on
        // g.reduceClipRegion(ellipsis);

        juce::DropShadow ds(juce::Colours::black, 10, {3, 2});
        ds.drawForPath(g, shadowPath);
        //-----------------------------------------------------

        // actual white circle bg
        g.setColour(c);
        g.fillEllipse(r.withCentre(bounds.getCentre()));

        g.setColour(juce::Colour{0x22000000});
        g.fillEllipse(r.withCentre(bounds.getCentre()));

        auto r2 = r * 0.9;

        g.setColour(c);
        g.fillEllipse(r2.withCentre(bounds.getCentre() - juce::Point<float>(0.5, 0.5)));

        // actual white circle bg
        g.setColour(juce::Colour{0x22000000});
        g.fillEllipse(r.withCentre(bounds.getCentre()));

        r2 *= 0.9;

        g.setColour (c);
        g.fillEllipse (r2.withCentre(bounds.getCentre() - juce::Point<float>(1, 1)));

        //=====================================================================

        auto thumbWidth = arcRadius * 0.7f;//lineW * 1.2f;
        auto thumbHeight = 5;
        juce::Point<float> thumbPoint(
            bounds.getCentreX()
                + (arcRadius * 0.8f) * std::cos(toAngle - juce::MathConstants<float>::halfPi),
            bounds.getCentreY()
                + (arcRadius * 0.8f) * std::sin(toAngle - juce::MathConstants<float>::halfPi));
        // juce::Point<float> thumbPoint(bounds.getCentreX() * std::cos(toAngle - juce::MathConstants<float>::halfPi),
        //                               bounds.getCentreY() * std::sin(toAngle - juce::MathConstants<float>::halfPi));

        // g.setColour(slider.findColour(juce::Slider::thumbColourId));
        // g.setColour(juce::Colour{0x99000000});
        // g.fillEllipse(juce::Rectangle<float>(thumbWidth, thumbWidth).withCentre(thumbPoint));

        juce::AffineTransform transform;
        float rotation = toAngle - juce::MathConstants<float>::halfPi;
        transform = juce::AffineTransform::scale(1.f)
            .rotation(rotation, thumbPoint.x, thumbPoint.y);
            //.translated(thumbWidth,thumbHeight);
        juce::Path p;
        p.addRoundedRectangle(
            thumbPoint.x - thumbWidth * 0.5f,
            thumbPoint.y - thumbHeight * 0.5f,
            thumbWidth,
            thumbHeight,
            5
        );
        p.applyTransform(transform);
        g.setColour(juce::Colour{0x99000000});
        g.fillPath(p);
        g.setColour(juce::Colour{0x66000000});
        g.strokePath(p, juce::PathStrokeType(2.0f));
    }
private:
    // bool initialized = false;
};

///////////////////////////////////////////////////////////////////////////////
/// SAME AS KnobLAF BUT CENTERED FOR PANNING ETC

class CentredKnobLAF : public KnobLAF
{

public:
    CentredKnobLAF() = default;
    ~CentredKnobLAF() override = default;

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos,
                           const float rotaryStartAngle,
                           const float rotaryEndAngle,
                           juce::Slider& slider) override

    {
        auto outline = slider.findColour (juce::Slider::rotarySliderOutlineColourId);
        auto fill = slider.findColour (juce::Slider::rotarySliderFillColourId);

        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced(2);

        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        // clip max knob size (but textbox below is still annoying) :
        radius = juce::jmin(radius, 18.f);
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto arcRadius = 0.75f * radius;

        auto c = fill;

        juce::Rectangle r{2 * arcRadius, 2 * arcRadius};

        juce::Path backgroundArcs;
        backgroundArcs.addCentredArc (bounds.getCentreX(),
                                      bounds.getCentreY(),
                                      arcRadius * 1.33f,
                                      arcRadius * 1.33f,
                                      0.0f,
                                      rotaryStartAngle,
                                      (rotaryStartAngle + rotaryEndAngle) * 0.5f - 0.35f,
                                      true);
        backgroundArcs.addCentredArc (bounds.getCentreX(),
                                      bounds.getCentreY(),
                                      arcRadius * 1.33f,
                                      arcRadius * 1.33f,
                                      0.0f,
                                      (rotaryStartAngle + rotaryEndAngle) * 0.5f + 0.35f,
                                      rotaryEndAngle,
                                      true);
        backgroundArcs.addEllipse(bounds.getCentreX() - 1, bounds.getCentreY() - arcRadius * 1.33f - 1, 2, 2);
        g.setColour(c.withAlpha(0.6f));
        g.strokePath(
            backgroundArcs,
            juce::PathStrokeType(
                controlStripLayout::knobIndicatorOffThickness,
                juce::PathStrokeType::curved,
                juce::PathStrokeType::rounded
            )
        );

        juce::Path valueArc;
        if (slider.isEnabled() &&
            toAngle < (rotaryStartAngle + rotaryEndAngle) * 0.5f - 0.35f) {
            valueArc.addCentredArc(bounds.getCentreX(),
                                   bounds.getCentreY(),
                                   arcRadius * 1.33f,
                                   arcRadius * 1.33f,
                                   0.0f,
                                   toAngle,
                                   (rotaryStartAngle + rotaryEndAngle) * 0.5f - 0.35f,
                                   true);
        } else if (slider.isEnabled() &&
            toAngle > (rotaryStartAngle + rotaryEndAngle) * 0.5f + 0.35f) {
            valueArc.addCentredArc(bounds.getCentreX(),
                                   bounds.getCentreY(),
                                   arcRadius * 1.33f,
                                   arcRadius * 1.33f,
                                   0.0f,
                                   (rotaryStartAngle + rotaryEndAngle) * 0.5f + 0.35f,
                                   toAngle,
                                   true);
        } else {
            valueArc.addEllipse(bounds.getCentreX() - 1, bounds.getCentreY() - arcRadius * 1.33f - 1, 2, 2);
        }

        g.setColour(c.withAlpha(0.8f));
        g.strokePath(
            valueArc,
            juce::PathStrokeType(
                controlStripLayout::knobIndicatorOnThickness,
                juce::PathStrokeType::curved,
                juce::PathStrokeType::rounded
            )
        );

        // DRAW SHADOWS ---------------------------------------
        juce::Path ellipsis;
        ellipsis.addEllipse(r.withCentre(bounds.getCentre()));

        // invert the path's fill shape and enlarge it,
        // so it casts a shadow
        juce::Path shadowPath(ellipsis);
        // shadowPath.addRectangle(shadowPath.getBounds().expanded(10));
        shadowPath.setUsingNonZeroWinding(false);
        // reduce clip region to avoid the shadow
        // being drawn outside of the shape to cast the shadow on
        // g.reduceClipRegion(ellipsis);

        juce::DropShadow ds(juce::Colours::black, 10, {3, 2});
        ds.drawForPath(g, shadowPath);
        //-----------------------------------------------------

        // actual white circle bg
        g.setColour (c);
        g.fillEllipse (r.withCentre(bounds.getCentre()));

        g.setColour (juce::Colour{0x22000000});
        g.fillEllipse (r.withCentre(bounds.getCentre()));

        auto r2 = r * 0.9;

        g.setColour(c);
        g.fillEllipse(r2.withCentre(bounds.getCentre() - juce::Point<float>(0.5, 0.5)));

        // actual white circle bg
        g.setColour(juce::Colour{0x22000000});
        g.fillEllipse(r.withCentre(bounds.getCentre()));

        r2 *= 0.9;

        g.setColour(c);
        g.fillEllipse(r2.withCentre(bounds.getCentre() - juce::Point<float>(1, 1)));

        //=====================================================================

        auto thumbWidth = arcRadius * 0.7f;//lineW * 1.2f;
        auto thumbHeight = 5;
        juce::Point<float> thumbPoint(
            bounds.getCentreX()
                + (arcRadius * 0.8f) * std::cos (toAngle - juce::MathConstants<float>::halfPi),
            bounds.getCentreY()
                + (arcRadius * 0.8f) * std::sin (toAngle - juce::MathConstants<float>::halfPi)
        );
        // juce::Point<float> thumbPoint(
        //      bounds.getCentreX() * std::cos (toAngle - juce::MathConstants<float>::halfPi),
        //      bounds.getCentreY() * std::sin (toAngle - juce::MathConstants<float>::halfPi)
        // );

        // g.setColour(slider.findColour(juce::Slider::thumbColourId));
        // g.setColour(juce::Colour{0x99000000});
        // g.fillEllipse(juce::Rectangle<float>(thumbWidth, thumbWidth).withCentre(thumbPoint));

        juce::AffineTransform transform;
        float rotation = toAngle - juce::MathConstants<float>::halfPi;
        transform = juce::AffineTransform::scale(1.f)
            .rotation(rotation, thumbPoint.x, thumbPoint.y);
            //.translated(thumbWidth,thumbHeight);
        juce::Path p;
        p.addRoundedRectangle(
            thumbPoint.x - thumbWidth * 0.5f,
            thumbPoint.y - thumbHeight * 0.5f,
            thumbWidth,
            thumbHeight,
            5
        );
        p.applyTransform(transform);
        g.setColour(juce::Colour{0x99000000});
        g.fillPath(p);
        g.setColour(juce::Colour{0x66000000});
        g.strokePath(p, juce::PathStrokeType(2.0f));
    }
};

///////////////////////////////////////////////////////////////////////////////
/// ANOTHER ONE (BIG BLACK KNOB WITH WHITE CIRCLE POINTER) ///

class NotchedKnobLAF : public juce::LookAndFeel_V4
{
    int notches;
    float notchAngle;
    std::unique_ptr<juce::Drawable> rotarySliderFg;

    // Ramanujan formula
    static float approximateEllipsePerimeter(float a, float b)
    {
        const float sum = a + b;
        const float diff = a - b;
        const float h = (diff * diff) / (sum * sum);

        return juce::MathConstants<float>::pi * sum *
               (1.0f + (3.0f * h) / (10.0f + std::sqrt(4.0f - 3.0f * h)));
    }
public:
    NotchedKnobLAF(int nbNotches = 12) : notches(nbNotches) {
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colour{0x00000000});

        rotarySliderFg = juce::Drawable::createFromImageData(
          BinaryData::basicknoboutline_svg,
          BinaryData::basicknoboutline_svgSize
        );

        float notchWidth = 1.0f / static_cast<float>(2 * notches);
        notchAngle = notchWidth * juce::MathConstants<float>::twoPi;
    }

    ~NotchedKnobLAF() override = default;

    juce::Font getLabelFont(juce::Label &) override
    {
        return juce::FontOptions{12.0f, juce::Font::plain};
    }

    juce::Slider::SliderLayout getSliderLayout(juce::Slider& s) override
    {
        auto layout = LookAndFeel_V4::getSliderLayout(s);
        if (s.getTextBoxPosition() == juce::Slider::NoTextBox)
            return layout;

        const bool compact = s.getProperties().getWithDefault("compactTextBox", false);
        const int tbH = layout.textBoxBounds.getHeight();

        const auto bounds
            = compact
                ? s.getLocalBounds().withTrimmedTop(6)
                : s.getLocalBounds();
        const int gap = compact ? -2 : -4; // <-- tweak this (0..4)

        layout.textBoxBounds = layout.textBoxBounds
            .withX(bounds.getX())
            .withWidth(bounds.getWidth())
            .withHeight(tbH)
            .withY(bounds.getBottom() - tbH + gap);

        layout.sliderBounds = bounds.withTrimmedBottom(tbH + gap);

        return layout;
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos,
                          const float rotaryStartAngle,
                          const float rotaryEndAngle,
                          juce::Slider& slider) override
    {
        const float centreX = (float) x + (float) width * 0.5f;
        const float centreY = (float) y + (float) height * 0.5f;

        auto fgRect = rotarySliderFg->getDrawableBounds();
        float dim = juce::jmin(width, height);
        float ratio = dim / fgRect.getWidth(); // or getHeight, is square
        float myRatio = 0.75f;

        // float transX = width > height ? (width - dim) / 2.0f : 0;
        // float transY = width > height ? 0 : (height - dim) / 2.0f;

        auto outline = slider.findColour(juce::Slider::rotarySliderOutlineColourId);
        // auto fill = slider.findColour (Slider::rotarySliderFillColourId);
        // auto fill = juce::Colour{0xaa393939};
        // auto fill = juce::Colours::whitesmoke;
        auto fill = juce::Colours::black;

        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();//.reduced(2);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        // clip max knob size (but textbox below is still annoying) :
        // radius = juce::jmin(radius, 22.f);
        auto arcRadius = myRatio * radius;

        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        auto c = fill;

        // g.setColour(juce::Colours::red);
        // g.fillRect(bounds);

        juce::Rectangle r{2 * arcRadius, 2 * arcRadius};

        juce::Path backgroundArc;
        backgroundArc.addCentredArc(bounds.getCentreX(),
                                    bounds.getCentreY(),
                                    arcRadius * 1.22f,
                                    arcRadius * 1.22f,
                                    0.0f,
                                    rotaryStartAngle,
                                    rotaryEndAngle,
                                    true);

        g.setColour(c.withAlpha(0.6f));
        // g.setColour(juce::Colour{0x99ffffff});
        g.strokePath(
            backgroundArc,
            juce::PathStrokeType(
                controlStripLayout::knobIndicatorOffThickness,
                juce::PathStrokeType::curved,
                juce::PathStrokeType::rounded
            )
        );

        if (slider.isEnabled() && slider.getValue() > 0)
        {
            juce::Path valueArc;
            valueArc.addCentredArc(bounds.getCentreX(),
                                   bounds.getCentreY(),
                                   arcRadius * 1.22f,
                                   arcRadius * 1.22f,
                                   0.0f,
                                   rotaryStartAngle,
                                   toAngle,
                                   true);

            g.setColour(c.withAlpha(0.8f));
            g.strokePath(
                valueArc,
                juce::PathStrokeType(
                    controlStripLayout::knobIndicatorOnThickness,
                    juce::PathStrokeType::curved,
                    juce::PathStrokeType::rounded
                )
            );
        }

        // FAILED : ALWAYS A SMALL OFFSET AT THE CENTRE DUE TO ROUNDING ERRORS (IN INKSCAPE ... ?)
        //juce::Colour fgc = juce::Colours::black;//findColour(Slider::rotarySliderFillColourId);
        // auto fg = rotarySliderFg->createCopy();
        /*
        rotarySliderFg->replaceColour(juce::Colours::black, fgc);
        rotarySliderFg->draw(g, 1.f,
          juce::AffineTransform::scale(ratio)
          .rotated(sliderPos * 3 * M_PI * 0.5, dim * 0.5, dim * 0.5)
          // .translated(radius / 2.0f, radius / 2.0f)
          // .translated(bounds.getCentreX() - dim * 0.5, bounds.getCentreY() - dim * 0.5)
          // .translated(bounds.getCentreX() - arcRadius, bounds.getCentreY() - arcRadius)
          // .translated(centreX - dim * 0.5f, centreY - dim * 0.5f)
          .translated(centreX - ratio * width * 0.5f, centreY - ratio * height * 0.5f)
          // .translated(r.getTopLeft())
          // .scaled(myRatio * 0.99f)
          .scaled(myRatio)
        );
        //*/

        // DRAW SHADOWS ---------------------------------------
        juce::Path ellipsis;
        ellipsis.addEllipse(r.withCentre(bounds.getCentre()));

        g.setColour(juce::Colour(0xff222222));
        juce::PathStrokeType pathStrokeType(2.0);
        // float segmentLength = approximateEllipsePerimeter(arcRadius, arcRadius) / notches;
        // or simpler (as we draw a circle, not an ellipse) :
        float segmentLength = arcRadius * juce::MathConstants<float>::twoPi / static_cast<float>(notches);
        float notchPercent = 0.3f;
        float dashedLength[2] = {
            segmentLength * notchPercent,
            segmentLength * (1.0f - notchPercent)
        };
        pathStrokeType.setJointStyle(juce::PathStrokeType::JointStyle::curved);
        pathStrokeType.setEndStyle(juce::PathStrokeType::EndCapStyle::rounded);
        pathStrokeType.createDashedStroke(ellipsis, ellipsis, dashedLength, 2);

        ellipsis.applyTransform(
            juce::AffineTransform::scale(1)
            .rotated(
                toAngle - juce::MathConstants<float>::halfPi,
                // sliderPos * 3 * M_PI * 0.5,
                width * 0.5,
                height * 0.5
            )
            .translated(
                bounds.getCentreX() - width * 0.5f,
                bounds.getCentreY() - height * 0.5f
            )
        );

        g.strokePath(
            ellipsis,
            pathStrokeType
        );

        // g.setColour(juce::Colour(0xffffffff));
        g.setColour(juce::Colour(0xff000000));

        // invert the path's fill shape and enlarge it,
        // so it casts a shadow
        juce::Path shadowPath(ellipsis);
        shadowPath.setUsingNonZeroWinding(false);
        // reduce clip region to avoid the shadow
        // being drawn outside of the shape to cast the shadow on
        // g.reduceClipRegion(ellipsis);

        juce::DropShadow ds(juce::Colours::black, 5, {2, 2});
        ds.drawForPath(g, shadowPath);
        //-----------------------------------------------------

        // actual white circle bg
        g.setColour(c);
        g.fillEllipse (r.withCentre(bounds.getCentre()));

        g.setColour(juce::Colour{0x22000000});
        g.fillEllipse (r.withCentre(bounds.getCentre()));

        auto r2 = r * 0.9;

        g.setColour(c);
        g.fillEllipse (r2.withCentre(bounds.getCentre() - juce::Point<float>(0.5, 0.5)));

        // actual white circle bg
        g.setColour (juce::Colour{0x22000000});
        g.fillEllipse (r.withCentre(bounds.getCentre()));

        r2 *= 0.9;

        g.setColour (c);
        g.fillEllipse (r2.withCentre(bounds.getCentre() - juce::Point<float>(1, 1)));

        //======================================================================
        // THUMB POINT

        auto thumbWidth = 7;//arcRadius * 0.25f;//lineW * 1.2f;
        auto thumbHeight = thumbWidth;
        juce::Point<float> thumbPoint (bounds.getCentreX() + (arcRadius * 0.5f) * std::cos(toAngle - juce::MathConstants<float>::halfPi),
                                 bounds.getCentreY() + (arcRadius * 0.5f) * std::sin(toAngle - juce::MathConstants<float>::halfPi));
        auto markerWidth = 3;
        auto markerHeight = 3;
        juce::Point<float> markerPoint(bounds.getCentreX() + (arcRadius * 1.22f) * std::cos(toAngle - juce::MathConstants<float>::halfPi),
                                 bounds.getCentreY() + (arcRadius * 1.22f) * std::sin(toAngle - juce::MathConstants<float>::halfPi));
        // Point<float> thumbPoint (bounds.getCentreX() * std::cos (toAngle - MathConstants<float>::halfPi),
        //                          bounds.getCentreY() * std::sin (toAngle - MathConstants<float>::halfPi));

        // g.setColour (slider.findColour (Slider::thumbColourId));
        // g.setColour(juce::Colour{0x99000000});
        g.setColour(juce::Colour{0x99ffffff});
        // g.fillEllipse(Rectangle<float> (thumbWidth, thumbWidth).withCentre (thumbPoint));

        juce::AffineTransform transform;
        float rotation = toAngle - juce::MathConstants<float>::halfPi;
        juce::Path p;
        p.addRoundedRectangle(
            (width - thumbWidth) * 0.5f,
            (height - thumbHeight) * 0.5f,
            thumbWidth,
            thumbHeight,
            5
        );
        p.applyTransform(juce::AffineTransform::scale(1.f)
            .translated(arcRadius * 0.75f, 0)
            .rotated(rotation, width * 0.5f, height * 0.5f)
        );

        // g.setColour(juce::Colour{0x99ffffff});
        // g.setColour(juce::Colours::whitesmoke);
        g.fillPath(p);
        // g.setColour(Colour{0x66000000});
        g.strokePath(p, juce::PathStrokeType(2.0f));

        //======================================================================
        // MARKER POINT
        transform = juce::AffineTransform::scale(1.f)
            .rotation(rotation, markerPoint.x, markerPoint.y);
        //.translated(thumbWidth,thumbHeight);
        p.clear();
        return;
        p.addRoundedRectangle(
            markerPoint.x - markerWidth * 0.5f,
            markerPoint.y - markerHeight * 0.5f,
            markerWidth,
            markerHeight,
            3
        );
        p.applyTransform(transform);

        if (sliderPos > 0.f) {
            g.setColour(c);
            // g.setColour(juce::Colour{0x99ffffff});
            // g.setColour(juce::Colours::whitesmoke);
            g.fillPath(p);
            // g.setColour(juce::Colour{0x66000000});
            g.strokePath(p, juce::PathStrokeType(2.0f));
        }
    }
};

///////////////////////////////////////////////////////////////////////////////
// original CMS knob Look and feel with centered MIDI digits display

class KnobLookAndFeel : public juce::LookAndFeel_V3
{
  bool drawValue;
  std::unique_ptr<juce::Drawable> rotarySliderBg;
  std::unique_ptr<juce::Drawable> rotarySliderFg;

public:
  KnobLookAndFeel(juce::Colour fg, juce::Colour bg, bool dv) : drawValue(dv)
  {
    // just a placeholder to retrieve colour later (see below in drawRotarySlider)
    setColour(juce::Slider::rotarySliderFillColourId, fg);
    setColour(juce::Slider::backgroundColourId, bg);

    rotarySliderBg = juce::Drawable::createFromImageData(
      BinaryData::basicknobbgbw_svg,
      BinaryData::basicknobbgbw_svgSize
    );

    rotarySliderFg = juce::Drawable::createFromImageData(
      BinaryData::basicknobindicator_svg,
      BinaryData::basicknobindicator_svgSize
    );
  }

  ~KnobLookAndFeel() override = default;

  void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                        float sliderPosProportional,
                        float rotaryStartAngle, float rotaryEndAngle,
                        juce::Slider& s) override
  {
    int actualValue = floor(sliderPosProportional * 127.999);

    // draw SVGs

    auto bgRect = rotarySliderBg->getDrawableBounds();
    //auto handleRect = rotarySliderFg->getDrawableBounds();
    //auto value = s.getValue();

    float dim = juce::jmin(width, height);
    float ratio = dim / bgRect.getWidth(); // or getHeight (the img is square)

    float transX = width > height ? (width - dim) / 2.0f : 0;
    float transY = width > height ? 0 : (height - dim) / 2.0f;

    juce::Colour bgc = findColour(juce::Slider::backgroundColourId);
    // auto bg = rotarySliderBg->createCopy();
    rotarySliderBg->replaceColour(juce::Colours::white, bgc);
    // rotarySliderBg->draw(g, 1.f,
    //   AffineTransform::scale(ratio)
    //   .translated(transX, transY)
    // );
    g.setColour(juce::Colours::whitesmoke);
    g.fillEllipse(x, y, width, height);
    g.setColour(juce::Colours::black);
    g.drawEllipse(x, y, width - 4, height - 4, 2);

    juce::Colour fgc = findColour(juce::Slider::rotarySliderFillColourId);
    // auto fg = rotarySliderFg->createCopy();
    rotarySliderFg->replaceColour(juce::Colours::white, fgc);
    rotarySliderFg->draw(g, 1.f,
      juce::AffineTransform::scale(ratio)
      .rotated(sliderPosProportional * 3 * M_PI * 0.5, dim * 0.5, dim * 0.5)
      .translated(transX, transY)
    );

    // draw numbers

    if (!drawValue) return;

    //Rectangle<int> bounds = g.getClipBounds();
    //float ddwf = bounds.getHeight() / 5.f;
    float ddw = dim / 5.1f;
    float ddh = 5 * ddw / 4;
    g.setFont(juce::Font(ddh - 2, juce::Font::bold));

    int digits[3] = {
      actualValue / 100,
      (actualValue % 100) / 10,
      (actualValue % 10)
    };

    juce::Point<int> centre = juce::Point<int>(dim / 2, dim / 2);
    std::vector<juce::Rectangle<float>> digitDisplays;
    const int digitSpacer = 0;
    for (int i = 0; i < 3; ++i) {
      digitDisplays.push_back(juce::Rectangle<float>(
        centre.getX() - (ddw - digitSpacer) / 2 + (i - 1) * ddw,
        centre.getY() - ddh / 2,
        (ddw - digitSpacer),
        ddh
      ));
      g.setColour(juce::Colours::white);
      g.fillRoundedRectangle(digitDisplays.back(), ddw / 4);
      g.setColour(juce::Colours::black);
      g.drawRoundedRectangle(digitDisplays.back(), ddw / 4, 1);
      g.drawText(
        juce::String(digits[i]),
        digitDisplays.back(),
        juce::Justification::centred
      );
    }
  }

  virtual void drawLabel(juce::Graphics& g, juce::Label& l) override
  {
    std::uint8_t grey = 230;
    juce::Rectangle<float> labelRect(1, 1, l.getWidth() - 2, l.getHeight() - 2);
    g.setColour(juce::Colour(grey, grey, grey));
    g.fillRoundedRectangle(labelRect, 5);

    juce::Font f = getLabelFont(l);
    g.setColour(juce::Colours::black);
    g.setFont(f);
    juce::Rectangle<int> txtr(l.getBorderSize().subtractedFrom(l.getLocalBounds()));
    g.drawFittedText(l.getText(), txtr, l.getJustificationType(),
                     juce::jmax (1, (int) (txtr.getHeight() / f.getHeight())),
                     l.getMinimumHorizontalScale());
    g.drawRoundedRectangle(labelRect, 5, 1);
  }
};

#endif //COMPOSESIREN_LOOKANDFEELS_H
