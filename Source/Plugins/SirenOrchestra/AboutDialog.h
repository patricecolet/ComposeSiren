#pragma once

// The About window: the build (version, git branch and commit, CMake options),
// and, while they run, the MCP server's calls per tool and the calls made to the
// park daemon. The build part is generated at build time by cmake/BuildInfo.cmake.

#include <juce_gui_basics/juce_gui_basics.h>

class SirenOrchestraPluginProcessor;

class AboutDialog : public juce::Component, private juce::Timer
{
public:
    explicit AboutDialog(SirenOrchestraPluginProcessor& processor);
    ~AboutDialog() override;

    // Opens the dialog in its own non-modal window near `parent`. The window
    // deletes itself when closed; the processor also deletes it on destruction.
    static juce::DialogWindow* show(SirenOrchestraPluginProcessor& processor, juce::Component* parent);

    void resized() override;

private:
    void timerCallback() override;
    void refresh();
    juce::String liveReport();

    SirenOrchestraPluginProcessor& processor;
    juce::Label title;
    juce::Label commitLabel;
    juce::HyperlinkButton commitLink;
    juce::TextEditor live;

    // bridge.backend() connects to the daemon's socket while the bridge is
    // off: ask every few ticks, not at every refresh.
    int ticksSinceDaemonProbe = 0;
    bool daemonReachable = false;
};
