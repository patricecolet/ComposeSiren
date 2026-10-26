// COMPOSESIREN_RUST_DSP: the plugin's voice plays the same samples, bit for bit, with the
// C++ synthesis ("cpp") and the Rust port ("rust-exact"), through the plugin's own MIDI
// handling. The other Rust variants must load and play.
// Usage: ComposeSirenRustDspParityTests <resources folder, ending with />

#include "Reverb.h"
#include "lib/wrappers/SirenVoice.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <utility>
#include <vector>

namespace {

std::vector<float> render(sirenId id, const std::string& res, const char* variant, double rate, bool& rust, bool skip = false)
{
    SirenVoiceUnit u(id, res, cs::DspChoice { variant, "fixed", skip });
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

std::pair<std::vector<float>, std::vector<float>> renderReverb(
    const std::vector<float>& inL,
    const std::vector<float>& inR,
    double rate,
    bool useRust,
    bool inPlace,
    bool& reportedRust)
{
    Reverb rvb(useRust);
    reportedRust = rvb.isRust();
    rvb.enable(true);
    rvb.setDryWet(0.45f);
    rvb.setDamping(0.35f);
    rvb.setRoomSize(0.75f);
    rvb.setWidth(0.9f);
    rvb.setLowCut(120.0f);
    rvb.setHighCut(7500.0f);
    rvb.setSampleRate(rate);

    const size_t n = std::min(inL.size(), inR.size());
    std::vector<float> outL = inPlace ? inL : std::vector<float>(n, 0.0f);
    std::vector<float> outR = inPlace ? inR : std::vector<float>(n, 0.0f);
    std::vector<float> srcL = inL;
    std::vector<float> srcR = inR;

    const size_t block = 512;
    for (size_t pos = 0; pos < n; pos += block) {
        const int chunk = static_cast<int>(std::min(block, n - pos));
        const size_t b = pos / block;
        if (b == 30) {
            rvb.setDryWet(0.75f);
            rvb.setRoomSize(0.9f);
            rvb.setDamping(0.2f);
            rvb.setWidth(0.6f);
            rvb.setLowCut(220.0f);
            rvb.setHighCut(6000.0f);
        } else if (b == 70) {
            rvb.enable(false);
        } else if (b == 90) {
            rvb.enable(true);
            rvb.setDryWet(0.55f);
            rvb.setLowCut(90.0f);
            rvb.setHighCut(9000.0f);
        }
        rvb.beginProcessBlock();
        if (inPlace) {
            rvb.process(outL.data() + pos, outR.data() + pos, outL.data() + pos, outR.data() + pos, chunk);
        } else {
            rvb.process(srcL.data() + pos, srcR.data() + pos, outL.data() + pos, outR.data() + pos, chunk);
        }
    }
    return { std::move(outL), std::move(outR) };
}

// S6 holding F5 (MIDI 89) looped over its frames (variant "frames"): frame 440 of the data
// is practically silent and clicks; dsp.skip_silent_frames plays frame 441 there.
std::vector<float> holdF5(const std::string& res, bool skip)
{
    SirenVoiceUnit u(S6, res, cs::DspChoice { "frames", "fixed", skip });
    u.setSampleRate(44100.0);
    std::vector<float> out(44100 * 2);
    for (size_t i = 0; i < out.size(); ++i) {
        if (i == 0) u.handleMidi(0x90, 89, 127);
        if (i % 512 == 0) u.beginProcessBlock();
        out[i] = u.process();
    }
    return out;
}

// the click: the peak RMS of the second difference over 128-sample windows, in dB over the
// median window, from 0.5 s
double clickDb(const std::vector<float>& x)
{
    std::vector<double> rms;
    for (size_t w = 22050; w + 130 <= x.size(); w += 128) {
        double s = 0;
        for (size_t k = w; k < w + 128; ++k) {
            const double d = double(x[k + 2]) - 2.0 * x[k + 1] + x[k];
            s += d * d;
        }
        rms.push_back(std::sqrt(s / 128));
    }
    const double peak = *std::max_element(rms.begin(), rms.end());
    std::nth_element(rms.begin(), rms.begin() + rms.size() / 2, rms.end());
    return 20 * std::log10(peak / rms[rms.size() / 2]);
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2) { std::cerr << "usage: " << argv[0] << " <resources/>\n"; return 2; }
    const std::string res = argv[1];
    int failures = 0;
    for (double rate : { 44100.0, 48000.0 }) {
        std::vector<float> s1Signal, s3Signal;
        for (int s = 0; s < 7; ++s) {
            const auto id = static_cast<sirenId>(s);
            bool rustC = true, rustR = false;
            const auto c = render(id, res, "cpp", rate, rustC);
            const auto r = render(id, res, "rust-exact", rate, rustR);
            if (s == 0) s1Signal = c;
            if (s == 2) s3Signal = c;
            float peak = 0;
            for (float x : c) peak = std::max(peak, std::abs(x));
            const bool same = c.size() == r.size() && std::memcmp(c.data(), r.data(), c.size() * sizeof(float)) == 0;
            std::cout << "S" << s + 1 << " @" << rate << ": " << (same && !rustC && rustR ? "identical" : "DIFFERENT")
                      << " (peak " << peak << ")\n";
            if (!same || rustC || !rustR || peak < 1e-3f) ++failures;
        }

        for (bool inPlace : { false, true }) {
            bool rustC = true, rustR = false;
            const auto [cL, cR] = renderReverb(s1Signal, s3Signal, rate, false, inPlace, rustC);
            const auto [rL, rR] = renderReverb(s1Signal, s3Signal, rate, true, inPlace, rustR);
            float peak = 0;
            for (size_t i = 0; i < cL.size(); ++i) {
                peak = std::max(peak, std::max(std::abs(cL[i]), std::abs(cR[i])));
            }
            const bool sameL = cL.size() == rL.size() && std::memcmp(cL.data(), rL.data(), cL.size() * sizeof(float)) == 0;
            const bool sameR = cR.size() == rR.size() && std::memcmp(cR.data(), rR.data(), cR.size() * sizeof(float)) == 0;
            const bool ok = sameL && sameR && !rustC && rustR && peak > 1e-3f;
            std::cout << "Reverb " << (inPlace ? "in-place" : "out-of-place") << " @" << rate << ": "
                      << (ok ? "identical" : "DIFFERENT") << " (peak " << peak << ")\n";
            if (!ok) ++failures;
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
    // dsp.skip_silent_frames: rust-exact never gets past the first frames, so it stays the C++
    // bit for bit; "frames" loses its click
    {
        bool rustC = true, rustR = false;
        const auto c = render(S6, res, "cpp", 44100.0, rustC);
        const auto r = render(S6, res, "rust-exact", 44100.0, rustR, true);
        const bool same = std::memcmp(c.data(), r.data(), c.size() * sizeof(float)) == 0;
        std::cout << "rust-exact, skipping silent frames: " << (same ? "identical" : "DIFFERENT") << "\n";
        if (!same) ++failures;
        const double off = clickDb(holdF5(res, false)), on = clickDb(holdF5(res, true));
        std::cout << "S6 F5 frames, click: " << off << " dB, skipping " << on << " dB\n";
        if (!(off > 3 + on && on < 6)) ++failures;
    }
    std::cout << (failures ? "FAILED: " : "ok: ") << failures << " failure(s)\n";
    return failures ? 1 : 0;
}
