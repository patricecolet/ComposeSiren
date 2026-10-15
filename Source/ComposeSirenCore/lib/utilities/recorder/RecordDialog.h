#pragma once

// The Record dialog (COMPOSESIREN_RECORD): format, file, start and stop, and
// the recording's duration and dropped frames. The MCP tools start_recording,
// stop_recording and recording_status drive the same recorder.

#include "Recorder.h"

#if COMPOSESIREN_RECORD

#include <juce_gui_basics/juce_gui_basics.h>

class RecordDialog : public juce::Component, private juce::Timer
{
public:
    explicit RecordDialog(Recorder& recorder);
    ~RecordDialog() override;

    // Opens the dialog in its own non-modal window near `parent`. The window
    // deletes itself when closed.
    static void show(Recorder& recorder, juce::Component* parent);

    void resized() override;
    void paint(juce::Graphics& g) override;

private:
    void timerCallback() override;
    void chooseFile();
    void startOrStop();
    void refresh();
    Recorder::Format selectedFormat() const;

    Recorder& recorder;
    juce::Label formatLabel { {}, "Format" };
    juce::ComboBox format;
    juce::Label fileLabel { {}, "File" };
    juce::Label file;
    juce::TextButton choose { "Choose..." };
    juce::TextButton startStop { "Start recording" };
    juce::ToggleButton fadeOut { "Fade out" };
    juce::Label status;
    juce::File chosen;
    std::unique_ptr<juce::FileChooser> chooser;
};

#endif
