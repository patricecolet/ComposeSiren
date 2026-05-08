//
// Created by joseph larralde on 21/04/2026.
//

#ifndef COMPOSESIREN_SIRENTRACKCOMPONENT_H
#define COMPOSESIREN_SIRENTRACKCOMPONENT_H

#include "SirenStripComponent.h"
#include "lib/wrappers/SirenStateMonitor.h"

class SirenTrackComponent : public juce::Component,
                            public SirenStateMonitor::Listener
{
    //--------------------------------------------------------------------------
    // private component displaying a white outline above whole strip
    class Selection : public juce::Component
    {
        bool enabled;
    public:
        Selection() : enabled(false) { setInterceptsMouseClicks(false, true); }
        ~Selection() override = default;
        void enable(bool e) { enabled = e; }
        void paint(juce::Graphics& g) override {
            if (!enabled) { return; }
            auto area = getLocalBounds().reduced(1, 1).toFloat();
            g.setColour(juce::Colour{0xddffffff});
            g.drawRoundedRectangle(area, controlStripLayout::cornerSize, 2);
        }
        void resized() override { repaint(); }
    };

    //--------------------------------------------------------------------------
    // private component capturing mouse events and notifying the parent
    class MouseListeningTitleLabel : public juce::Label
    {
        SirenTrackComponent* sirenTrack;
    public:
        MouseListeningTitleLabel(SirenTrackComponent* s) : sirenTrack(s) {}
        ~MouseListeningTitleLabel() override = default;
        void mouseEnter(const juce::MouseEvent& event) override {
            setColour(juce::Label::backgroundColourId,
                      juce::Colour{0x22ffffff});
        }
        void mouseExit(const juce::MouseEvent& event) override {
            setColour(juce::Label::backgroundColourId,
                      juce::Colour{0x00000000});
        }
        void mouseDown(const juce::MouseEvent&) override {
            if (isMouseOver()) { sirenTrack->onTitleLabelClicked(); }
        }
    };

public:
    // this is implemented by SirenTrackMenu in SirenOrchestra Editor class
    class Listener
    {
    public:
        Listener() = default;
        virtual ~Listener() = default;
        virtual void sirenTrackSelected(SirenTrackComponent* s) = 0;
    };

    SirenTrackComponent(sirenId sid,
                        juce::AudioProcessorValueTreeState& vts,
                        const parameterLayoutGroupData& layoutGroupData,
                        SirenStateMonitor& ssm);
    ~SirenTrackComponent() override;

    // juce::Component
    void paint(juce::Graphics&) override;
    void resized() override;

    // SirenStateMonitor::Listener
    // void activeSirenIds(const std::vector<sirenId>&) override;
    void currentSirenState(const sirenId, const SirenVoice::State&) override;

    void addListener(Listener* l);
    void removeListener(Listener* l);

    float getMinWidth() const;
    float getTitleWidth() const;
    float getTrackControlsWidth() const;
    float getSirenControlsWidth() const;

    void setShowGroupLabels(bool s);
    void setShowKnobLabels(bool s);
    void setShowTextBox(bool s);
    void setBackgroundColour(juce::Colour c);
    void setBackgroundStripColour(juce::Colour c);

    bool getSelected() const;
    void setSelected(bool s);
    void onTitleLabelClicked();

    void setIsPlayingNote(bool isPlaying);

private:
    sirenId id;
    SirenStripComponent sirenControls;
    SirenStateMonitor& sirenStateMonitor;

    MouseListeningTitleLabel titleLabel{this};
    std::set<Listener*> listeners;
    Selection selection;
    bool selected = false;
    bool isPlayingNote = false;

    juce::Colour backgroundColour{juce::Colours::black};
    juce::Colour backgroundStripColour{juce::Colour{0xff314159}};

    GuiCellGroup trackGroup;

    Spacer spacer1;
    Spacer spacer2;

    SliderCell pan          {sirenControls.ksw, sirenControls.sh, sirenControls.lh};
    SliderCell outputGain   {sirenControls.ksw, sirenControls.sh, sirenControls.lh};
};

#endif //COMPOSESIREN_SIRENTRACKCOMPONENT_H