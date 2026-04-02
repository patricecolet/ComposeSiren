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

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
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
        g.strokePath(backgroundArc,
                     juce::PathStrokeType(2,
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
            g.strokePath(valueArc,
                         juce::PathStrokeType(4,
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
/// ANOTHER ONE (BIG BLACK KNOB WITH WHITE CIRCLE POINTER) ///

class KnobLAF2 : public juce::LookAndFeel_V4
{
    std::unique_ptr<juce::Drawable> rotarySliderFg;

public:
    KnobLAF2() {
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colour{0x00000000});

        rotarySliderFg = juce::Drawable::createFromImageData(
          BinaryData::basicknoboutline_svg,
          BinaryData::basicknoboutline_svgSize
        );
    }

    ~KnobLAF2() override = default;

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
        auto fgRect = rotarySliderFg->getDrawableBounds();
        float dim = juce::jmin(width, height);
        float ratio = dim / fgRect.getWidth(); // or getHeight, is square
        float myRatio = 0.75f;

        float transX = width > height ? (width - dim) / 2.0f : 0;
        float transY = width > height ? 0 : (height - dim) / 2.0f;

        auto outline = slider.findColour(juce::Slider::rotarySliderOutlineColourId);
        // auto fill = slider.findColour (Slider::rotarySliderFillColourId);
        // auto fill = juce::Colour{0xaa393939};
        auto fill = juce::Colours::whitesmoke;

        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced(2);
        auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
        // clip max knob size (but textbox below is still annoying) :
        radius = juce::jmin(radius, 22.f);
        auto arcRadius = myRatio * radius;

        float offset = (myRatio) * radius;
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        auto c = fill;

        juce::Rectangle r{2 * arcRadius, 2 * arcRadius};

        juce::Path backgroundArc;
        backgroundArc.addCentredArc (bounds.getCentreX(),
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
                2,
                juce::PathStrokeType::curved,
                juce::PathStrokeType::rounded
            )
        );

        if (slider.isEnabled() && slider.getValue() > 0)
        {
            juce::Path valueArc;
            valueArc.addCentredArc (bounds.getCentreX(),
                                     bounds.getCentreY(),
                                     arcRadius * 1.22f,
                                     arcRadius * 1.22f,
                                     0.0f,
                                     rotaryStartAngle,
                                     toAngle,
                                     true);

            // g.setColour (fill);
            // g.setColour (Colour{0xeeffffff});
            // g.setColour(juce::Colours::whitesmoke.withAlpha(0.8f));
            g.setColour(c.withAlpha(0.8f));
            g.strokePath(
                valueArc,
                juce::PathStrokeType(
                    3,
                    juce::PathStrokeType::curved,
                    juce::PathStrokeType::rounded
                )
            );
        }

        juce::Colour fgc = juce::Colours::white;//findColour(Slider::rotarySliderFillColourId);
        // auto fg = rotarySliderFg->createCopy();
        /*
        rotarySliderFg->replaceColour(Colours::black, fgc);
        rotarySliderFg->draw(g, 1.f,
          juce::AffineTransform::scale(ratio)
          .rotated(sliderPos * 3 * M_PI * 0.5, dim * 0.5, dim * 0.5)
          // .translated(transX + offset, transY + offset)
          // .translated(bounds.getCentreX() - dim * 0.5,bounds.getCentreY() - dim * 0.5)
          .translated(bounds.getCentreX() - arcRadius * 0.99f, bounds.getCentreY() - arcRadius * 0.85f)
          // .translated(r.getTopLeft())
          .scaled(myRatio * 0.99f)
        );
        //*/

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

        // juce::DropShadow ds(juce::Colour{0xaa000000}, 3, {2, 2});
        juce::DropShadow ds(juce::Colours::black, 3, {2, 2});
        ds.drawForPath(g, shadowPath);
        //-----------------------------------------------------

        // actual white circle bg
        g.setColour(c);
        g.fillEllipse (r.withCentre(bounds.getCentre()));

        g.setColour(juce::Colour{0x22000000});
        g.fillEllipse (r.withCentre(bounds.getCentre()));

        auto r2 = r * 0.9;

        g.setColour (c);
        g.fillEllipse (r2.withCentre(bounds.getCentre() - juce::Point<float>(0.5, 0.5)));

        // actual white circle bg
        g.setColour (juce::Colour{0x22000000});
        g.fillEllipse (r.withCentre(bounds.getCentre()));

        r2 *= 0.9;

        g.setColour (c);
        g.fillEllipse (r2.withCentre(bounds.getCentre() - juce::Point<float>(1, 1)));

        //=====================================================================

        auto thumbWidth = 7;//arcRadius * 0.25f;//lineW * 1.2f;
        auto thumbHeight = thumbWidth;
        juce::Point<float> thumbPoint (bounds.getCentreX() + (arcRadius * 0.5f) * std::cos (toAngle - juce::MathConstants<float>::halfPi),
                                 bounds.getCentreY() + (arcRadius * 0.5f) * std::sin (toAngle - juce::MathConstants<float>::halfPi));
        auto markerWidth = 3;
        auto markerHeight = 3;
        juce::Point<float> markerPoint(bounds.getCentreX() + (arcRadius * 1.22f) * std::cos(toAngle - juce::MathConstants<float>::halfPi),
                                 bounds.getCentreY() + (arcRadius * 1.22f) * std::sin(toAngle - juce::MathConstants<float>::halfPi));
        // Point<float> thumbPoint (bounds.getCentreX() * std::cos (toAngle - MathConstants<float>::halfPi),
        //                          bounds.getCentreY() * std::sin (toAngle - MathConstants<float>::halfPi));

        // g.setColour (slider.findColour (Slider::thumbColourId));
        g.setColour(juce::Colour{0x99000000});
        // g.fillEllipse(Rectangle<float> (thumbWidth, thumbWidth).withCentre (thumbPoint));

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

        // g.setColour(juce::Colour{0x99ffffff});
        // g.setColour(juce::Colours::whitesmoke);
        g.fillPath(p);
        // g.setColour(Colour{0x66000000});
        g.strokePath(p, juce::PathStrokeType(2.0f));

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
/// THIS ONE IS SAME AS ORIGINAL BUT CENTERED ///

class KnobLAF3 : public KnobLAF
{

public:
    KnobLAF3() = default;
    ~KnobLAF3() override = default;

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
                2,
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
                4,
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
