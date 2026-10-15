// COMPOSESIREN_RUST_DSP: the voice swaps its synthesis while it plays (SirenVoice::reloadDsp).
// A voice renders as a host would (512-sample blocks, events at their sample) while the DSP
// choice moves cpp -> rust-exact -> trio-f32 -> trio -> cpp. Each change, requested between
// notes, is built off the audio thread, waits for the next note-on, and must keep the voice
// rendering.
// After each swap to cpp or rust-exact the voice must play, bit for bit, what a fresh C++
// voice plays from that note-on (given the same controllers), once the fade is over.
// Usage: ComposeSirenDspHotSwapTests <resources folder, ending with />

#include "lib/wrappers/SirenVoice.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

namespace {

struct Event { int sample; int status, d1, d2; };
constexpr int block = 512;

// what a fresh voice unit plays from `from` (a note-on), given the controllers and pitch wheel before it
std::vector<float> reference(sirenId id, const std::string& res, const char* variant, double rate,
                             const std::vector<Event>& events, int from, int to)
{
    SirenVoiceUnit u(id, res, cs::DspChoice { variant, "fixed" });
    u.setSampleRate(rate);
    for (const auto& e : events)
        if (e.sample < from && ((e.status >> 4) == 0xB || (e.status >> 4) == 0xE)) u.handleMidi(e.status, e.d1, e.d2);
    std::vector<float> out;
    for (int i = from; i < to; ++i) {
        for (const auto& e : events)
            if (e.sample == i) u.handleMidi(e.status, e.d1, e.d2);
        if (i % block == 0) u.beginProcessBlock();
        out.push_back(u.process());
    }
    return out;
}

void waitForLoader(SirenVoice& v)
{
    for (int i = 0; i < 6000 && v.isReloading(); ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

struct PitchProbe : SirenVoice::Listener {
    float pitch = 0;
    void currentSirenState(sirenId, SirenVoice::State s) override { pitch = s.currentPitch; }
};

double rmsDb(const std::vector<float>& x, int from, int to)
{
    double sum = 0;
    for (int i = from; i < to; ++i) sum += double(x[static_cast<size_t>(i)]) * x[static_cast<size_t>(i)];
    return 10.0 * std::log10(std::max(1e-20, sum / std::max(1, to - from)));
}

struct MidChange { int block; const char* variant; };

struct Take {
    std::vector<float> out;
    std::vector<float> pitch; // at each block start
    std::vector<int> swapBlock;
};

// one held note (with a legato move at block 470), starting on `first`, through `changes`
Take renderHeld(const std::string& res, double rate, const char* first, const std::vector<MidChange>& changes)
{
    const int total = 900 * block;
    cs::setDspChoice({ first, "fixed" });
    SirenVoice v;
    PitchProbe probe;
    v.addListener(&probe);
    v.setSirenId(S3, res);
    v.setSampleRate(rate);
    Take t { std::vector<float>(static_cast<size_t>(total)), std::vector<float>(total / block), {} };
    for (int b = 0; b * block < total; ++b) {
        for (const auto& c : changes)
            if (c.block == b) {
                cs::setDspChoice({ c.variant, "fixed" });
                v.reloadDsp();
                waitForLoader(v);
            }
        const auto before = v.swapCount();
        if (!v.getRawSirenHandle()) return t;
        v.beginProcessBlock();
        if (b > 0 && v.swapCount() != before) t.swapBlock.push_back(b); // not the load
        v.notifyListeners();
        t.pitch[static_cast<size_t>(b)] = probe.pitch;
        for (int i = b * block; i < (b + 1) * block; ++i) {
            if (i == 0) v.handleMidi(0x90, 60, 100);
            if (i == 470 * block) v.handleMidi(0x90, 40, 100); // legato: the swap at 500 is mid-glide
            t.out[static_cast<size_t>(i)] = v.process();
        }
        v.collectGarbage();
    }
    v.removeListener(&probe);
    return t;
}

// Note A, released, a DSP change with no note held, then note B: B starts from the pitch the
// siren had (it glides from there, as the motor would), not from the bottom of its range.
// The pitch at each block start, swapped (cpp -> each variant) and unswapped (cpp alone).
template <class Check>
void noteAfterSwap(const std::string& res, double rate, Check& check)
{
    auto run = [&](const char* to, int noteB) {
        cs::setDspChoice({ "cpp", "fixed" });
        SirenVoice v;
        PitchProbe probe;
        v.addListener(&probe);
        v.setSirenId(S3, res);
        v.setSampleRate(rate);
        std::vector<float> pitch;
        for (int b = 0; b < 1100; ++b) {
            if (b == 1020 && to != nullptr) {
                cs::setDspChoice({ to, "fixed" });
                v.reloadDsp();
                waitForLoader(v);
            }
            if (!v.getRawSirenHandle()) break;
            v.beginProcessBlock();
            v.notifyListeners();
            pitch.push_back(probe.pitch);
            for (int i = 0; i < block; ++i) {
                if (b == 0 && i == 0) v.handleMidi(0x90, 72, 100);
                if (b == 1000 && i == 0) v.handleMidi(0x80, 72, 0);
                if (b == 1040 && i == 0) v.handleMidi(0x90, noteB, 100);
                v.process();
            }
            v.collectGarbage();
        }
        v.removeListener(&probe);
        return pitch;
    };
    for (int noteB : { 60, 74 }) {
        const auto ref = run(nullptr, noteB);
        if (std::getenv("HS_TRACE")) for (int b : { 0, 200, 500, 999, 1010, 1039, 1041, 1060, 1099 }) if (b < (int) ref.size()) std::cout << "  pitch[" << b << "] " << ref[static_cast<size_t>(b)] << "\n";
        for (const char* to : { "rust-exact", "trio-f32", "trio" }) {
            const auto p = run(to, noteB);
            if (p.size() < 1100 || ref.size() < 1100) { check(false, "note after a swap: rendered"); return; }
            // block 1041: the first pitch read after B's note-on
            const float before = p[1039], after = p[1041], unswapped = ref[1041];
            const float d = std::abs(after - unswapped);
            check(d < 0.25f && std::abs(after - before) < 1.0f,
                  std::string("note ") + std::to_string(noteB) + " after a swap to " + to
                  + ": starts from the previous pitch (" + std::to_string(before) + " -> " + std::to_string(after)
                  + " semitones, unswapped " + std::to_string(unswapped) + ")");
        }
    }
}

// A held note through every swap: the new synthesis takes the note over at the next block.
// The level is compared with each synthesis playing the same note alone, unswapped: before
// the swap with the old one, after the fade with the new one, during it with their mean
// power (the level a listener expects), so the dB differences are the swap's own.
template <class Check>
void midNote(const std::string& res, double rate, Check& check)
{
    const std::vector<MidChange> changes {
        { 500, "rust-exact" }, { 600, "trio" }, { 700, "trio-f32" }, { 800, "cpp" } };
    const auto t = renderHeld(res, rate, "cpp", changes);
    check(t.swapBlock.size() == changes.size(), "mid-note: each change swaps while the note is held ("
          + std::to_string(t.swapBlock.size()) + " of " + std::to_string(changes.size()) + ")");
    const int w = static_cast<int>(0.05 * rate); // 50 ms windows
    const int fade = static_cast<int>(SirenVoice::fadeSeconds * rate);
    const char* from = "cpp";
    auto alone = renderHeld(res, rate, from, {});
    for (size_t k = 0; k < t.swapBlock.size() && k < changes.size(); ++k) {
        const auto next = renderHeld(res, rate, changes[k].variant, {});
        const int s = t.swapBlock[k] * block;
        const double dBefore = rmsDb(t.out, s - w, s) - rmsDb(alone.out, s - w, s);
        const double expect = 10.0 * std::log10(0.5 * (std::pow(10.0, rmsDb(alone.out, s, s + fade) / 10.0)
                                                      + std::pow(10.0, rmsDb(next.out, s, s + fade) / 10.0)));
        const double dDuring = rmsDb(t.out, s, s + fade) - expect;
        const double dAfter = rmsDb(t.out, s + fade, s + fade + w) - rmsDb(next.out, s + fade, s + fade + w);
        const double dLevel = std::max({ std::abs(dBefore), std::abs(dDuring), std::abs(dAfter) });
        // pitch at the swap block's start (the old unit) and the next one (the new one),
        // against the same note on the new synthesis alone
        const size_t b = static_cast<size_t>(t.swapBlock[k]);
        const double dCents = 100.0 * (t.pitch[b + 1] - next.pitch[b + 1]);
        const double moved = 100.0 * (t.pitch[b + 1] - t.pitch[b - 1]);
        char line[320];
        std::snprintf(line, sizeof line,
            "mid-note %s -> %s at block %d: RMS vs unswapped %+.2f dB before, %+.2f during the fade, "
            "%+.2f after (max %.2f dB); pitch %.3f -> %.3f (moved %+.2f cents, %+.2f cents off the new DSP alone)",
            from, changes[k].variant, t.swapBlock[k], dBefore, dDuring, dAfter, dLevel,
            t.pitch[b - 1], t.pitch[b + 1], moved, dCents);
        check(t.swapBlock[k] == changes[k].block && dLevel < 1.0 && std::abs(dCents) < 1.0, line);
        from = changes[k].variant;
        alone = next;
    }
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2) { std::cerr << "usage: " << argv[0] << " <resources/>\n"; return 2; }
    const std::string res = argv[1];
    const double rate = 48000.0;
    const sirenId id = S3;
    int failures = 0;
    auto check = [&](bool ok, const std::string& what) {
        std::cout << (ok ? "ok   " : "FAIL ") << what << "\n";
        if (!ok) ++failures;
    };

    // the choice at each note-on: changes are requested between notes
    struct Change { int block; const char* variant; };
    const std::vector<Change> changes {
        { 170, "rust-exact" }, { 370, "trio-f32" }, { 570, "trio" }, { 700, "cpp" } };
    const std::vector<Event> events {
        { 0, 0x90, 60, 100 },
        { 40 * block + 17, 0xB0, 1, 64 },        // vibrato
        { 80 * block + 3, 0xE0, 0, 72 },         // pitch wheel
        { 150 * block, 0x80, 60, 0 },
        { 200 * block + 100, 0x90, 64, 110 },    // swap to rust-exact here
        { 350 * block, 0x80, 64, 0 },
        { 400 * block + 7, 0x90, 67, 90 },       // trio-f32
        { 550 * block, 0x80, 67, 0 },
        { 600 * block, 0x90, 62, 100 },          // trio
        { 650 * block, 0x80, 62, 0 },
        { 800 * block + 33, 0x90, 65, 120 },     // cpp
        { 950 * block, 0x80, 65, 0 } };
    const int total = 1000 * block;
    const std::vector<int> swapAt { 200 * block + 100, 400 * block + 7, 600 * block, 800 * block + 33 };

    cs::setDspChoice({ "cpp", "fixed" });
    SirenVoice v;
    v.setSirenId(id, res);
    v.setSampleRate(rate);
    std::vector<float> out(static_cast<size_t>(total));
    std::vector<std::uint32_t> swapsAt;
    bool finite = true;
    for (int b = 0; b * block < total; ++b) {
        for (const auto& c : changes)
            if (c.block == b) {
                cs::setDspChoice({ c.variant, "fixed" });
                v.reloadDsp();
                waitForLoader(v);
                check(v.hasPendingSwap(), std::string("built off the audio thread: ") + c.variant);
            }
        if (!v.getRawSirenHandle()) { check(false, "a siren to play"); return 1; }
        v.beginProcessBlock();
        for (int i = b * block; i < (b + 1) * block; ++i) {
            for (const auto& e : events)
                if (e.sample == i) {
                    v.handleMidi(e.status, e.d1, e.d2);
                    if ((e.status >> 4) == 0x9) swapsAt.push_back(v.swapCount());
                }
            const float s = v.process();
            finite = finite && std::isfinite(s);
            out[static_cast<size_t>(i)] = s;
        }
        v.collectGarbage(); // the message thread's timer
    }
    check(finite, "every sample finite");
    // one swap at load, then one at each note-on after a change
    const std::vector<std::uint32_t> expected { 1, 2, 3, 4, 5 };
    check(swapsAt == expected, "with no note held, swaps only at the note-ons after a change");

    // each segment plays
    for (size_t k = 0; k < swapAt.size(); ++k) {
        float peak = 0;
        for (int i = swapAt[k]; i < swapAt[k] + 100 * block; ++i) peak = std::max(peak, std::abs(out[static_cast<size_t>(i)]));
        check(peak > 1e-3f, std::string("plays after the swap to ") + changes[k].variant + " (peak " + std::to_string(peak) + ")");
    }

    // the swaps carry the siren on: after the swap to rust-exact (the C++ synthesis, bit for
    // bit), the voice plays what one C++ voice playing every event from the start plays
    {
        const int fade = static_cast<int>(SirenVoice::fadeSeconds * rate);
        const int from = swapAt[0], to = swapAt[1];
        const auto ref = reference(id, res, "cpp", rate, events, 0, to);
        float diff = 0;
        for (int i = from + fade; i < to; ++i) diff = std::max(diff, std::abs(ref[static_cast<size_t>(i)] - out[static_cast<size_t>(i)]));
        check(diff < 1e-4f, "after the swap to rust-exact: the C++ voice carried on (max |diff| " + std::to_string(diff) + ")");
    }

    noteAfterSwap(res, rate, check);

    // a siren change still swaps at once, and the voice keeps playing
    v.setSirenId(S4, res);
    const auto before = v.swapCount();
    v.getRawSirenHandle();
    check(v.swapCount() == before + 1, "a siren change swaps at the next block");

    midNote(res, rate, check);

    std::cout << (failures ? "FAILED: " : "ok: ") << failures << " failure(s)\n";
    return failures ? 1 : 0;
}
