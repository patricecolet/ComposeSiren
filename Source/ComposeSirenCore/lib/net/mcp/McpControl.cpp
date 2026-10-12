#include "McpControl.h"

#include "McpDeviceHooks.h"

#include <cstring>
#include <optional>

#include "lib/settings/Settings.h"

namespace {

const char* hostOwnsDevices =
    "Audio and MIDI devices belong to the host. Open the standalone to change them.";

juce::var failure(const juce::String& message)
{
    auto* object = new juce::DynamicObject();
    object->setProperty("ok", false);
    object->setProperty("error", message);
    return juce::var(object);
}

juce::String jsonOf(const juce::var& value)
{
    return juce::JSON::toString(value, true);
}

juce::var okObject()
{
    auto* object = new juce::DynamicObject();
    object->setProperty("ok", true);
    return juce::var(object);
}

} // namespace

McpControl::McpControl(juce::AudioProcessor& processorIn,
                       const juce::String& pluginName,
                       const juce::String& pluginCode)
    : processor(processorIn),
      state(std::make_shared<State>())
{
    state->processor = &processor;
    state->control = this;

#if COMPOSESIREN_MCP
    const int standalone = McpDeviceHooks::installed() ? 1 : 0;
    server = cs_mcp_start(pluginName.toRawUTF8(),
                          pluginCode.toRawUTF8(),
                          standalone,
                          &McpControl::dispatch,
                          state.get(),
                          &port);
#else
    juce::ignoreUnused(pluginName, pluginCode);
#endif
}

McpControl::~McpControl()
{
    state->shuttingDown.store(true);
    pump();
    state->control = nullptr;
#if COMPOSESIREN_MCP
    cs_mcp_stop(server);
#endif
    server = nullptr;
    state->alive.store(false);
    state->processor = nullptr;
}

std::vector<McpControl::ToolCount> McpControl::toolCounts() const
{
    std::vector<ToolCount> counts;
#if COMPOSESIREN_MCP
    if (server == nullptr)
        return counts;
    char* text = cs_mcp_stats_json(server);
    if (text == nullptr)
        return counts;
    const auto parsed = juce::JSON::parse(juce::String::fromUTF8(text));
    cs_mcp_free_string(text);
    if (auto* tools = parsed.getProperty("tools", {}).getDynamicObject()) {
        for (const auto& entry : tools->getProperties())
            counts.push_back({ entry.name.toString(),
                               static_cast<juce::int64>(entry.value.getProperty("calls", 0)),
                               static_cast<juce::int64>(entry.value.getProperty("errors", 0)) });
    }
#endif
    return counts;
}

void McpControl::pump()
{
    if (!state->alive.load() || state->processor == nullptr)
        return;

    std::vector<std::shared_ptr<Job>> jobs;
    {
        std::lock_guard lock(state->jobsMutex);
        jobs.swap(state->jobs);
    }

    for (const auto& job : jobs) {
        juce::var parsed;
        const auto parseResult = juce::JSON::parse(juce::String(job->request), parsed);
        const auto response = parseResult.wasOk()
            ? handle(parsed)
            : jsonOf(failure("the command was not JSON")).toStdString();
        {
            std::lock_guard lock(job->mutex);
            job->response = response;
            job->done = true;
        }
        job->cv.notify_one();
    }
}

void McpControl::drainMidi(juce::MidiBuffer& midi) const
{
    std::vector<juce::MidiMessage> messages;
    {
        std::lock_guard lock(state->midiMutex);
        messages.swap(state->midi);
    }
    for (const auto& message : messages)
        midi.addEvent(message, 0);
}

char* McpControl::dispatch(const char* requestJson, void* user)
{
    auto* raw = static_cast<State*>(user);
    if (raw == nullptr || raw->shuttingDown.load() || !raw->alive.load())
        return duplicate("{\"ok\":false,\"error\":\"server is stopping\"}");

    // The state is owned by a shared_ptr in McpControl. The callback only has
    // the raw pointer; the server is stopped before that pointer is cleared.
    auto job = std::make_shared<Job>();
    job->request = requestJson != nullptr ? requestJson : "";
    {
        std::lock_guard lock(raw->jobsMutex);
        if (raw->shuttingDown.load())
            return duplicate("{\"ok\":false,\"error\":\"server is stopping\"}");
        raw->jobs.push_back(job);
    }

    auto* messageManager = juce::MessageManager::getInstanceWithoutCreating();
    if (messageManager != nullptr && messageManager->isThisTheMessageThread()) {
        if (raw->control != nullptr)
            raw->control->pump();
    } else if (messageManager != nullptr) {
        juce::MessageManager::callAsync([raw] {
            if (!raw->alive.load() || raw->control == nullptr)
                return;
            raw->control->pump();
        });
    }

    {
        std::unique_lock lock(job->mutex);
        job->cv.wait_for(lock, std::chrono::seconds(2), [&] { return job->done; });
        if (!job->done)
            return duplicate("{\"ok\":false,\"error\":\"the message thread did not answer\"}");
        return duplicate(job->response);
    }
}

char* McpControl::duplicate(const std::string& text)
{
    auto* copy = static_cast<char*>(std::malloc(text.size() + 1));
    if (copy == nullptr)
        return nullptr;
    std::memcpy(copy, text.data(), text.size() + 1);
    return copy;
}

std::string McpControl::handle(const juce::var& request)
{
    const auto op = request.getProperty("op", {}).toString();

    if (op == "list_parameters")
        return listParameters();
    if (op == "get_parameter")
        return getParameter(request.getProperty("id", {}).toString());
    if (op == "set_parameter")
        return setParameter(request.getProperty("id", {}).toString(),
                            static_cast<double>(request.getProperty("value", 0)));
    if (op == "send_midi")
        return sendMidi(static_cast<int>(request.getProperty("status", 0)),
                        static_cast<int>(request.getProperty("data1", 0)),
                        static_cast<int>(request.getProperty("data2", 0)));
#if COMPOSESIREN_SONG_TITLE
    if (op == "set_song_title")
        return setSongTitle(request);
    if (op == "set_song_progress")
        return setSongProgress(request);
    if (op == "clear_song_title")
        return clearSongTitle();
#endif

    if (op == "list_settings")
        return listSettings();
    if (op == "get_setting")
        return getSetting(request.getProperty("id", {}).toString());
    if (op == "set_setting")
        return setSetting(request.getProperty("id", {}).toString(),
                          request.getProperty("value", {}));

#if COMPOSESIREN_RESETALLCONTROLLERS
    if (op == "reset_controllers")
        return resetControllers(static_cast<int>(request.getProperty("siren", 0)));

#endif
#if COMPOSESIREN_RECORD
    if (op == "start_recording" || op == "stop_recording" || op == "recording_status")
        return recording(op, request);
#endif

    const auto hooks = McpDeviceHooks::copy();
    if (op == "list_audio_devices")
        return jsonOf(hooks.listAudio ? hooks.listAudio() : failure(hostOwnsDevices)).toStdString();
    if (op == "set_audio_device")
        return jsonOf(hooks.setAudio ? hooks.setAudio(request) : failure(hostOwnsDevices)).toStdString();
    if (op == "list_midi_devices")
        return jsonOf(hooks.listMidi ? hooks.listMidi() : failure(hostOwnsDevices)).toStdString();
    if (op == "set_midi_input" || op == "set_midi_output")
        return jsonOf(hooks.setMidi ? hooks.setMidi(request) : failure(hostOwnsDevices)).toStdString();

    return jsonOf(failure("unknown command " + op)).toStdString();
}

juce::RangedAudioParameter* McpControl::findParameter(const juce::String& id) const
{
    juce::RangedAudioParameter* byName = nullptr;
    int nameMatches = 0;

    for (auto* parameter : processor.getParameters()) {
        auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(parameter);
        if (ranged == nullptr)
            continue;
        if (ranged->getParameterID() == id)
            return ranged;
        if (ranged->getName(128).equalsIgnoreCase(id)) {
            byName = ranged;
            ++nameMatches;
        }
    }
    return nameMatches == 1 ? byName : nullptr;
}

juce::var McpControl::parameterObject(juce::RangedAudioParameter& parameter)
{
    const auto range = parameter.getNormalisableRange();
    auto* object = new juce::DynamicObject();
    object->setProperty("id", parameter.getParameterID());
    object->setProperty("name", parameter.getName(128));
    object->setProperty("value", parameter.convertFrom0to1(parameter.getValue()));
    object->setProperty("text", parameter.getCurrentValueAsText());
    object->setProperty("minimum", range.start);
    object->setProperty("maximum", range.end);
    object->setProperty("default", parameter.convertFrom0to1(parameter.getDefaultValue()));
    return juce::var(object);
}

std::string McpControl::listParameters() const
{
    juce::Array<juce::var> parameters;
    for (auto* parameter : processor.getParameters()) {
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(parameter))
            parameters.add(parameterObject(*ranged));
    }
    auto* object = new juce::DynamicObject();
    object->setProperty("ok", true);
    object->setProperty("parameters", parameters);
    return jsonOf(juce::var(object)).toStdString();
}

std::string McpControl::getParameter(const juce::String& id) const
{
    auto* parameter = findParameter(id);
    if (parameter == nullptr)
        return jsonOf(failure("no parameter named " + id)).toStdString();
    auto object = parameterObject(*parameter);
    object.getDynamicObject()->setProperty("ok", true);
    return jsonOf(object).toStdString();
}

std::string McpControl::setParameter(const juce::String& id, double value) const
{
    auto* parameter = findParameter(id);
    if (parameter == nullptr)
        return jsonOf(failure("no parameter named " + id)).toStdString();

    const auto normalised = parameter->convertTo0to1(static_cast<float>(value));
    parameter->beginChangeGesture();
    parameter->setValueNotifyingHost(normalised);
    parameter->endChangeGesture();

    auto object = parameterObject(*parameter);
    object.getDynamicObject()->setProperty("ok", true);
    return jsonOf(object).toStdString();
}

namespace {

std::optional<cs::Settings::Id> settingIdFromString(const juce::String& name)
{
    for (std::size_t i = 0; i < cs::Settings::settingCount; ++i) {
        const auto id = cs::Settings::idAt(i);
        const auto& d = cs::Settings::describe(id);
        if (name == juce::String(d.id.data(), d.id.size()))
            return id;
    }
    return std::nullopt;
}

juce::var settingObject(cs::Settings& settings, cs::Settings::Id id)
{
    const auto& d = cs::Settings::describe(id);
    auto* object = new juce::DynamicObject();
    object->setProperty("id", juce::String(d.id.data(), d.id.size()));
    object->setProperty("group", juce::String(d.group.data(), d.group.size()));
    object->setProperty("name", juce::String(d.label.data(), d.label.size()));
    object->setProperty("type", juce::String(
        d.type == mecaviv::metadata::SettingType::String ? "string"
        : d.type == mecaviv::metadata::SettingType::Bool ? "bool"
        : d.type == mecaviv::metadata::SettingType::Int ? "int"
        : d.type == mecaviv::metadata::SettingType::Choice ? "choice"
                                                          : "float"));
    object->setProperty("available", cs::Settings::isAvailable(id));
    if (d.type == mecaviv::metadata::SettingType::String)
        object->setProperty("value", settings.getString(id));
    else
        object->setProperty("value", settings.get(id));
    return juce::var(object);
}

} // namespace

std::string McpControl::listSettings() const
{
    juce::SharedResourcePointer<cs::Settings> settings;
    juce::Array<juce::var> rows;
    for (std::size_t i = 0; i < cs::Settings::settingCount; ++i)
        rows.add(settingObject(*settings, cs::Settings::idAt(i)));
    auto* object = new juce::DynamicObject();
    object->setProperty("ok", true);
    object->setProperty("settings", rows);
    return jsonOf(juce::var(object)).toStdString();
}

std::string McpControl::getSetting(const juce::String& id) const
{
    const auto parsed = settingIdFromString(id);
    if (!parsed)
        return jsonOf(failure("no setting named " + id)).toStdString();
    juce::SharedResourcePointer<cs::Settings> settings;
    auto object = settingObject(*settings, *parsed);
    object.getDynamicObject()->setProperty("ok", true);
    return jsonOf(object).toStdString();
}

std::string McpControl::setSetting(const juce::String& id, const juce::var& value) const
{
    const auto parsed = settingIdFromString(id);
    if (!parsed)
        return jsonOf(failure("no setting named " + id)).toStdString();
    if (!cs::Settings::isAvailable(*parsed))
        return jsonOf(failure("setting " + id + " is not in this build")).toStdString();

    juce::SharedResourcePointer<cs::Settings> settings;
    const auto& d = cs::Settings::describe(*parsed);
    if (d.type == mecaviv::metadata::SettingType::String)
        settings->setString(*parsed, value.toString());
    else
        settings->set(*parsed, static_cast<double>(value));

    auto object = settingObject(*settings, *parsed);
    object.getDynamicObject()->setProperty("ok", true);
    return jsonOf(object).toStdString();
}

std::string McpControl::sendMidi(int status, int data1, int data2) const
{
    if (status < 0 || status > 255 || data1 < 0 || data1 > 127 || data2 < 0 || data2 > 127)
        return jsonOf(failure("MIDI bytes are out of range")).toStdString();

    const unsigned char bytes[] = {
        static_cast<unsigned char>(status),
        static_cast<unsigned char>(data1),
        static_cast<unsigned char>(data2),
    };
    {
        std::lock_guard lock(state->midiMutex);
        state->midi.emplace_back(bytes, 3);
    }
    auto object = okObject();
    object.getDynamicObject()->setProperty("queued", true);
    return jsonOf(object).toStdString();
}

#if COMPOSESIREN_SONG_TITLE
namespace {

juce::var songObject(const SongDisplay& song)
{
    auto object = okObject();
    auto* o = object.getDynamicObject();
    o->setProperty("title", song.title);
    o->setProperty("position_seconds", song.currentPosition());
    o->setProperty("duration_seconds", song.durationSeconds);
    o->setProperty("active", song.active);
    return object;
}

} // namespace

SongDisplay McpControl::getSongDisplay() const
{
    std::lock_guard lock(songMutex);
    return song;
}

std::string McpControl::setSongTitle(const juce::var& request)
{
    const auto title = request.getProperty("title", {}).toString().trim();
    if (title.isEmpty())
        return clearSongTitle();

    SongDisplay next;
    next.title = title;
    next.durationSeconds = juce::jmax(0.0, static_cast<double>(request.getProperty("duration_seconds", 0)));
    next.positionSeconds = 0.0;
    next.updatedAtMs = juce::Time::getMillisecondCounter();
    next.active = true;
    {
        std::lock_guard lock(songMutex);
        song = next;
    }
    sendChangeMessage();
    return jsonOf(songObject(next)).toStdString();
}

std::string McpControl::setSongProgress(const juce::var& request)
{
    SongDisplay next;
    {
        std::lock_guard lock(songMutex);
        next = song;
    }
    if (!next.active)
        return jsonOf(failure("no song title is set (set_song_title first)")).toStdString();

    next.positionSeconds = juce::jmax(0.0, static_cast<double>(request.getProperty("position_seconds", 0)));
    const auto duration = request.getProperty("duration_seconds", {});
    if (!duration.isVoid())
        next.durationSeconds = juce::jmax(0.0, static_cast<double>(duration));
    next.updatedAtMs = juce::Time::getMillisecondCounter();
    {
        std::lock_guard lock(songMutex);
        song = next;
    }
    sendChangeMessage();
    return jsonOf(songObject(next)).toStdString();
}

std::string McpControl::clearSongTitle()
{
    {
        std::lock_guard lock(songMutex);
        song = {};
    }
    sendChangeMessage();
    return jsonOf(okObject()).toStdString();
}
#endif // COMPOSESIREN_SONG_TITLE

#if COMPOSESIREN_RESETALLCONTROLLERS
std::string McpControl::resetControllers(int siren) const
{
    if (!controllerResetEnabled)
        return jsonOf(failure("Controller reset is disabled in Settings.")).toStdString();
    if (!resetHandler)
        return jsonOf(failure("this plugin cannot reset its controllers")).toStdString();
    if (siren < 0 || siren > 16)
        return jsonOf(failure("siren must be a siren number, or omitted for every siren")).toStdString();

    // The DSP forgets its controllers, releases its notes and goes silent. It
    // does that on the audio thread, at the start of the next block.
    if (!resetHandler(siren))
        return jsonOf(failure("no such siren")).toStdString();

    // Then the parameters go back to their defaults, so that the UI, the host
    // and the DSP agree (each change is bridged to its MIDI CC). Siren
    // parameters are the ones of the groups S (one siren) and S1, S2, ...;
    // reverb (R) and master (M) are left alone.
    const auto wanted = siren > 0 ? "S" + juce::String(siren) : juce::String();
    int count = 0;
    for (auto* parameter : processor.getParameters()) {
        auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(parameter);
        if (ranged == nullptr)
            continue;
        const auto id = ranged->getParameterID();
        const auto group = id.upToFirstOccurrenceOf(" | ", false, false);
        if (!group.startsWithChar('S') || (group.length() > 1 && !group.substring(1).containsOnly("0123456789")))
            continue;
        if (siren > 0 && group != "S" && group != wanted)
            continue;
        ranged->beginChangeGesture();
        ranged->setValueNotifyingHost(ranged->getDefaultValue());
        ranged->endChangeGesture();
        ++count;
    }

    auto object = okObject();
    object.getDynamicObject()->setProperty("siren", siren > 0 ? juce::var(siren) : juce::var("all"));
    object.getDynamicObject()->setProperty("parameters", count);
    return jsonOf(object).toStdString();
}

#endif
#if COMPOSESIREN_RECORD
std::string McpControl::recording(const juce::String& op, const juce::var& request) const
{
    if (recorder == nullptr)
        return jsonOf(failure("this plugin does not record")).toStdString();

    auto describe = [this] {
        const auto s = recorder->status();
        auto object = okObject();
        auto* o = object.getDynamicObject();
        o->setProperty("recording", s.recording);
        o->setProperty("fading", s.fading);
        o->setProperty("path", s.file.getFullPathName());
        o->setProperty("format", s.format ? Recorder::name(*s.format) : juce::String());
        o->setProperty("sampleRate", s.sampleRate);
        o->setProperty("channels", s.channels);
        o->setProperty("seconds", s.seconds());
        o->setProperty("framesDropped", static_cast<juce::int64>(s.framesDropped));
        if (s.error.isNotEmpty())
            o->setProperty("error", s.error);
        return object;
    };

    if (op == "start_recording") {
        const auto formatName = request.getProperty("format", {}).toString();
        auto format = Recorder::Format::flac24;
        if (formatName.isNotEmpty()) {
            const auto named = Recorder::formatNamed(formatName);
            if (!named)
                return jsonOf(failure("unknown format " + formatName + ": flac, wav or wav-float")).toStdString();
            format = *named;
        }
        const auto path = request.getProperty("path", {}).toString();
        const auto file = path.isNotEmpty() ? juce::File::getCurrentWorkingDirectory().getChildFile(path)
                                            : Recorder::defaultFile(format);
        const auto result = recorder->start(file, format);
        if (result.failed())
            return jsonOf(failure(result.getErrorMessage())).toStdString();
        return jsonOf(describe()).toStdString();
    }
    if (op == "stop_recording") {
        // fade: end without cutting a sound short (see Recorder::stopFading);
        // the reply comes at once, recording_status says when it has ended.
        const auto fade = static_cast<bool>(request.getProperty("fade", false));
        const auto wait = request.getProperty("wait_seconds", {});
        const auto length = request.getProperty("fade_seconds", {});
        const auto result = fade ? recorder->stopFading(wait.isVoid() ? 2.0 : static_cast<double>(wait),
                                                        length.isVoid() ? 3.0 : static_cast<double>(length))
                                 : recorder->stop();
        if (result.failed())
            return jsonOf(failure(result.getErrorMessage())).toStdString();
        return jsonOf(describe()).toStdString();
    }
    return jsonOf(describe()).toStdString();
}
#endif
