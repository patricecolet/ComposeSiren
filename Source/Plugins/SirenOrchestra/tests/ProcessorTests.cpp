// The real SirenOrchestra processor, as the standalone app runs it (its settings file
// included, so the DSP choice is the machine's).
// - reload (the saved state must hold values set just before saving: getStateInformation
//   flushes the parameters, APVTS only copies them into its tree on a timer): a processor that saved vibrato (speed, depth, acceleration), a volume and the
//   reverb switch, then a new one restored from that state, plays what a processor given the
//   same values directly plays, and its parameters (what the editor shows) hold them;
// - `render <channel> <midi note> <seconds> <out.wav>`: the processor playing one note, to a
//   16-bit WAV (left output), for click hunting.
// Usage: SirenOrchestraProcessorTests [render ...]

#include "../PluginProcessor.h"
#include "pathUtilities.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <vector>

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

namespace {

constexpr double rate = 48000.0;
constexpr int block = 512;

std::unique_ptr<juce::AudioProcessor> make() { return std::unique_ptr<juce::AudioProcessor>(createPluginFilter()); }

juce::RangedAudioParameter* param(juce::AudioProcessor& p, const char* group, ParameterId id)
{
    const juce::String want(ParameterIdGet::toJuceParameterId(group, id));
    for (auto* q : p.getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*>(q); r != nullptr && r->getParameterID() == want) return r;
    std::cerr << "no parameter " << want << "\n";
    std::abort();
}

void set(juce::AudioProcessor& p, const char* group, ParameterId id, float v)
{
    auto* q = param(p, group, id);
    q->setValueNotifyingHost(q->convertTo0to1(v));
}

float get(juce::AudioProcessor& p, const char* group, ParameterId id)
{
    auto* q = param(p, group, id);
    return q->convertFrom0to1(q->getValue());
}

// let the message thread run (async updates, the loader's notifications)
void pump(int ms) { juce::Thread::sleep(ms); }

std::vector<float> play(juce::AudioProcessor& p, int channel, int note, int blocks, int noteAt, int noteOff = -1)
{
    std::vector<float> out;
    juce::AudioBuffer<float> buf(p.getTotalNumOutputChannels(), block);
    for (int b = 0; b < blocks; ++b) {
        juce::MidiBuffer midi;
        if (b == noteAt) midi.addEvent(juce::MidiMessage::noteOn(channel, note, (juce::uint8) 110), 0);
        if (b == noteOff) midi.addEvent(juce::MidiMessage::noteOff(channel, note), 0);
        buf.clear();
        p.processBlock(buf, midi);
        out.insert(out.end(), buf.getReadPointer(0), buf.getReadPointer(0) + block);
        if (b % 8 == 0) pump(1);
    }
    return out;
}

void wav(const juce::String& path, const std::vector<float>& x)
{
    juce::File f(path);
    f.deleteFile();
    juce::MemoryOutputStream o;
    const auto n = static_cast<juce::uint32>(x.size() * 2);
    o.write("RIFF", 4); o.writeInt(static_cast<int>(36 + n)); o.write("WAVEfmt ", 8); o.writeInt(16);
    o.writeShort(1); o.writeShort(1); o.writeInt(static_cast<int>(rate)); o.writeInt(static_cast<int>(rate * 2));
    o.writeShort(2); o.writeShort(16); o.write("data", 4); o.writeInt(static_cast<int>(n));
    float peak = 1e-9f;
    for (float s : x) peak = std::max(peak, std::abs(s));
    for (float s : x) o.writeShort(static_cast<short>(s / peak * 0.9f * 32767.0f));
    f.replaceWithData(o.getData(), o.getDataSize());
}

} // namespace

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc >= 6 && juce::String(argv[1]) == "render") {
        auto p = make();
        p->prepareToPlay(rate, block);
        pump(1500); // the sirens load
        const int blocks = static_cast<int>(std::atof(argv[4]) * rate / block);
        wav(argv[5], play(*p, std::atoi(argv[2]), std::atoi(argv[3]), blocks, 20));
        p->releaseResources();
        return 0;
    }

    if (!juce::File(getDefaultResourcesPath()).isDirectory()) {
        std::cout << "skipped: no data at " << getDefaultResourcesPath() << "\n";
        return 0;
    }
    int failures = 0;
    auto check = [&](bool ok, const std::string& what) {
        std::cout << (ok ? "ok   " : "FAIL ") << what << "\n";
        if (!ok) ++failures;
    };

    struct V { ParameterId id; float v; };
    const std::vector<V> values { { ParameterId::VibratoFrequency, 80 }, { ParameterId::VibratoAmplitude, 100 },
                                  { ParameterId::VibratoAcceleration, 40 }, { ParameterId::Volume, 90 } };
    juce::MemoryBlock saved;
    {
        auto a = make();
        for (auto& v : values) set(*a, "S3", v.id, v.v);
        set(*a, "R", ParameterId::ReverbEnable, 1);
        pump(50);
        a->getStateInformation(saved);
    }

    // as the standalone app does at launch: the processor, its state, then the audio
    auto b = make();
    b->setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
    b->prepareToPlay(rate, block);
    pump(1500);
    for (auto& v : values)
        check(std::abs(get(*b, "S3", v.id) - v.v) < 0.5f,
              std::string("restored ") + ParameterIdGet::codeName(v.id) + " = " + std::to_string(get(*b, "S3", v.id)));
    check(get(*b, "R", ParameterId::ReverbEnable) > 0.5f, "restored reverb on");

    // the same values given directly
    auto c = make();
    c->prepareToPlay(rate, block);
    pump(1500);
    for (auto& v : values) set(*c, "S3", v.id, v.v);
    set(*c, "R", ParameterId::ReverbEnable, 1);
    pump(50);

    // and none: the vibrato must make a difference
    auto d = make();
    d->prepareToPlay(rate, block);
    pump(1500);
    set(*d, "S3", ParameterId::Volume, 90);
    set(*d, "R", ParameterId::ReverbEnable, 1);
    pump(50);

    const int blocks = 600, noteAt = 40;
    const auto xb = play(*b, 3, 60, blocks, noteAt);
    const auto xc = play(*c, 3, 60, blocks, noteAt);
    const auto xd = play(*d, 3, 60, blocks, noteAt);
    auto rms = [](const std::vector<float>& x, const std::vector<float>& y) {
        double s = 0, r = 0;
        for (size_t i = noteAt * block; i < x.size(); ++i) { s += double(x[i] - y[i]) * (x[i] - y[i]); r += double(y[i]) * y[i]; }
        return 10.0 * std::log10(std::max(1e-30, s) / std::max(1e-30, r));
    };
    const double bc = rms(xb, xc), dc = rms(xd, xc);
    check(dc > -20.0, "vibrato audible (no-vibrato vs vibrato: " + std::to_string(dc) + " dB)");
    check(bc < -60.0, "reloaded plays as set directly (difference " + std::to_string(bc) + " dB)");

    for (auto* p : { b.get(), c.get(), d.get() }) p->releaseResources();
    std::cout << (failures ? "FAILED: " : "ok: ") << failures << " failure(s)\n";
    return failures ? 1 : 0;
}
