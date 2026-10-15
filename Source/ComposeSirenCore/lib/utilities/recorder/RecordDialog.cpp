#include "RecordDialog.h"

#if COMPOSESIREN_RECORD

#include "../NonModalDialog.h"

namespace {

juce::String clock(double seconds)
{
    const auto s = juce::roundToInt(seconds);
    return juce::String(s / 60) + ":" + juce::String(s % 60).paddedLeft('0', 2);
}

} // namespace

RecordDialog::RecordDialog(Recorder& r) : recorder(r)
{
    format.addItem("FLAC, 24-bit", 1 + static_cast<int>(Recorder::Format::flac24));
    format.addItem("WAV, 24-bit", 1 + static_cast<int>(Recorder::Format::wav24));
    format.addItem("WAV, 32-bit float", 1 + static_cast<int>(Recorder::Format::wavFloat));
    format.setSelectedId(1, juce::dontSendNotification);
    format.onChange = [this] {
        if (chosen != juce::File())
            chosen = chosen.withFileExtension(Recorder::extension(selectedFormat()));
        refresh();
    };
    file.setColour(juce::Label::outlineColourId, juce::Colours::grey);
    file.setMinimumHorizontalScale(0.6f);
    choose.onClick = [this] { chooseFile(); };
    startStop.onClick = [this] { startOrStop(); };
    fadeOut.setToggleState(true, juce::dontSendNotification);
    fadeOut.setTooltip("On Stop, wait up to 2 s for the sound to die out; if it still sounds (a drone), fade it out over 3 s.");
    fadeOut.setColour(juce::ToggleButton::textColourId, juce::Colours::whitesmoke);
    fadeOut.setColour(juce::ToggleButton::tickColourId, juce::Colours::whitesmoke);
    status.setJustificationType(juce::Justification::centredLeft);

    for (auto* c : std::initializer_list<juce::Component*> { &formatLabel, &format, &fileLabel, &file, &choose,
                                                             &startStop, &fadeOut, &status })
        addAndMakeVisible(c);
    setSize(460, 170);
    refresh();
    startTimerHz(4);
}

RecordDialog::~RecordDialog()
{
    stopTimer();
}

void RecordDialog::show(Recorder& recorder, juce::Component* parent)
{
    cs::NonModalDialog::show("Record", new RecordDialog(recorder), parent, false);
}

Recorder::Format RecordDialog::selectedFormat() const
{
    return static_cast<Recorder::Format>(juce::jmax(0, format.getSelectedId() - 1));
}

void RecordDialog::chooseFile()
{
    const auto ext = Recorder::extension(selectedFormat());
    const auto start = chosen != juce::File() ? chosen : Recorder::defaultFile(selectedFormat());
    chooser = std::make_unique<juce::FileChooser>("Record to", start, "*." + ext);
    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                             | juce::FileBrowserComponent::warnAboutOverwriting,
                         [this, ext](const juce::FileChooser& fc) {
                             const auto result = fc.getResult();
                             if (result != juce::File())
                                 chosen = result.withFileExtension(ext);
                             refresh();
                         });
}

void RecordDialog::startOrStop()
{
    const auto current = recorder.status();
    if (current.fading)
        return;
    if (current.recording) {
        const auto result = fadeOut.getToggleState() ? recorder.stopFading() : recorder.stop();
        if (result.failed())
            cs::showNonModalMessage("Record", result.getErrorMessage(), getTopLevelComponent());
    } else {
        const auto target = chosen != juce::File() ? chosen : Recorder::defaultFile(selectedFormat());
        const auto result = recorder.start(target, selectedFormat());
        if (result.failed())
            cs::showNonModalMessage("Record", result.getErrorMessage(), getTopLevelComponent());
        // A default name is taken once: the next recording gets a new one.
        chosen = juce::File();
    }
    refresh();
}

void RecordDialog::timerCallback()
{
    refresh();
}

void RecordDialog::refresh()
{
    const auto s = recorder.status();
    startStop.setButtonText(s.fading ? "Fading out..." : s.recording ? "Stop" : "Start recording");
    startStop.setEnabled(!s.fading);
    startStop.setColour(juce::TextButton::buttonColourId, s.recording ? juce::Colours::darkred : juce::Colour { 0xff37474f });
    format.setEnabled(!s.recording);
    choose.setEnabled(!s.recording);
    if (s.recording)
        file.setText(s.file.getFullPathName(), juce::dontSendNotification);
    else
        file.setText(chosen != juce::File() ? chosen.getFullPathName()
                                            : Recorder::defaultFile(selectedFormat()).getParentDirectory().getFullPathName()
                                                  + "/ComposeSiren-<date>-<time>." + Recorder::extension(selectedFormat()),
                     juce::dontSendNotification);

    juce::String text;
    if (s.fading)
        text << "Fading out  " << clock(s.seconds());
    else if (s.recording)
        text << "Recording  " << clock(s.seconds());
    else if (s.file != juce::File())
        text << "Last: " << s.file.getFileName() << "  (" << clock(s.seconds()) << ")";
    else
        text << "Not recording";
    if (s.framesDropped > 0)
        text << "   dropped " << juce::String(s.framesDropped) << " frames";
    if (s.error.isNotEmpty())
        text << "   error: " << s.error;
    if (s.sampleRate > 0)
        text << "   " << juce::String(s.sampleRate / 1000.0, 1) << " kHz, " << s.channels << " ch";
    status.setText(text, juce::dontSendNotification);
    repaint();
}

void RecordDialog::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour { 0xff263238 });
    if (recorder.status().recording) {
        g.setColour(juce::Colours::red);
        g.fillEllipse(status.getBounds().toFloat().withWidth(10).withHeight(10).translated(-16, 7));
    }
}

void RecordDialog::resized()
{
    auto area = getLocalBounds().reduced(14);
    auto row = [&area](int h) {
        auto r = area.removeFromTop(h);
        area.removeFromTop(8);
        return r;
    };
    auto r = row(26);
    formatLabel.setBounds(r.removeFromLeft(60));
    format.setBounds(r.removeFromLeft(200));
    r = row(26);
    fileLabel.setBounds(r.removeFromLeft(60));
    choose.setBounds(r.removeFromRight(90));
    r.removeFromRight(6);
    file.setBounds(r);
    r = row(30);
    startStop.setBounds(r.removeFromLeft(160));
    r.removeFromLeft(12);
    fadeOut.setBounds(r.removeFromLeft(120));
    r = row(24);
    status.setBounds(r.withTrimmedLeft(20));
}

#endif
