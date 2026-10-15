#include "AboutDialog.h"

#include <BuildInfo.h>

#include <lib/utilities/NonModalDialog.h>

#include "PluginProcessor.h"

namespace {

constexpr int nameColumn = 34;

juce::String line(const juce::String& name, const juce::String& value)
{
    return "  " + name.paddedRight(' ', nameColumn) + value + "\n";
}

juce::String optionsReport()
{
    juce::String text = "CMake options\n";
    for (const auto& option : cs::buildinfo::options)
        text += line(option.name, option.value);
    return text;
}

juce::String mcpReport(McpControl& mcp)
{
    juce::String text = "\nMCP server\n";
#if !COMPOSESIREN_MCP
    return text + "  not built in (COMPOSESIREN_MCP=OFF)\n";
#else
    if (!mcp.isRunning())
        return text + "  not running (no free port, see the log)\n";

    text += "  listening on 127.0.0.1:" + juce::String(mcp.getPort()) + "\n";
    const auto counts = mcp.toolCounts();
    if (counts.empty())
        return text + "  no call yet\n";

    juce::int64 total = 0;
    for (const auto& count : counts) {
        total += count.calls;
        text += line(count.tool, juce::String(count.calls)
                                     + (count.errors > 0 ? "  (" + juce::String(count.errors) + " failed)" : ""));
    }
    return text + line("total", juce::String(total));
#endif
}

#if COMPOSESIREN_MECAVIV_BRIDGE
juce::String daemonReport(mecaviv::Bridge& bridge, bool reachable)
{
    juce::String text = "\nPark daemon (mecaviv-bridge-daemon)\n";
    const auto backend = bridge.backend();
    const auto stats = bridge.daemonStats();
    const auto n = [](std::uint64_t value) { return juce::String(static_cast<juce::int64>(value)); };

    if (backend == mecaviv::Bridge::Backend::daemon)
        text += "  running, " + juce::String(bridge.isEnabled() ? "in use" : "reachable, \"Sirenes physiques\" is off") + "\n";
    else if (reachable)
        text += "  running, not used by this bridge\n";
    else
        text += "  not running"
              + juce::String(backend == mecaviv::Bridge::Backend::in_process ? " (in-process link in use)" : "") + "\n";

    if (stats.sessions == 0)
        return text + "  no call made"
             + (stats.session_failures > 0 ? " (" + n(stats.session_failures) + " connection attempts failed)" : "") + "\n";

    text += line("sessions opened", n(stats.sessions));
    if (stats.session_failures > 0)
        text += line("connection attempts failed", n(stats.session_failures));
    text += line("midi", n(stats.midi)
                             + (stats.midi_ignored > 0 ? "  (" + n(stats.midi_ignored) + " not carried)" : ""));
    text += line("reset (one siren)", n(stats.resets));
    text += line("reset all", n(stats.reset_all));
    text += line("st all", n(stats.st_all));
    text += line("drive states received", n(stats.drive_states));
    return text;
}
#else
juce::String daemonReport()
{
    return juce::String("\nPark daemon (mecaviv-bridge-daemon)\n")
#if COMPOSESIREN_PARK_BRIDGE
         + "  not used: the park is driven by SirenLink (COMPOSESIREN_MECAVIV_BRIDGE=OFF)\n";
#else
         + "  not built in (COMPOSESIREN_PARK_BRIDGE=OFF)\n";
#endif
}
#endif

} // namespace

AboutDialog::AboutDialog(SirenOrchestraPluginProcessor& p) : processor(p)
{
    namespace info = cs::buildinfo;
    const juce::String commit = info::commit;

    title.setText("SirenOrchestra " + juce::String(info::version) + " (" + info::buildType + ")",
                  juce::dontSendNotification);
    title.setFont(juce::FontOptions(20.f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, juce::Colours::whitesmoke);

    commitLabel.setText(commit.isEmpty()
                            ? "Not built from a git checkout"
                            : "Branch " + juce::String(info::branch).quoted() + " at " + commit.substring(0, 12)
                                  + (info::dirty ? " (uncommitted changes)" : ""),
                        juce::dontSendNotification);
    commitLabel.setColour(juce::Label::textColourId, juce::Colours::whitesmoke);
    commitLabel.setTooltip(commit);

    const juce::String url = info::commitUrl;
    if (url.isNotEmpty()) {
        commitLink.setButtonText("Open this commit on GitHub");
        commitLink.setURL(juce::URL(url));
        commitLink.setTooltip(url);
    } else {
        commitLink.setButtonText(commit.isEmpty() ? "" : "This commit is not on GitHub (not pushed, or not fetched)");
        commitLink.setEnabled(false);
    }
    commitLink.setFont(juce::FontOptions(14.f), false, juce::Justification::centredLeft);

    live.setMultiLine(true, false);
    live.setReadOnly(true);
    live.setScrollbarsShown(true);
    live.setCaretVisible(false);
    live.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 13.f, juce::Font::plain));
    live.setColour(juce::TextEditor::backgroundColourId, juce::Colour { 0xff1b2428 });
    live.setColour(juce::TextEditor::textColourId, juce::Colours::whitesmoke);
    live.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);

    for (auto* c : std::initializer_list<juce::Component*> { &title, &commitLabel, &commitLink, &live })
        addAndMakeVisible(c);

    setSize(560, 560);
    refresh();
    startTimerHz(1);
}

AboutDialog::~AboutDialog()
{
    stopTimer();
}

juce::DialogWindow* AboutDialog::show(SirenOrchestraPluginProcessor& processor, juce::Component* parent)
{
    return cs::NonModalDialog::show("About SirenOrchestra", new AboutDialog(processor), parent, true);
}

void AboutDialog::resized()
{
    auto area = getLocalBounds().reduced(12);
    title.setBounds(area.removeFromTop(28));
    commitLabel.setBounds(area.removeFromTop(22));
    commitLink.setBounds(area.removeFromTop(22));
    area.removeFromTop(8);
    live.setBounds(area);
}

void AboutDialog::timerCallback()
{
    refresh();
}

juce::String AboutDialog::liveReport()
{
    auto text = optionsReport() + mcpReport(processor.getMcp());
#if COMPOSESIREN_MECAVIV_BRIDGE
    auto& bridge = processor.getUdpBridge();
    if (bridge.isEnabled()) {
        daemonReachable = bridge.backend() == mecaviv::Bridge::Backend::daemon;
        ticksSinceDaemonProbe = 0;
    } else if (ticksSinceDaemonProbe-- <= 0) {
        daemonReachable = bridge.backend() == mecaviv::Bridge::Backend::daemon;
        ticksSinceDaemonProbe = 5;
    }
    text += daemonReport(bridge, daemonReachable);
#else
    text += daemonReport();
#endif
    return text;
}

void AboutDialog::refresh()
{
    const auto text = liveReport();
    if (text != live.getText()) {
        // keep the scroll position: setText would jump back to the top
        const auto firstVisible = live.getTextIndexAt(0, 0);
        live.setText(text, false);
        live.setCaretPosition(firstVisible);
        live.scrollEditorToPositionCaret(0, 0);
    }
}
