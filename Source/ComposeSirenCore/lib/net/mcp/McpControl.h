#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#if COMPOSESIREN_RESETALLCONTROLLERS
#include <functional>
#endif
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "lib/utilities/recorder/Recorder.h"

#ifndef COMPOSESIREN_MCP
#define COMPOSESIREN_MCP 0
#endif
#if COMPOSESIREN_MCP
#include "composesiren_mcp.h"
#else
struct cs_mcp_server_t;
#endif

#ifndef COMPOSESIREN_SONG_TITLE
#define COMPOSESIREN_SONG_TITLE 0
#endif

// In-process MCP server. Rust owns the HTTP listener and the discovery file
// ~/.composesiren_mcp.json. This class runs the commands on the message thread
// and queues MIDI for the next audio block.

#if COMPOSESIREN_SONG_TITLE
/// What `set_song_title` / `set_song_progress` show in the UI (title bar),
/// and `clear_song_title` ends.
struct SongDisplay
{
    juce::String title;
    double positionSeconds = 0.0;
    double durationSeconds = 0.0; // 0: unknown, no progress bar
    juce::uint32 updatedAtMs = 0;
    bool active = false;

    /// Where the song is now: the last position, moving on with the clock so
    /// the window title keeps filling between two `set_song_progress` calls.
    /// Never past the length when it is known.
    double currentPosition() const
    {
        if (!active)
            return 0.0;
        double at = positionSeconds + (juce::Time::getMillisecondCounter() - updatedAtMs) * 0.001;
        return durationSeconds > 0.0 ? juce::jmin(at, durationSeconds) : at;
    }
};
#endif

class McpControl
#if COMPOSESIREN_SONG_TITLE
    : public juce::ChangeBroadcaster
#endif
{
public:
    McpControl(juce::AudioProcessor& processor, const juce::String& pluginName, const juce::String& pluginCode);
    ~McpControl();

    McpControl(const McpControl&) = delete;
    McpControl& operator=(const McpControl&) = delete;

    // Call from the message thread (the editor timer does). Also invoked by
    // the server when a command arrives.
    void pump();

    // Hold command execution while the editor is being destroyed. New and
    // queued commands fail fast instead of running into a half-dissolved UI
    // (setValueNotifyingHost walks parameter listeners). The server keeps
    // listening; clear this once the editor is gone so headless use works.
    void setDispatchPaused(bool paused);
    bool isDispatchPaused() const { return state->dispatchPaused.load(); }

    // Move queued MIDI into the block, at sample 0, before the host's events.
    void drainMidi(juce::MidiBuffer& midi) const;

    int getPort() const { return port; }

    // Whether the server is listening (COMPOSESIREN_MCP, and it could bind a port).
    bool isRunning() const { return server != nullptr; }

#if COMPOSESIREN_SONG_TITLE
    // The song the last `set_song_title` showed (active false: none).
    SongDisplay getSongDisplay() const;
#endif
#if COMPOSESIREN_RESETALLCONTROLLERS
    // The DSP side of `reset_controllers`. Called on the message thread with
    // 0 for every siren, or a 1-based siren number. It must be thread-safe
    // with the audio thread (SirenEnsemble::requestReset is) and return false
    // when it has no such siren.
    using ResetHandler = std::function<bool(int siren)>;
    void setResetHandler(ResetHandler handler) { resetHandler = std::move(handler); }
    // Message thread only, like the settings listener and resetControllers.
    void setControllerResetEnabled(bool enabled) { controllerResetEnabled = enabled; }
#endif

    struct ToolCount
    {
        juce::String tool;
        juce::int64 calls = 0;
        juce::int64 errors = 0;
    };

    // Calls per tool since the server started, by tool name. Empty when it
    // is not running or no tool was called. Call from any thread; takes a lock
    // in Rust for the length of a map copy.
    std::vector<ToolCount> toolCounts() const;

#if COMPOSESIREN_RECORD
    // The recorder the recording commands drive (null: they fail).
    void setRecorder(Recorder* r) { recorder = r; }
#endif

private:
    struct Job
    {
        std::string request;
        std::string response;
        bool done = false;
        std::mutex mutex;
        std::condition_variable cv;
    };

    struct State
    {
        std::atomic<bool> alive { true };
        std::atomic<bool> shuttingDown { false };
        std::atomic<bool> dispatchPaused { false };
        McpControl* control = nullptr;
        juce::AudioProcessor* processor = nullptr;
        std::mutex jobsMutex;
        std::vector<std::shared_ptr<Job>> jobs;
        mutable std::mutex midiMutex;
        std::vector<juce::MidiMessage> midi;
    };

    static char* dispatch(const char* requestJson, void* user);
    std::string handle(const juce::var& request);
    std::string listParameters() const;
    std::string getParameter(const juce::String& id) const;
    std::string setParameter(const juce::String& id, double value) const;
    std::string sendMidi(int status, int data1, int data2) const;
    std::string listSettings() const;
    std::string getSetting(const juce::String& id) const;
    std::string setSetting(const juce::String& id, const juce::var& value) const;
#if COMPOSESIREN_SONG_TITLE
    std::string setSongTitle(const juce::var& request);
    std::string setSongProgress(const juce::var& request);
    std::string clearSongTitle();

    mutable std::mutex songMutex;
    mutable SongDisplay song;
#endif
#if COMPOSESIREN_RESETALLCONTROLLERS
    std::string resetControllers(int siren) const;
#endif
    juce::RangedAudioParameter* findParameter(const juce::String& id) const;
    static juce::var parameterObject(juce::RangedAudioParameter& parameter);
    static char* duplicate(const std::string& text);

    juce::AudioProcessor& processor;
    std::shared_ptr<State> state;
    cs_mcp_server_t* server = nullptr;
    int port = 0;
#if COMPOSESIREN_RESETALLCONTROLLERS
    ResetHandler resetHandler;
    bool controllerResetEnabled = true;
#endif
#if COMPOSESIREN_RECORD
    Recorder* recorder = nullptr;
    std::string recording(const juce::String& op, const juce::var& request) const;
#endif
};
