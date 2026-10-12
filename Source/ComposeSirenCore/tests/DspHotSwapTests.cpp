// COMPOSESIREN_RUST_DSP: the voice swaps its synthesis while it plays (SirenVoice::reloadDsp).
// A voice renders as a host would (512-sample blocks, events at their sample) while the DSP
// choice moves cpp -> rust-exact -> trio-f32 -> trio -> cpp. Each change is built off the
// audio thread, waits for a note-on with no note held, and must keep the voice rendering.
// After each swap to cpp or rust-exact the voice must play, bit for bit, what a fresh C++
// voice plays from that note-on (given the same controllers), once the fade is over.
// Usage: ComposeSirenDspHotSwapTests <resources folder, ending with />

#include "lib/wrappers/SirenVoice.h"

#include <chrono>
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

    // the choice at each note-on: changes are requested a while before, during a held note
    struct Change { int block; const char* variant; };
    const std::vector<Change> changes {
        { 60, "rust-exact" }, { 260, "trio-f32" }, { 460, "trio" }, { 660, "cpp" } };
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
    check(swapsAt == expected, "swaps only at the note-ons after a change, never during a held note");

    // each segment plays
    for (size_t k = 0; k < swapAt.size(); ++k) {
        float peak = 0;
        for (int i = swapAt[k]; i < swapAt[k] + 100 * block; ++i) peak = std::max(peak, std::abs(out[static_cast<size_t>(i)]));
        check(peak > 1e-3f, std::string("plays after the swap to ") + changes[k].variant + " (peak " + std::to_string(peak) + ")");
    }

    // bit for bit with a fresh C++ voice after the swaps to rust-exact and cpp, once faded
    const int fade = static_cast<int>(SirenVoice::fadeSeconds * rate);
    for (size_t k : { size_t(0), size_t(3) }) {
        const int from = swapAt[k], to = k + 1 < swapAt.size() ? swapAt[k + 1] : total;
        const auto ref = reference(id, res, "cpp", rate, events, from, to);
        const bool same = std::memcmp(ref.data() + fade, out.data() + from + fade,
                                      static_cast<size_t>(to - from - fade) * sizeof(float)) == 0;
        float diff = 0;
        for (int i = fade; i < to - from; ++i) diff = std::max(diff, std::abs(ref[static_cast<size_t>(i)] - out[static_cast<size_t>(from + i)]));
        check(same, std::string("after the swap to ") + changes[k].variant + ": identical to the C++ voice (max |diff| " + std::to_string(diff) + ")");
    }

    // a siren change still swaps at once, and the voice keeps playing
    v.setSirenId(S4, res);
    const auto before = v.swapCount();
    v.getRawSirenHandle();
    check(v.swapCount() == before + 1, "a siren change swaps at the next block");

    std::cout << (failures ? "FAILED: " : "ok: ") << failures << " failure(s)\n";
    return failures ? 1 : 0;
}
