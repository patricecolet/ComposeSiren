// COMPOSESIREN_RUST_DSP: the plugin's voice plays the same samples, bit for bit, with the
// C++ synthesis ("cpp") and the Rust port ("rust-exact"), through the plugin's own MIDI
// handling. The other Rust variants must load and play.
// Usage: ComposeSirenRustDspParityTests <resources folder, ending with />

#include "lib/wrappers/SirenVoice.h"

#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>

namespace {

std::vector<float> render(sirenId id, const std::string& res, const char* variant, double rate, bool& rust)
{
    SirenVoiceUnit u(id, res, cs::DspChoice { variant, "fixed" });
    rust = u.isRust();
    u.setSampleRate(rate);
    const int n = static_cast<int>(rate * 4);
    std::vector<float> out(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        // the events at the start of a 512-sample block, as a host would deliver them
        if (i == 0) u.handleMidi(0x90, 60, 100);
        if (i == 512 * 40) u.handleMidi(0xB0, 1, 64);        // vibrato
        if (i == 512 * 90) u.handleMidi(0xE0, 0, 80);        // pitch bend
        if (i == 512 * 150) u.handleMidi(0x90, 67, 120);     // legato to a fifth
        if (i == 512 * 250) u.handleMidi(0x80, 67, 0);
        if (i % 512 == 0) u.beginProcessBlock();
        out[static_cast<size_t>(i)] = u.process();
    }
    return out;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2) { std::cerr << "usage: " << argv[0] << " <resources/>\n"; return 2; }
    const std::string res = argv[1];
    int failures = 0;
    for (double rate : { 44100.0, 48000.0 }) {
        for (int s = 0; s < 7; ++s) {
            const auto id = static_cast<sirenId>(s);
            bool rustC = true, rustR = false;
            const auto c = render(id, res, "cpp", rate, rustC);
            const auto r = render(id, res, "rust-exact", rate, rustR);
            float peak = 0;
            for (float x : c) peak = std::max(peak, std::abs(x));
            const bool same = c.size() == r.size() && std::memcmp(c.data(), r.data(), c.size() * sizeof(float)) == 0;
            std::cout << "S" << s + 1 << " @" << rate << ": " << (same && !rustC && rustR ? "identical" : "DIFFERENT")
                      << " (peak " << peak << ")\n";
            if (!same || rustC || !rustR || peak < 1e-3f) ++failures;
        }
    }
    for (const char* v : { "rust-time", "frames", "trio", "trio-linear", "trio-nearest" }) {
        bool rust = false;
        const auto o = render(S3, res, v, 48000.0, rust);
        float peak = 0;
        bool finite = true;
        for (float x : o) { peak = std::max(peak, std::abs(x)); finite = finite && std::isfinite(x); }
        std::cout << v << ": " << (rust && finite && peak > 1e-3f ? "plays" : "FAILS") << " (peak " << peak << ")\n";
        if (!rust || !finite || peak <= 1e-3f) ++failures;
    }
    std::cout << (failures ? "FAILED: " : "ok: ") << failures << " failure(s)\n";
    return failures ? 1 : 0;
}
